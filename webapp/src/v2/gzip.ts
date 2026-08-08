/**
 * Browser-native gzip compression, per SPEC_V2 §9 "Compression": React is the
 * only component that compresses or decompresses repository data, using
 * `CompressionStream("gzip")` / `DecompressionStream("gzip")`. An unsupported
 * browser must receive an explicit compatibility error; there is no
 * uncompressed fallback.
 */

export class GzipUnsupportedError extends Error {
  public constructor() {
    super(
      "This browser does not support the CompressionStream and DecompressionStream APIs required to read or save a repository.",
    );
    this.name = "GzipUnsupportedError";
  }
}

export class GzipDecodeError extends Error {
  public constructor(message: string, options?: { cause?: unknown }) {
    super(message, options);
    this.name = "GzipDecodeError";
  }
}

export function isGzipSupported(): boolean {
  return (
    typeof CompressionStream === "function" &&
    typeof DecompressionStream === "function"
  );
}

function requireGzipSupport(): void {
  if (!isGzipSupported()) {
    throw new GzipUnsupportedError();
  }
}

function toReadableStream(bytes: Uint8Array): ReadableStream<Uint8Array> {
  const copy = new Uint8Array(bytes);
  return new ReadableStream<Uint8Array>({
    start(controller) {
      controller.enqueue(copy);
      controller.close();
    },
  });
}

async function pipeThroughStream(
  bytes: Uint8Array,
  transform: ReadableWritablePair<Uint8Array, Uint8Array>,
): Promise<Uint8Array> {
  const stream = toReadableStream(bytes).pipeThrough(transform);
  const buffer = await new Response(stream).arrayBuffer();
  return new Uint8Array(buffer);
}

export async function gzipCompress(bytes: Uint8Array): Promise<Uint8Array> {
  requireGzipSupport();
  return pipeThroughStream(bytes, new CompressionStream("gzip"));
}

export async function gzipDecompress(bytes: Uint8Array): Promise<Uint8Array> {
  requireGzipSupport();
  try {
    return await pipeThroughStream(bytes, new DecompressionStream("gzip"));
  } catch (error: unknown) {
    throw new GzipDecodeError(
      "The compressed data could not be decompressed.",
      {
        cause: error,
      },
    );
  }
}
