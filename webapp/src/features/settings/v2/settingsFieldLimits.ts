import { utf8ByteLength } from "../../../v2/repository";

// SPEC_V2 §13.4/§13.9 device-name examples are all well under this bound;
// `apiRequestGuards.ts`'s `isOptionalDeviceSettings`/`isSetupIdentity` are the
// actual enforced gate (1..32 UTF-8 bytes) — this constant is presentation
// only, for the live byte-count helper text.
export const deviceNameMaxBytes = 32;
export const ssidMaxBytes = 32;
export const passphraseMinBytes = 8;
export const passphraseMaxBytes = 63;

export function byteCountLabel(value: string, max: number): string {
  return `${String(utf8ByteLength(value))}/${String(max)} bytes`;
}
