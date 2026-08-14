import { describe, expect, test } from "vitest";
import canonicalRepository from "../../contracts/v2/repository/canonical.json";
import { gzipCompress } from "../src/v2/gzip";
import { v2Limits } from "../src/v2/limits";
import type { Repository } from "../src/v2/repository";
import { serializeRepository } from "../src/v2/repository";
import { createEmptyRepository } from "../src/v2/repositoryValidation";
import { createRepositoryWorkingCopyStore } from "../src/v2/repositoryWorkingCopy";
import {
  SnapshotCommitUncertainError,
  SnapshotTooLargeError,
  SnapshotValidationError,
  deleteSnapshot,
  downloadSnapshotBytes,
  exportRepository,
  importRepository,
  listSnapshots,
  loadSnapshotIntoWorkingCopy,
  replaceSnapshotWithWorkingCopy,
  saveWorkingCopyAsSnapshot,
} from "../src/v2/snapshotClient";
import {
  getFetchCalls,
  jsonResponse,
  planFetch,
  planJsonResponse,
} from "./fakeFetch";

const canonical = canonicalRepository as Repository;

async function canonicalGzipBytes(): Promise<Uint8Array> {
  return gzipCompress(new TextEncoder().encode(serializeRepository(canonical)));
}

function planEmptySnapshotList(): void {
  planJsonResponse({ blobs: [], usedBytes: 0, remainingBytes: 1_000_000 });
}

function binaryResponse(bytes: Uint8Array): Response {
  return new Response(bytes, {
    status: 200,
    headers: { "Content-Type": "application/gzip" },
  });
}

