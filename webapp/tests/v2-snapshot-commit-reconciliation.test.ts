import { describe, expect, test } from "vitest";
import canonicalRepository from "../../contracts/v2/repository/canonical.json";
import type { Repository } from "../src/v2/repository";
import { createEmptyRepository } from "../src/v2/repositoryValidation";
import { createRepositoryWorkingCopyStore } from "../src/v2/repositoryWorkingCopy";
import {
  SnapshotCommitUncertainError,
  saveWorkingCopyAsSnapshot,
} from "../src/v2/snapshotClient";
import {
  getFetchCalls,
  jsonResponse,
  planFetch,
  planJsonResponse,
} from "./fakeFetch";

const canonical = canonicalRepository as Repository;

function binaryResponse(bytes: Uint8Array): Response {
  return new Response(bytes, {
    status: 200,
    headers: { "Content-Type": "application/gzip" },
  });
}

function commitUncertainResponse(): Response {
  return jsonResponse(
    {
      error: {
        code: "commit_uncertain",
        message: "Blob activation succeeded but durability is uncertain.",
      },
    },
    503,
  );
}

describe("v2 snapshot commit reconciliation edges", () => {
  test("ambiguous exact new matches remain latched and block another POST", async () => {
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
      return commitUncertainResponse();
    });
    planJsonResponse({
      blobs: [
        { id: "5", sizeBytes: 42 },
        { id: "4", sizeBytes: 42 },
        { id: "3", sizeBytes: 10 },
      ],
      usedBytes: 94,
      remainingBytes: 999_916,
    });
    planFetch((call) => {
      expect(call.method).toBe("GET");
      expect(call.url).toBe("/api/v1/blob/5");
      return binaryResponse(postedBytes);
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
      reconciliation: {
        state: "ambiguous",
        matchingBlobIds: ["5", "4"],
      },
    });
    expect(store.getIsDirty()).toBe(true);
    expect(getFetchCalls().filter((call) => call.method === "POST")).toHaveLength(
      1,
    );

    planJsonResponse({
      blobs: [
        { id: "5", sizeBytes: 42 },
        { id: "4", sizeBytes: 42 },
        { id: "3", sizeBytes: 10 },
      ],
      usedBytes: 94,
      remainingBytes: 999_916,
    });
    planFetch((call) => {
      expect(call.url).toBe("/api/v1/blob/5");
      return binaryResponse(postedBytes);
    });
    planFetch((call) => {
      expect(call.url).toBe("/api/v1/blob/4");
      return binaryResponse(postedBytes);
    });

    const second = await saveWorkingCopyAsSnapshot(store).catch(
      (error: unknown) => error,
    );
    expect(second).toBeInstanceOf(SnapshotCommitUncertainError);
    expect(second).toMatchObject({
      reconciliation: {
        state: "ambiguous",
        matchingBlobIds: ["5", "4"],
      },
    });
    expect(getFetchCalls().filter((call) => call.method === "POST")).toHaveLength(
      1,
    );
    expect(store.getIsDirty()).toBe(true);
  });

  test("reconciliation ignores a pre-existing identical blob and inspects only new IDs", async () => {
    const store = createRepositoryWorkingCopyStore(createEmptyRepository());
    store.applyContentChange(canonical);
    let postedBytes = new Uint8Array();

    // Blob 3 represents an already-stored byte-identical snapshot. Its ID is
    // captured before the POST, so reconciliation must never download or use it
    // as evidence that the uncertain create succeeded.
    planJsonResponse({
      blobs: [{ id: "3", sizeBytes: 42 }],
      usedBytes: 42,
      remainingBytes: 1_000_000,
    });
    planFetch((call) => {
      expect(call.method).toBe("POST");
      postedBytes = new Uint8Array(call.body as Uint8Array);
      expect(postedBytes.byteLength).toBeGreaterThan(0);
      return commitUncertainResponse();
    });
    planJsonResponse({
      blobs: [
        { id: "4", sizeBytes: 3 },
        { id: "3", sizeBytes: postedBytes.byteLength },
      ],
      usedBytes: postedBytes.byteLength + 3,
      remainingBytes: 999_997,
    });
    planFetch((call) => {
      expect(call.method).toBe("GET");
      expect(call.url).toBe("/api/v1/blob/4");
      return binaryResponse(new Uint8Array([1, 2, 3]));
    });

    const result = await saveWorkingCopyAsSnapshot(store).catch(
      (error: unknown) => error,
    );
    expect(result).toBeInstanceOf(SnapshotCommitUncertainError);
    expect(result).toMatchObject({ reconciliation: { state: "not_found" } });
    expect(store.getIsDirty()).toBe(true);
    expect(getFetchCalls().some((call) => call.url === "/api/v1/blob/3")).toBe(
      false,
    );
    expect(getFetchCalls().filter((call) => call.method === "POST")).toHaveLength(
      1,
    );
  });
});
