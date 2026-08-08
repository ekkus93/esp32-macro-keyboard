import { GzipUnsupportedError, gzipCompress, gzipDecompress } from "./gzip";
import { serializeRepository, type Repository } from "./repository";

const textEncoder = new TextEncoder();

/**
 * Raised when stored blob bytes cannot be turned back into a JSON value:
 * corrupt gzip data, non-UTF-8 content, or malformed JSON. Distinct from
 * {@link GzipUnsupportedError} (a browser-compatibility error), this is the
 * "selected blob is unreadable" case from SPEC_V2 §9: the caller should let
 * the user choose another stored snapshot rather than treat it as fatal.
 */
export class RepositoryDecodeError extends Error {
  public constructor(message: string, options?: { cause?: unknown }) {
    super(message, options);
    this.name = "RepositoryDecodeError";
  }
}

/** Serializes and gzip-compresses a repository for upload or export. */
export async function encodeRepositorySnapshot(
  repository: Repository,
): Promise<Uint8Array> {
  const json = serializeRepository(repository);
  return gzipCompress(textEncoder.encode(json));
}

/**
 * Gzip-decompresses and JSON-parses stored snapshot bytes. Returns an
 * `unknown` value: callers MUST run it through
 * {@link import("./repositoryValidation").validateRepositoryForUse} before
 * treating it as a `Repository` or replacing any working copy.
 *
 * Throws {@link GzipUnsupportedError} when the browser lacks the required
 * compression APIs, or {@link RepositoryDecodeError} when the bytes are not a
 * readable gzip/UTF-8/JSON payload.
 */
export async function decodeRepositorySnapshot(
  bytes: Uint8Array,
): Promise<unknown> {
  let decompressed: Uint8Array;
  try {
    decompressed = await gzipDecompress(bytes);
  } catch (error: unknown) {
    if (error instanceof GzipUnsupportedError) {
      throw error;
    }
    throw new RepositoryDecodeError(
      "The stored snapshot could not be decompressed.",
      { cause: error },
    );
  }

  let text: string;
  try {
    text = new TextDecoder("utf-8", { fatal: true }).decode(decompressed);
  } catch (error: unknown) {
    throw new RepositoryDecodeError("The stored snapshot is not valid UTF-8.", {
      cause: error,
    });
  }

  try {
    return JSON.parse(text) as unknown;
  } catch (error: unknown) {
    throw new RepositoryDecodeError("The stored snapshot is not valid JSON.", {
      cause: error,
    });
  }
}