describe("v2 snapshot client", () => {
  test("listSnapshots returns the validated blob list", async () => {
    planJsonResponse({
      blobs: [
        { id: "3", sizeBytes: 10 },
        { id: "2", sizeBytes: 20 },
      ],
      usedBytes: 30,
      remainingBytes: 100,
    });
    const result = await listSnapshots();
    expect(result.blobs).toHaveLength(2);
    const [call] = getFetchCalls();
    expect(call?.url).toBe("/api/v1/blob");
    expect(call?.method).toBe("GET");
  });

  test("saveWorkingCopyAsSnapshot uploads gzip bytes and clears dirty on 201", async () => {
    const store = createRepositoryWorkingCopyStore(createEmptyRepository());
    store.applyContentChange(canonical);
    expect(store.getIsDirty()).toBe(true);

    planEmptySnapshotList();
    planFetch((call) => {
      expect(call.method).toBe("POST");
      expect(call.headers.get("Content-Type")).toBe("application/gzip");
      expect(call.url).toBe("/api/v1/blob");
      return jsonResponse({ id: "4", sizeBytes: 42 }, 201);
    });

    const created = await saveWorkingCopyAsSnapshot(store);
    expect(created).toEqual({ id: "4", sizeBytes: 42 });
    expect(store.getIsDirty()).toBe(false);
    expect(store.getBaseline()).toEqual(canonical);
    expect(getFetchCalls().map((call) => call.method)).toEqual(["GET", "POST"]);
  });

  test("saveWorkingCopyAsSnapshot leaves the working copy dirty when the upload fails", async () => {
    const store = createRepositoryWorkingCopyStore(createEmptyRepository());
    store.applyContentChange(canonical);

    planEmptySnapshotList();
    planJsonResponse(
      { error: { code: "storage_full", message: "No space remains." } },
      507,
    );

    await expect(saveWorkingCopyAsSnapshot(store)).rejects.toMatchObject({
      status: 507,
    });
    expect(store.getIsDirty()).toBe(true);
    expect(store.getRepository()).toEqual(canonical);
  });

  test("commit uncertainty reconciles an exact new blob with GETs only and never duplicates POST", async () => {
    const store = createRepositoryWorkingCopyStore(createEmptyRepository());
    store.applyContentChange(canonical);
    let postedBytes = new Uint8Array();

    planJsonResponse({
      blobs: [{ id: "3", sizeBytes: 10 }],
      usedBytes: 10,
      remainingBytes: 1_000_000,
    });
    planFetch((call) => {
      expect(call.method).toBe("POST");
      expect(call.body).toBeInstanceOf(Uint8Array);
      postedBytes = new Uint8Array(call.body as Uint8Array);
      return jsonResponse(
        {
          error: {
            code: "commit_uncertain",
            message: "Blob activation succeeded but durability is uncertain.",
          },
        },
        503,
      );
    });
    planJsonResponse({
      blobs: [
        { id: "4", sizeBytes: 42 },
        { id: "3", sizeBytes: 10 },
      ],
      usedBytes: 52,
      remainingBytes: 999_958,
    });
    planFetch((call) => {
      expect(call.method).toBe("GET");
      expect(call.url).toBe("/api/v1/blob/4");
      return binaryResponse(postedBytes);
    });

    const first = await saveWorkingCopyAsSnapshot(store).catch(
      (error: unknown) => error,
    );
    expect(first).toBeInstanceOf(SnapshotCommitUncertainError);
    expect(first).toMatchObject({
      reconciliation: { state: "matched", blobId: "4" },
    });
    expect(store.getIsDirty()).toBe(true);
    expect(
      getFetchCalls().filter((call) => call.method === "POST"),
    ).toHaveLength(1);

    planJsonResponse({
      blobs: [
        { id: "4", sizeBytes: 42 },
        { id: "3", sizeBytes: 10 },
      ],
      usedBytes: 52,
      remainingBytes: 999_958,
    });
    planFetch((call) => {
      expect(call.method).toBe("GET");
      expect(call.url).toBe("/api/v1/blob/4");
      return binaryResponse(postedBytes);
    });

    const second = await saveWorkingCopyAsSnapshot(store).catch(
      (error: unknown) => error,
    );
    expect(second).toBeInstanceOf(SnapshotCommitUncertainError);
    expect(second).toMatchObject({
      reconciliation: { state: "matched", blobId: "4" },
    });
    expect(
      getFetchCalls().filter((call) => call.method === "POST"),
    ).toHaveLength(1);
    expect(store.getIsDirty()).toBe(true);
  });

  test("authoritative no-match requires a later explicit Save before another POST", async () => {
    const store = createRepositoryWorkingCopyStore(createEmptyRepository());
    store.applyContentChange(canonical);

    planEmptySnapshotList();
    planJsonResponse(
      {
        error: {
          code: "commit_uncertain",
          message: "Blob activation succeeded but durability is uncertain.",
        },
      },
      503,
    );
    planEmptySnapshotList();

    const uncertain = await saveWorkingCopyAsSnapshot(store).catch(
      (error: unknown) => error,
    );
    expect(uncertain).toBeInstanceOf(SnapshotCommitUncertainError);
    expect(uncertain).toMatchObject({ reconciliation: { state: "not_found" } });
    expect(
      getFetchCalls().filter((call) => call.method === "POST"),
    ).toHaveLength(1);
    expect(store.getIsDirty()).toBe(true);

    planEmptySnapshotList();
    planJsonResponse({ id: "5", sizeBytes: 42 }, 201);
    await expect(saveWorkingCopyAsSnapshot(store)).resolves.toEqual({
      id: "5",
      sizeBytes: 42,
    });
    expect(
      getFetchCalls().filter((call) => call.method === "POST"),
    ).toHaveLength(2);
    expect(store.getIsDirty()).toBe(false);
  });

  test("unavailable reconciliation blocks later POST attempts until GET recovery succeeds", async () => {
    const store = createRepositoryWorkingCopyStore(createEmptyRepository());
    store.applyContentChange(canonical);

    planEmptySnapshotList();
    planJsonResponse(
      {
        error: {
          code: "commit_uncertain",
          message: "Blob activation succeeded but durability is uncertain.",
        },
      },
      503,
    );
    planFetch(() => {
      throw new TypeError("device unavailable");
    });

    const first = await saveWorkingCopyAsSnapshot(store).catch(
      (error: unknown) => error,
    );
    expect(first).toBeInstanceOf(SnapshotCommitUncertainError);
    expect(first).toMatchObject({ reconciliation: { state: "unavailable" } });
    expect(
      getFetchCalls().filter((call) => call.method === "POST"),
    ).toHaveLength(1);

    planFetch(() => {
      throw new TypeError("device unavailable");
    });
    const second = await saveWorkingCopyAsSnapshot(store).catch(
      (error: unknown) => error,
    );
    expect(second).toBeInstanceOf(SnapshotCommitUncertainError);
    expect(second).toMatchObject({ reconciliation: { state: "unavailable" } });
    expect(
      getFetchCalls().filter((call) => call.method === "POST"),
    ).toHaveLength(1);
    expect(store.getIsDirty()).toBe(true);
  });

  function randomSource(length: number): string {
    // High-entropy filler: gzip cannot meaningfully compress this, unlike a
    // repeated character, so the compressed upload genuinely exceeds
    // v2Limits.blobMaxBytes. `{` and `}` are excluded — they are the macro
    // language's directive delimiters (§7), and this fixture must stay
    // grammar-valid now that `saveWorkingCopyAsSnapshot` validates the whole
    // repository before ever reaching the size check (V2-110).
    const bytes = new Uint8Array(length);
    crypto.getRandomValues(bytes);
    return Array.from(bytes, (byte) => {
      let codePoint = 33 + (byte % 92);
      if (codePoint >= 123) {
        codePoint += 1; // skip '{' (123)
      }
      if (codePoint >= 125) {
        codePoint += 1; // skip '}' (125)
      }
      return String.fromCharCode(codePoint);
    }).join("");
  }

  test("saveWorkingCopyAsSnapshot validates the entire repository before ever calling fetch, and leaves the working copy dirty on failure (V2-110)", async () => {
    const invalid: Repository = {
      ...canonical,
      packages: [
        { id: "not-a-uuid", name: "Bad package", macros: [] },
        ...canonical.packages,
      ],
    };
    const store = createRepositoryWorkingCopyStore(createEmptyRepository());
    store.applyContentChange(invalid);

    const rejection = expect(saveWorkingCopyAsSnapshot(store)).rejects;
    await rejection.toBeInstanceOf(SnapshotValidationError);
    await rejection.toMatchObject({
      issues: expect.arrayContaining([
        expect.objectContaining({ path: "$.packages[0].id" }) as unknown,
      ]) as unknown,
    });
    expect(getFetchCalls()).toHaveLength(0);
    expect(store.getIsDirty()).toBe(true);
    expect(store.getRepository()).toEqual(invalid);
  });

  test("saveWorkingCopyAsSnapshot never issues a DELETE — normal saves are additive only (V2-116)", async () => {
    const store = createRepositoryWorkingCopyStore(createEmptyRepository());
    store.applyContentChange(canonical);
    planEmptySnapshotList();
    planFetch((call) => {
      expect(call.method).toBe("POST");
      return jsonResponse({ id: "6", sizeBytes: 42 }, 201);
    });
    await saveWorkingCopyAsSnapshot(store);
    expect(getFetchCalls().every((call) => call.method !== "DELETE")).toBe(
      true,
    );
  });

  test("saveWorkingCopyAsSnapshot refuses an oversized snapshot before ever calling fetch", async () => {
    // This fixture is genuinely large (~1.6 MB of high-entropy filler, so
    // gzip cannot compress it away - see randomSource()'s comment), and
    // gzip-compressing that much incompressible data is real synchronous
    // CPU work. Under `vitest run --coverage`, V8's coverage instrumentation
    // slows that hot loop enough to exceed the default 5000ms timeout, even
    // though the same code is fast under a plain (non-coverage) run. Not a
    // logic bug - just needs more headroom for instrumented runs.
    const oversizedPackage = {
      id: "550e8400-e29b-41d4-a716-446655440099",
      name: "Oversized",
      macros: Array.from({ length: 400 }, (_, index) => ({
        id: `6ba7b810-9dad-41d1-80b4-00c04fd4${(3000 + index).toString(16).padStart(4, "0")}`,
        name: `Macro ${String(index)}`,
        source: randomSource(4096),
        keyPressMs: 8,
        interKeyMs: 15,
      })),
    };
    const oversized: Repository = {
      format: "esp32-macro-keyboard-repository",
      schemaVersion: 1,
      packages: [oversizedPackage],
    };
    const store = createRepositoryWorkingCopyStore(oversized);
    expect(serializeRepository(oversized).length).toBeGreaterThan(
      v2Limits.blobMaxBytes,
    );

    await expect(saveWorkingCopyAsSnapshot(store)).rejects.toBeInstanceOf(
      SnapshotTooLargeError,
    );
    expect(getFetchCalls()).toHaveLength(0);
    expect(store.getIsDirty()).toBe(false);
  }, 20000);

  test("loadSnapshotIntoWorkingCopy validates before replacing the working copy", async () => {
    const store = createRepositoryWorkingCopyStore(createEmptyRepository());
    const bytes = await canonicalGzipBytes();
    planFetch((call) => {
      expect(call.url).toBe("/api/v1/blob/3");
      return binaryResponse(bytes);
    });

    const result = await loadSnapshotIntoWorkingCopy("3", store);
    expect(result).toEqual({ ok: true, repository: canonical, created: false });
    expect(store.getRepository()).toEqual(canonical);
    expect(store.getIsDirty()).toBe(false);
    expect(store.getBaseline()).toEqual(canonical);
  });

  test("loadSnapshotIntoWorkingCopy leaves the working copy untouched when the blob is corrupt", async () => {
    const store = createRepositoryWorkingCopyStore(canonical);
    planFetch(
      () =>
        new Response(new Uint8Array([1, 2, 3]), {
          status: 200,
          headers: { "Content-Type": "application/gzip" },
        }),
    );

    const result = await loadSnapshotIntoWorkingCopy("9", store);
    expect(result).toEqual({
      ok: false,
      reason: "unreadable",
      message: "The stored snapshot could not be decompressed.",
    });
    expect(store.getRepository()).toBe(canonical);
    expect(store.getIsDirty()).toBe(false);
  });

  test("loadSnapshotIntoWorkingCopy leaves the working copy untouched when the schema is invalid", async () => {
    const store = createRepositoryWorkingCopyStore(canonical);
    const invalid = { format: "wrong", schemaVersion: 1, packages: [] };
    const bytes = await gzipCompress(
      new TextEncoder().encode(JSON.stringify(invalid)),
    );
    planFetch(() => binaryResponse(bytes));

    const result = await loadSnapshotIntoWorkingCopy("9", store);
    expect(result.ok).toBe(false);
    if (!result.ok) {
      expect(result.reason).toBe("invalid");
    }
    expect(store.getRepository()).toBe(canonical);
  });

  test("downloadSnapshotBytes returns the raw stored bytes without decompressing", async () => {
    const bytes = await canonicalGzipBytes();
    planFetch(() => binaryResponse(bytes));
    const downloaded = await downloadSnapshotBytes("1");
    expect(Array.from(downloaded)).toEqual(Array.from(bytes));
  });

  test("deleteSnapshot issues a DELETE and expects 204", async () => {
    planFetch((call) => {
      expect(call.method).toBe("DELETE");
      expect(call.url).toBe("/api/v1/blob/1");
      return new Response(null, { status: 204 });
    });
    await expect(deleteSnapshot("1")).resolves.toBeUndefined();
  });

  test("exportRepository produces the spec'd suffix and MIME type without any network call", async () => {
    const exported = await exportRepository(canonical);
    expect(exported.filename.endsWith(".emk-repository.json.gz")).toBe(true);
    expect(exported.mimeType).toBe("application/gzip");
    expect(getFetchCalls()).toHaveLength(0);
  });

  test("importRepository validates and reports counts without touching a store or uploading", async () => {
    const bytes = await canonicalGzipBytes();
    const result = await importRepository(bytes);
    expect(result).toEqual({
      ok: true,
      repository: canonical,
      packageCount: 1,
      macroCount: 1,
    });
    expect(getFetchCalls()).toHaveLength(0);
  });

  test("importRepository reports invalid schema without throwing", async () => {
    const invalid = { format: "wrong", schemaVersion: 1, packages: [] };
    const bytes = await gzipCompress(
      new TextEncoder().encode(JSON.stringify(invalid)),
    );
    const result = await importRepository(bytes);
    expect(result.ok).toBe(false);
  });

  describe("replaceSnapshotWithWorkingCopy — V2-116 advanced non-atomic replace", () => {
    test("deletes then adds, in that order, on success", async () => {
      const store = createRepositoryWorkingCopyStore(createEmptyRepository());
      store.applyContentChange(canonical);
      const order: string[] = [];
      planFetch((call) => {
        expect(call.method).toBe("DELETE");
        expect(call.url).toBe("/api/v1/blob/1");
        order.push("delete");
        return new Response(null, { status: 204 });
      });
      planFetch((call) => {
        expect(call.method).toBe("GET");
        expect(call.url).toBe("/api/v1/blob");
        order.push("list");
        return jsonResponse({
          blobs: [],
          usedBytes: 0,
          remainingBytes: 1_000_000,
        });
      });
      planFetch((call) => {
        expect(call.method).toBe("POST");
        order.push("add");
        return jsonResponse({ id: "9", sizeBytes: 42 }, 201);
      });

      const result = await replaceSnapshotWithWorkingCopy("1", store);
      expect(order).toEqual(["delete", "list", "add"]);
      expect(result).toEqual({
        ok: true,
        deletedId: "1",
        created: { id: "9", sizeBytes: 42 },
      });
      expect(store.getIsDirty()).toBe(false);
    });

    test("a delete failure never attempts the add, and changes nothing", async () => {
      const store = createRepositoryWorkingCopyStore(createEmptyRepository());
      store.applyContentChange(canonical);
      planJsonResponse(
        { error: { code: "not_found", message: "No such blob." } },
        404,
      );

      const result = await replaceSnapshotWithWorkingCopy("1", store);
      expect(result.ok).toBe(false);
      if (!result.ok) {
        expect(result.stage).toBe("delete");
        expect(result.deletedId).toBeNull();
      }
      expect(getFetchCalls()).toHaveLength(1);
      expect(store.getIsDirty()).toBe(true);
    });

    test("a delete success followed by an add failure reports the deleted ID, and the working copy stays dirty (not restored)", async () => {
      const store = createRepositoryWorkingCopyStore(createEmptyRepository());
      store.applyContentChange(canonical);
      planFetch(() => new Response(null, { status: 204 }));
      planEmptySnapshotList();
      planJsonResponse(
        { error: { code: "storage_full", message: "No space remains." } },
        507,
      );

      const result = await replaceSnapshotWithWorkingCopy("1", store);
      expect(result.ok).toBe(false);
      if (!result.ok) {
        expect(result.stage).toBe("add");
        expect(result.deletedId).toBe("1");
      }
      // The deleted blob is gone and this function never recreates it under
      // the old ID — the working copy remains dirty and unsaved, exactly as
      // an ordinary failed save would leave it (SPEC_V2 §10.6).
      expect(store.getIsDirty()).toBe(true);
      expect(store.getRepository()).toEqual(canonical);
    });
  });
});
