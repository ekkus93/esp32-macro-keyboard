# Static Analysis Exceptions

This register lists the **only** approved repository-wide clang-tidy check
exceptions, authorized by FIX1 RESPONSES Q2. They are explicit, reviewed
exceptions — not hidden suppressions. Each fires exclusively on a third-party
signature or a C-library limitation we implement against but do not control, and
none has a per-finding exemption on the pinned toolchain.

`scripts/check-static-analysis-policy.sh` enforces this register: it fails if a
fourth check is disabled, an approved name is misspelled or broadened to a
wildcard, `WarningsAsErrors` is weakened, or a first-party `NOLINT` / compiler
diagnostic pragma / `-Wno-*` suppression appears.

**Pinned toolchain:** Espressif LLVM (esp-clang) `19.1.2`, ESP-IDF `v5.5.5`,
target `esp32s3`; host `clang-format`/`clang-tidy` `18` (apt, Ubuntu 24.04).

**Not authorized:** any additional disabled check, per-finding `NOLINT`,
`#pragma GCC diagnostic ignored`, `-Wno-*`, `-Wno-error=*`, `eslint-disable`,
`@ts-ignore`, `@ts-nocheck`, coverage-exclusion markers, or first-party
formatting suppression.

---

## 1. `clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling`

- **Representative finding:** every `memset`/`memcpy` is flagged with a demand to
  use the C11 Annex K `*_s` functions (e.g. `memset_s`, `memcpy_s`).
- **Why remediation is invalid/unavailable:** newlib / ESP-IDF do not provide the
  Annex K bounds-checked functions, so the demanded fix cannot be written or
  linked on this toolchain.
- **Affected source/interface:** all first-party C that copies or zeroes buffers.
- **Compensating controls:**
  - strict size and bounds validation before every copy;
  - checked `snprintf`/copy lengths;
  - `-Werror` on all first-party warnings;
  - ASan/UBSan host tests;
  - deterministic short-read, short-write, overflow, and malformed-input tests;
  - no unbounded `strcpy`, `strcat`, or `sprintf` (enforced by the policy scan).
- **Re-evaluation condition:** the target libc gains Annex K `*_s` functions, or
  ESP-IDF ships a supported bounded-copy API.
- **Decision:** FIX1 RESPONSES Q2 (2025-07-26).

## 2. `readability-non-const-parameter`

- **Representative finding:** the `buffer` parameter of the TinyUSB
  `tud_hid_get_report_cb` callback "can be pointer to const".
- **Why remediation is invalid/unavailable:** the TinyUSB HID prototype mandates a
  non-const `uint8_t *`; qualifying it `const` is a conflicting-types compile error
  (the prototype is in scope via `tusb.h`). The check has no per-parameter option.
- **Affected source/interface:** `firmware/components/usb_keyboard/usb_descriptors.c`
  TinyUSB HID callbacks.
- **Compensating controls:**
  - the mandated callback adapters are minimal;
  - callback input is treated as read-only unless the external contract requires
    writing;
  - data is copied into internal const-qualified representations before flowing
    deeper into first-party code where appropriate;
  - normal firmware build coverage exercises the callback signature;
  - the check is re-enabled temporarily during toolchain upgrades to confirm no
    unrelated first-party findings appeared.
- **Re-evaluation condition:** TinyUSB changes the callback signature, or the check
  gains a per-parameter exemption.
- **Decision:** FIX1 RESPONSES Q2 (2025-07-26).

## 3. `concurrency-mt-unsafe`

- **Representative finding:** `readdir()` in the POSIX storage backend
  (`firmware/components/storage/storage_fs_ops.c`) is "not thread safe".
- **Why remediation is invalid/unavailable:** the only mt-safe replacement
  `readdir_r()` is deprecated on the glibc host build (`tests/host`) and undeclared
  without a feature macro, which breaks the `-Werror` host build; there is no
  non-deprecated portable alternative.
- **Affected source/interface:** POSIX directory iteration inside the storage
  ownership boundary.
- **Compensating controls:**
  - all repository mutation and recovery operations are serialized (FIX1 §7.5);
  - one `DIR *` is never shared between concurrent operations;
  - directory iteration stays inside the repository/storage ownership boundary;
  - lock ownership is tested at every directory-iteration seam;
  - ThreadSanitizer / real-thread smoke coverage will be added when the host
    toolchain supports it reliably, without replacing the deterministic lock tests.
- **Re-evaluation condition:** a non-deprecated portable mt-safe directory read
  becomes available, or the storage layer stops iterating directories directly.
- **Decision:** FIX1 RESPONSES Q2 (2025-07-26).
