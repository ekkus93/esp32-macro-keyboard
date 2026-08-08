import { describe, expect, test } from "vitest";
import canonicalRepository from "../../contracts/v2/repository/canonical.json";
import {
  GzipDecodeError,
  GzipUnsupportedError,
  gzipCompress,
  gzipDecompress,
  isGzipSupported,
} from "../src/v2/gzip";
import {
  RepositoryDecodeError,
  decodeRepositorySnapshot,
  encodeRepositorySnapshot,
} from "../src/v2/repositoryCodec";
import { serializeRepository, type Repository } from "../src/v2/repository";

describe("v2 gzip compression", () => {
  test("feature-detects CompressionStream/DecompressionStream", () => {
    expect(isGzipSupported()).toBe(true);
  });

  test("round-trips arbitrary bytes through gzip", async () => {
    const original = new TextEncoder().encode("hello, macro keyboard");
    const compressed = await gzipCompress(original);
    expect(Array.from(compressed)).not.toEqual(Array.from(original));
    const decompressed = await gzipDecompress(compressed);
    // Compared as plain arrays: the test runner's fetch/stream globals and
    // jsdom's typed-array globals can live in different realms, giving
    // byte-identical Uint8Arrays with different constructors — toEqual's
    // prototype check would otherwise fail on that harness artifact alone.
    expect(Array.from(decompressed)).toEqual(Array.from(original));
  });

  test("gzipCompress throws GzipUnsupportedError when unsupported", async () => {
    const original = globalThis.CompressionStream;
    // @ts-expect-error simulating an unsupported browser
    delete globalThis.CompressionStream;
    try {
      await expect(gzipCompress(new Uint8Array([1]))).rejects.toBeInstanceOf(
        GzipUnsupportedError,
      );
    } finally {
      globalThis.CompressionStream = original;
    }
  });

  test("gzipDecompress throws GzipDecodeError on corrupt input", async () => {
    await expect(
      gzipDecompress(new Uint8Array([1, 2, 3, 4, 5])),
    ).rejects.toBeInstanceOf(GzipDecodeError);
  });
});

describe("v2 repository codec", () => {
  test("round-trips the canonical repository through gzip", async () => {
    const repository = canonicalRepository as Repository;
    const bytes = await encodeRepositorySnapshot(repository);
    const decoded = await decodeRepositorySnapshot(bytes);
    expect(decoded).toEqual(JSON.parse(serializeRepository(repository)));
  });

  test("decodeRepositorySnapshot throws RepositoryDecodeError on corrupt gzip", async () => {
    await expect(
      decodeRepositorySnapshot(new Uint8Array([1, 2, 3])),
    ).rejects.toBeInstanceOf(RepositoryDecodeError);
  });

  test("decodeRepositorySnapshot throws RepositoryDecodeError on non-JSON content", async () => {
    const bytes = await gzipCompress(new TextEncoder().encode("not json"));
    await expect(decodeRepositorySnapshot(bytes)).rejects.toBeInstanceOf(
      RepositoryDecodeError,
    );
  });

  test("decodeRepositorySnapshot surfaces GzipUnsupportedError distinctly", async () => {
    const original = globalThis.DecompressionStream;
    // @ts-expect-error simulating an unsupported browser
    delete globalThis.DecompressionStream;
    try {
      await expect(
        decodeRepositorySnapshot(new Uint8Array([1, 2, 3])),
      ).rejects.toBeInstanceOf(GzipUnsupportedError);
    } finally {
      globalThis.DecompressionStream = original;
    }
  });
});
