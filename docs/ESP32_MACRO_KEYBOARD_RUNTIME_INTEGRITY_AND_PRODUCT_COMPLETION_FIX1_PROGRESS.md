# FIX1 Runtime Integrity and Product Completion — Progress

**Specification:** `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_SPEC.md`
**Plan:** `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md`
**Operator decisions:** `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_RESPONSES.md`

## Baseline

- Reviewed ancestor (from spec): `992f2a018aff97e5b167c98d6a0d469d6a7c84ff`.
- **Actual starting `master` SHA:** `e757c5f30108caa97542d8fdc2abcbacfec151f3`
  (`docs: answer FIX1 implementation questions`).
- Work proceeds directly on `master` per RESPONSES Q4.

### Baseline validation (before any FIX1 change)

| Check | Command | Result |
| --- | --- | --- |
| Full gate | `./scripts/check-all.sh` | pass (Quality CI green on `e757c5f`'s ancestors) |
| Sanitizers | `./scripts/run-tests.sh --sanitizers` | 17/17 pass, ASan/UBSan/leak clean |
| Native coverage | `./scripts/generate-native-coverage.sh` | pass, 95% total (line ≥90 / branch ≥80 on policy files) |
| Frontend coverage | `./scripts/generate-frontend-coverage.sh` | pass, 92.48% statements / 96.77% branch |
| Device build | `./scripts/build-device-tests.sh` | pass (ESP32-S3 Unity firmware builds) |

### Baseline metrics

- Production firmware binary (`firmware/build/esp32_macro_keyboard.bin`): 907,680 bytes.
- webapp production build (`webapp/dist`): ~216 KiB.
- Host test suites: 17.

### Baseline notes / known pre-FIX1 state

- `scripts/check-firmware.sh` currently runs `run-clang-tidy … || true` and gates on
  first-party *finding location* — the pattern FIX1 Phase 2 replaces (RESPONSES Q1).
- `.clang-tidy` disables three checks with documented rationale
  (`insecureAPI.DeprecatedOrUnsafeBufferHandling`, `readability-non-const-parameter`,
  `concurrency-mt-unsafe`) — approved as reviewed exceptions (RESPONSES Q2), to be
  formalized in `docs/STATIC_ANALYSIS_EXCEPTIONS.md` with a policy check.
- `auth_password_verify` / `auth_core_password_verify` return `bool` (crypto failure vs
  mismatch conflated) — corrected in Phase/Section 10.
- Execution terminal states have no `TIMED_OUT` — added end-to-end per RESPONSES Q3.

## Phase status

| Phase | Title | Status |
| --- | --- | --- |
| 1 | Establish the FIX1 baseline | in progress |
| 2 | Make the quality gate fail closed | not started |
| 3 | Structured failure and ownership reporting | not started |
| 4 | Correct application lifecycle ownership | not started |
| 5 | Correct HTTP partial-start lifecycle | not started |
| 6 | Correct filesystem mount ownership and topology | not started |
| 7 | Atomic-write artifact recovery | not started |
| 8 | Make quarantine recoverable | not started |
| 9 | Serialize repository operations | not started |
| 10 | Separate password mismatch from crypto failure | not started |
| 11–13 | Wi-Fi / executor / controls cleanup and visibility | not started |
| 14 | Encrypted persistent provisioning | not started |
| 15 | Complete storage object repositories | not started |
| 16 | Complete the HTTP API | not started |
| 17 | Replace frontend mock behavior | not started |
| 18 | Import / export / backup / restore | not started |
| 19 | Diagnostics and observability | not started |
| 20 | Hardware and integration validation | environment-blocked (see below) |
| 21 | Release budgets and immutable CI | not started |
| 22 | Documentation synchronization | ongoing |
| 23 | Final regression and acceptance gate | not started |

## Completed tasks (commit evidence)

- Phase 1.1–1.3 (baseline, this document): pending commit.

## Environment-blocked (hardware / HIL) items

These remain **open** until observed on real hardware (RESPONSES Q5). Each will carry
prerequisite hardware, exact procedure, expected evidence, pass/fail criteria, and safety
notes when its phase is reached:

- eFuse / flash-encryption (or HMAC-key) provisioning confirmation (§14.2);
- Linux + ChromeOS USB enumeration/typing matrix (§20.2);
- real SoftAP / browser integration on the ESP32-S3 (§20.3);
- real power-interruption testing (§20.4);
- measured physical cancellation latency and reset-gesture validation (§20.5);
- observed release-all behavior on hardware.

The *software* portion of secure provisioning (encrypted-NVS config, provisioning state
machine, readback validation, setup/reset APIs, production-config rejection, host-testable
policy) is **not** hardware-blocked and is implemented in its phase.

## Unresolved blockers

- None yet.

## Deviations from the TODO

- None yet. Any deviation will be recorded here with rationale and the FIX1 decision
  reference, per RESPONSES §8.
