# Claude Code handoff — finish V2 with a physical ESP32-S3

**Date:** 2026-08-08  
**Repository:** `ekkus93/esp32-macro-keyboard`  
**Validated source/evidence baseline before this handoff document:**
`34fb4bf4f3dfed94205b6294203dac3c05aabc3f`  
**Working branch:** `master`  
**Primary target hardware:** reference `ESP32-S3R8`  
**Purpose:** continue the V2 rebuild from the current exact state, finish the
remaining implementation in TODO order, and use the attached physical board for
the hardware evidence that hosted CI cannot provide.

This document is a handoff/status document. It is **not** a specification and it
must not be treated as authority when it conflicts with the V2 specification or
V2 TODO.

---

## 1. Read these first and use this authority order

Before changing code, read:

1. `CLAUDE.md`
2. `docs/SPEC_V2.md`
3. `docs/UI_UX_SPEC_V2.md`
4. `docs/TODO_V2.md`
5. `docs/implementation-v2/V2_MIGRATION_MAP.md`
6. the relevant implementation reports under `docs/implementation-v2/`
7. this handoff document

The synchronized V2 specs are the product authority. `docs/TODO_V2.md` is the
authoritative implementation sequence and evidence checklist.

Do **not** use these as V2 authority:

- `docs/SPEC.md` — retired V1 specification stub and frozen;
- `docs/TODO.md` — retired V1 TODO stub;
- old FIX/FIX1 documents;
- old screenshots or old V1 behavior;
- `docs/IMPLEMENTATION_STATUS.md` as a current V2 status source. It was last
  updated on 2026-08-01 and explicitly describes the older FIX1/V1-era state.

Do not modify `docs/SPEC_V2.md` or `docs/UI_UX_SPEC_V2.md` merely to make the
current code easier to satisfy. If code and spec genuinely disagree, fix the
code when possible and report the conflict rather than silently changing the
requirement.

---

## 2. Hard operating rules

These rules are deliberate. Do not weaken them to make progress look green.

- Work directly on `master`; do not create a branch or PR unless the owner asks.
- Never force-push, reset `master`, or rewrite history. Use forward commits.
- Do not mark a TODO item complete without implementation plus reproducible
  evidence.
- A successful compile is not physical hardware evidence.
- Host fakes are not physical hardware evidence.
- An ESP32-S3 device-test **build** is not execution of the Unity tests.
- Do not introduce `|| true`, ignored exit codes, warning suppression, analyzer
  exclusions, hidden stderr, or silent fallback behavior.
- Do not add compatibility aliases or fallback reads/writes for the retired V1
  firmware-owned package/macro architecture.
- Do not reintroduce old package/macro CRUD routes.
- Do not add V1-to-V2 migration, dual read, dual write, schema translation, or
  automatic recovery that is not in the V2 specs.
- Do not format the userdata filesystem on mount failure.
- Do not weaken the npm audit policy. The current policy is strict-zero.
- Do not add a Clang-Tidy suppression for a first-party problem. Fix the code.
- Do not commit real passwords, setup codes, session tokens, Wi-Fi credentials,
  raw flash dumps, raw userdata images, repository contents, or macro source.
- Use disposable test credentials and keep secrets outside the repository.
- Keep production web assets local; no remote runtime assets.

The project has repeatedly benefited from fail-closed gates finding real defects.
Treat a new gate failure as a defect to understand, not a nuisance to suppress.

---

## 3. Exact handoff baseline and CI state

The last code/evidence SHA validated before creation of this handoff was:

```text
34fb4bf4f3dfed94205b6294203dac3c05aabc3f
```

That exact revision passed all four permanent Cutover A gates:

| Permanent workflow | Run | Result |
| --- | ---: | --- |
| Browser Tests | `31249402350` | success |
| Host Tests | `31249402323` | success; all five jobs |
| Device Test Build | `31249402329` | success; ESP32-S3 device-test firmware built |
| Quality | `31249402346` | success; strict-zero npm audit and full authoritative checks |

The five Host jobs on that SHA were:

- Frontend Tests;
- Host Tests;
- Host ASan and UBSan;
- Frontend Coverage;
- Native Coverage.

The Quality workflow passed the complete `./scripts/check-all.sh` authoritative
path. Do not replace this evidence with an older SHA.

Creation of this handoff file is documentation-only and necessarily advances
`master`. At the start of work, run:

```bash
git status --short
git rev-parse HEAD
git log -1 --oneline
```

Confirm the tree is clean. Confirm the current HEAD is a descendant of the
validated baseline above. If there are additional commits after this handoff,
inspect them before doing any hardware work.

---

## 4. Toolchain and local environment

The repository enforces exact versions. Follow `CLAUDE.md` rather than using
whatever happens to be installed globally.

Required core versions:

- ESP-IDF: `v5.5.5`
- target: `esp32s3`
- Node.js: `v24.18.0`

Before firmware or device work:

```bash
. "$HOME/esp/esp-idf-v5.5.5/export.sh"
./scripts/verify-toolchain.sh
```

Important repository entry points:

```bash
./scripts/check-all.sh
./scripts/run-tests.sh
./scripts/run-tests.sh --sanitizers
./scripts/generate-native-coverage.sh
./scripts/check-webapp.sh
./scripts/check-firmware.sh
./scripts/check-format.sh
bash ./scripts/build-device-tests.sh
```

Use the repository scripts rather than inventing parallel build/test commands.
Narrow loops are fine while iterating, but run the authoritative gate at phase
boundaries and before treating a commit as settled.

---

## 5. Current V2 architecture and completed foundation

### 5.1 Retired firmware-owned repository architecture is gone

Phase 2 is implemented. The firmware no longer owns package/macro repository
state. Do not bring any of it back.

The V2 ownership model is:

- React/TypeScript owns repository/package/macro data and validation;
- firmware stores opaque gzip snapshot blobs;
- firmware does not parse repository JSON or inspect gzip contents;
- firmware provides the bounded API, settings/authentication, send/executor,
  USB HID, storage, network, diagnostics, and device lifecycle behavior.

The old package and macro CRUD resources are intentionally absent and have
negative tests to prevent reintroduction.

### 5.2 Opaque blob storage software path is implemented

Phase 3 software implementation is substantially complete:

- userdata LittleFS does not format on mount failure;
- final blob paths are `/data/repository/<fixed-width-id>.gz`;
- uploads use a temporary file and rename commit point;
- blob upload is bounded at 131,072 bytes;
- list/load/delete semantics are byte-oriented;
- firmware never decompresses or parses the repository blob;
- storage exhaustion maps to HTTP 507;
- interrupted temporary files are cleaned on boot;
- capacity/image proof exists in host/image tests.

However, V2-035 is still physically open. Hosted CI cannot prove real power-loss,
real flash persistence, real ENOSPC behavior, or mount-failure non-formatting on
actual flash.

A dedicated fail-closed physical collector already exists:

- `scripts/run-v2-035-hardware.py`
- `tests/scripts/test-v2-035-hardware.py`
- `docs/implementation-v2/V2_035_HARDWARE_HARNESS_PREPARATION_2026-08-06.md`
- `docs/implementation-v2/V2_035_HARDWARE_EVIDENCE_HARNESS_2026-08-06.md`

Do not rewrite this harness casually. It already contains provenance binding,
blob ownership tracking, recovery behavior, byte-identity checks, exact status
checks, and protection against deleting user-owned blobs.

### 5.3 V2 session and rate-limit semantics are implemented

V2-042 is checked complete in software. See:

`docs/implementation-v2/V2_042_SESSIONS_AND_RATE_LIMITING_2026-08-07.md`

Important properties already implemented and tested:

- session token contains 32 random bytes of entropy;
- at most eight sessions in bounded RAM;
- 24-hour idle expiry;
- seven-day absolute expiry;
- deterministic LRU replacement for a ninth session;
- failed session replacement rolls back transactionally;
- reboot/reinitialization invalidates RAM-only sessions;
- per-source rolling login throttling;
- five failures in 60 seconds causes a 300-second lockout;
- fixed-size per-source throttle state;
- login fails closed if the peer IPv4 address cannot be resolved;
- login cookie uses `HttpOnly; SameSite=Strict; Path=/`;
- logout clears the server session and cookie.

Do not replace this with a global throttle fallback or unbounded map.

---

## 6. Current Phase 4 state — this is the immediate implementation boundary

Phase 4 is **not** complete.

The important current status is:

- V2-040 — partially implemented; Cutover A is complete, Cutover B remains;
- V2-041 — open; real ESP32-S3R8 PBKDF2 benchmark and final iteration freeze
  remain;
- V2-042 — software complete;
- V2-043 — open;
- V2-044 — open;
- Phase 4 hardware exit evidence — open.

### 6.1 V2-040 host contract preparation already exists

Read:

`docs/implementation-v2/V2_040_HOST_PREPARATION_2026-08-07.md`

The prepared C contract is:

- `firmware/components/app_contracts_v2/include/setup_contract_v2.h`
- `firmware/components/app_contracts_v2/setup_contract_v2.c`

Do not duplicate this logic in the HTTP handler.

Prepared APIs include:

```c
app_v2_setup_session_generate(...)
app_v2_setup_session_init(...)
app_v2_setup_session_consume(...)
app_v2_setup_state_from_settings(...)
app_v2_setup_prepare_candidate(...)
app_v2_setup_accepted_response_init(...)
```

The prepared contract already covers:

- unbiased generation of an eight-digit decimal setup code;
- fail-closed entropy handling;
- one-time session/code consumption;
- constant-time comparison of well-formed codes;
- malformed/mismatched/consumed/already-provisioned rejection;
- exact minimal setup-state response;
- strict UTF-8 and byte-bound field validation;
- password-material validation;
- candidate settings preparation while preserving unrelated settings;
- separation of candidate preparation from setup-code consumption;
- exact accepted/restart/reconnect response semantics.

Use this contract as the live handler's business logic boundary.

### 6.2 V2-040 Cutover A is complete

Read:

`docs/implementation-v2/V2_040_CUTOVER_A_V2_BOOT_AUTHORITY_2026-08-07.md`

Cutover A established these production properties:

- canonical `device_settings` is the only boot settings authority;
- legacy provisioning state is not read at startup;
- normal-mode AP/station/password state comes from the validated V2 settings;
- an unprovisioned boot generates a fresh eight-digit setup code using ESP32
  randomness;
- the setup code is intentionally emitted only on the trusted serial surface;
- setup mode registers only `GET /api/v1/setup`, `POST /api/v1/setup`, and
  required static assets;
- `GET /api/v1/setup` returns only `provisioned:false` and `deviceName`;
- legacy normal-mode mutation paths are not allowed to write state that V2
  startup ignores.

Do not add a V1 compatibility path around this boundary.

### 6.3 The intentional current stub: `POST /api/v1/setup`

This is the most important immediate fact for continuation.

File:

`firmware/components/web_server/web_server_setup.c`

Current behavior:

- if the server is not in setup mode, POST returns `409 Conflict`;
- if the server is in setup mode, POST deliberately returns:

```text
503 Service Unavailable
V2 setup submission is not yet enabled
```

That `503` is not an accidental regression. It is the explicit fail-closed
Cutover A boundary. **Cutover B must replace it with the real transactional V2
setup submission path.**

The setup route table itself is already isolated in
`firmware/components/web_server/web_server_lifecycle.c`.

### 6.4 Device settings persistence API already exists

Use the existing device-settings API instead of creating a second persistence
mechanism:

`firmware/components/device_settings/include/device_settings.h`

Relevant functions:

```c
app_error_code_t device_settings_read(app_v2_device_settings_t *out_settings);
app_error_code_t device_settings_replace(
    const app_v2_device_settings_t *settings,
    bool *out_changed);
```

Cutover B should be based on candidate preparation followed by one canonical
`device_settings_replace()` commit. Do not dual-write legacy provisioning state.

### 6.5 Cutover B implementation requirements

Implement V2-040 fully before claiming first-run provisioning complete.
At minimum, the live path must:

1. accept only the exact V2 setup request schema;
2. enforce bounded body size before unbounded allocation/parsing;
3. reject unknown JSON fields;
4. validate the setup code using the prepared setup-session contract;
5. strictly validate device name, AP SSID, AP passphrase, administrator
   password, and physical-confirmation setting;
6. derive valid password material through the V2 password-verifier path;
7. read the current canonical settings;
8. prepare a candidate using `app_v2_setup_prepare_candidate()`;
9. atomically replace canonical settings with `device_settings_replace()`;
10. consume the one-time setup code **only after** the settings commit succeeds;
11. return the exact non-secret accepted/restart/reconnect response;
12. never return the administrator password, AP passphrase, verifier, salt,
    setup code, or session secret;
13. after provisioning, make setup-state GET return `404` and setup submission
    return the specified conflict behavior;
14. test wrong, malformed, expired/reboot-stale, reused, and mismatched codes;
15. test settings-commit failure and prove the setup code is not consumed on
    failure;
16. test preservation of unrelated settings;
17. test reconnect/AP credential transition semantics;
18. keep all non-setup API routes unavailable while unprovisioned.

Do not make setup "work" by writing a legacy record and rebooting into old
behavior. The V2 record is the only authority.

---

## 7. V2-041 — use the physical board to finish the PBKDF2 decision

V2-041 is deliberately hardware-dependent. The V2 TODO requires:

- PBKDF2-HMAC-SHA-256;
- random per-password salt;
- stored verifier version, salt, and iteration count;
- constant-time verifier comparison;
- benchmark candidate counts on the reference ESP32-S3R8;
- choose one exact count that produces approximately 250–500 ms;
- freeze that exact count in code and tests;
- confirm no watchdog starvation/problem;
- ensure passwords and derived material never enter logs or diagnostics.

The on-device test application already includes the benchmark. Read:

`firmware/test_app/README.md`

It provides `[device][auth][benchmark]` / `[benchmark]` tests and emits lines
beginning with:

```text
PBKDF2_BENCH
```

Those lines contain candidate iteration count, sample count, median, p90, and
worst-case microseconds.

Recommended procedure:

```bash
. "$HOME/esp/esp-idf-v5.5.5/export.sh"
bash ./scripts/build-device-tests.sh
cd firmware/test_app
idf.py -B build -p "$FLASH_PORT" flash monitor
```

Press Enter for the Unity menu and run `[benchmark]`.

Record in the implementation report:

- exact Git SHA;
- board model/variant;
- ESP-IDF version;
- serial/flash port;
- host OS;
- build/flash/monitor commands;
- every raw `PBKDF2_BENCH` line needed to justify the choice;
- selected iteration count;
- median/p90/worst timing;
- watchdog or starvation observation.

Do not choose a convenient round number without measurement. Do not use a
workstation benchmark as a substitute for the reference board.

Once the count is selected, freeze it in the canonical V2 implementation and
regression tests, run the software gates, then use it in the real setup/password
paths.

---

## 8. The two USB connectors are not interchangeable

Identify the actual devices every time. Do not trust `/dev/ttyACM0` numbering
from a previous boot or plug order.

Run:

```bash
lsusb | grep -E '303a|1a86|10c4'
ls -l /dev/ttyACM* /dev/ttyUSB* 2>/dev/null || true
```

Use the identities, not the guessed device number.

| Physical connector | Typical identity | Intended use |
| --- | --- | --- |
| ESP32-S3 native USB | `303a:4001` app/TinyUSB HID; `303a:1001` USB-Serial/JTAG otherwise | flashing, esptool, HID validation, boot/log output |
| UART bridge | `1a86:55d3` CH340 or `10c4:ea60` CP210x | interactive UART0 console |

Critical detail: the interactive console is on the UART bridge, not the native
USB CDC input path. The current configuration uses UART0 as the default console
and USB-Serial/JTAG only as a secondary output path.

If you type commands such as `confirm`, `cancel`, or interactive console input
into the native USB serial interface, the input may simply be discarded. Do not
interpret that as a firmware command-processing bug until the port identity is
verified.

Native USB serial reads can also be affected by the CDC open/handshake behavior
that resets or gates output. Open the intended port deliberately and record which
interface is being used.

---

## 9. Physical storage evidence already has a staged runbook

After V2 provisioning/authentication is genuinely working on the production V2
firmware, execute the prepared V2-035 hardware harness instead of inventing a
new ad-hoc storage test.

Read the complete runbook:

`docs/implementation-v2/V2_035_HARDWARE_EVIDENCE_HARNESS_2026-08-06.md`

It validates seven scenarios:

1. numeric ordering and deletion preservation;
2. real power-cycle persistence;
3. power interruption during a maximum-length upload;
4. temporary-file cleanup after reboot;
5. real storage exhaustion and HTTP 507;
6. mount failure without silent formatting;
7. final cleanup/evidence validation.

The collector is provenance-bound. Generate and use the production flash
manifest:

```bash
git status --short
idf.py -C firmware build
bash scripts/generate-flash-manifest.sh
python3 -m json.tool firmware/build/flash-manifest.json
```

The manifest must be clean and production. The collector verifies the application
image hash, ELF hash/build ID, ESP-IDF version, and the board-reported diagnostics
build ID before mutating blobs.

Example environment:

```bash
export DEVICE_URL='http://192.168.4.1'
export FLASH_MANIFEST="${PWD}/firmware/build/flash-manifest.json"
export V2_035_PASSWORD='use-a-disposable-real-test-password'
export V2_035_STATE='/tmp/esp32-macro-keyboard-v2-035-state.json'
```

Keep the password out of shell history where practical and never commit it.

The mount-failure test is intentionally destructive to the userdata partition.
Follow the runbook exactly. It backs up the partition, writes a deterministic
invalid image, proves the failed boot did not modify/format it, restores the
backup, and verifies byte identity. Keep raw images and complete serial logs
outside Git.

Final sanitized evidence belongs under `docs/hardware-evidence/` only after it
passes the harness validator.

---

## 10. On-device Unity validation is still required

The test app is currently device-build-tested in CI, not device-executed as part
of the V2 evidence.

Read:

`firmware/test_app/README.md`

The physical Phase 15 requirement explicitly says:

- build and flash the on-device test application;
- run the complete Unity menu on the reference board;
- record every test name and result;
- do not substitute a device-test build for execution.

Run the complete `*` menu at the appropriate validation point and retain the
serial results in a sanitized implementation/hardware report. Never claim this
complete from the CI Device Test Build workflow.

---

## 11. What remains after immediate Phase 4 work

Do not jump directly from Cutover B to "release validation". Continue
`docs/TODO_V2.md` in order and inspect existing code before deciding whether a
task needs implementation, adaptation, deletion, or only evidence.

The major remaining V2 phases are:

- **Phase 4 — Authentication, provisioning, and device settings**
  - finish V2-040;
  - hardware-calibrate/freeze V2-041;
  - V2-043 device UI preferences;
  - V2-044 Wi-Fi and reset semantics;
  - close the Phase 4 exit gate.
- **Phase 5 — Exact V2 HTTP API**
  - common route/auth/method/content-type/body/error policy;
  - exact route implementations and contract evidence.
- **Phase 6 — Macro compiler, executor, USB HID, and send lifecycle**
  - shared corpus compliance;
  - compile-before-execute semantics;
  - send lifecycle, cancellation, timeout, release-all;
  - real HID/cancellation hardware evidence.
- **Phase 7 — React repository core and persistence client**
  - strict repository validation;
  - gzip/snapshot client behavior;
  - dirty-state and browser-storage prohibition;
  - settings/send helpers.
- **Phase 8 — Startup, provisioning, and authentication UI**
  - first-run setup screens;
  - sign-in/session startup decision table;
  - first-phone and recovery behavior.
- **Phase 9 — Macros page and Quick Send operating console**
  - application shell;
  - send/confirmation/progress/cancel behavior;
  - no standalone progress/result route for ordinary Quick Send.
- **Phase 10 — Macro editing and package management**
  - editor/package workflows entirely in the React working copy;
  - dirty-state protection;
  - no firmware package/macro CRUD calls.
- **Phase 11 — Snapshots, import, and export UI**
  - manual additive snapshots;
  - explicit load/delete/import/export;
  - no automatic snapshot creation/deletion.
- **Phase 12 — Settings, diagnostics, and destructive operations UI**
  - settings/password/reset/factory-reset UX;
  - diagnostics with no package/macro data or secrets.
- **Phase 13 — Portrait phones, responsive layout, and accessibility**
  - Android portrait/landscape;
  - tablet/desktop landscape;
  - keyboard/screen-reader/accessibility evidence.
- **Phase 14 — Migration cleanup, static assets, scripts, documentation**
  - delete dead V1 code;
  - repository-wide V1 authority cleanup;
  - local-only static assets;
  - accurate final documentation.
- **Phase 15 — Full validation and release evidence**
  - software quality gates;
  - on-device Unity execution;
  - USB HID host matrix;
  - storage/power-failure matrix;
  - network/authentication matrix;
  - Android UI workflow matrix;
  - final secret-leak, partition-size, release-all, and spec/TODO reconciliation.

Many later tasks are still unchecked. Some existing pre-V2 or already-adapted code
may satisfy part of a later requirement, but **do not check the item merely
because similar code exists**. Audit it against the V2 requirement and add the
required evidence.

---

## 12. Phase 15 hardware evidence that must not be skipped

The final TODO explicitly requires physical evidence in several areas.

### 12.1 V2-151 — On-device Unity

Required:

- flash the device-test app;
- execute the complete Unity menu;
- record all tests/results.

### 12.2 V2-152 — USB HID hardware matrix

Required/expected:

- Linux validation;
- ChromeOS when a test machine is available;
- Windows when a test machine is available;
- record optional unavailable hosts without pretending they passed;
- verify USB identity, printable text, chords, release-all, cancellation,
  timeout, and reconnect from captured HID reports.

Linux is the minimum immediately available host in the current development
context. Use captured reports/evidence, not visual "it typed something" alone.

### 12.3 V2-153 — Storage and power-failure matrix

Required:

- add/list/load/delete on hardware;
- power-cycle byte identity;
- interrupted upload recovery;
- full-partition behavior;
- mount failure without formatting;
- factory reset and reprovisioning.

The V2-035 harness covers most of the storage/power-loss mechanics and should be
reused.

### 12.4 V2-154 — Network and authentication matrix

Required:

- first-run setup;
- minimal unauthenticated setup GET before provisioning;
- setup GET `404` after provisioning;
- login/logout;
- idle and absolute session expiry;
- lockout/rate-limit behavior;
- AP availability after station failure;
- bounded reconnect behavior;
- password change;
- PBKDF2 timing;
- no secret in serial, HTTP, logs, diagnostics, or exports.

### 12.5 V2-155 — Android UI workflow matrix

Required scenarios begin with:

- first-ever launch and setup;
- configured-device sign-in;
- first sign-in from a new Android phone;
- already-authenticated refresh;

Continue the full checklist in `docs/TODO_V2.md`. Do not reduce this to a desktop
browser smoke test.

---

## 13. Recommended continuation order

Use this as the practical execution sequence unless the authoritative TODO has a
stricter dependency for the specific subtask.

### Step A — establish the bench and clean baseline

1. Pull current `master`.
2. Confirm clean working tree and inspect commits after
   `34fb4bf4f3dfed94205b6294203dac3c05aabc3f`.
3. Source ESP-IDF v5.5.5.
4. Confirm Node v24.18.0.
5. Run `./scripts/check-all.sh` before invasive work.
6. Identify native USB and UART bridge by USB VID:PID.
7. Record actual board variant and available ports.

### Step B — run the PBKDF2 benchmark early

The physical board is now available, so remove the known V2-041 blocker early:

1. build/flash `firmware/test_app`;
2. run `[benchmark]` on the reference ESP32-S3R8;
3. capture the `PBKDF2_BENCH` results;
4. select the exact approximately 250–500 ms count;
5. freeze and test the selected constant;
6. run host/firmware/Quality gates.

This avoids implementing Cutover B around an arbitrary provisional iteration
count that immediately has to change.

### Step C — finish V2-040 Cutover B

1. add handler-level tests first;
2. replace the setup POST `503` with the prepared V2 contract path;
3. integrate password-material derivation using the measured constant;
4. commit through `device_settings_replace()`;
5. consume the code only after successful commit;
6. implement exact response/error/reconnect behavior;
7. run narrow tests;
8. run `./scripts/check-all.sh`;
9. require the four permanent CI workflows on the exact resulting SHA.

### Step D — physically prove first-run provisioning

On a clean/disposable device state:

1. erase/reset to the exact V2 unprovisioned state using the supported procedure;
2. boot production firmware;
3. capture the one-time setup code from the trusted serial path without
   committing it;
4. verify only setup GET/POST and required static assets are available;
5. verify setup GET exact field set;
6. exercise wrong/malformed code paths;
7. submit the correct setup request;
8. verify restart/reconnect behavior;
9. verify setup GET becomes `404`;
10. verify setup POST after provisioning becomes the specified conflict;
11. verify normal authenticated routes become available only after setup;
12. power-cycle and verify canonical settings persist;
13. prove the old setup code cannot be reused.

### Step E — finish V2-043 and V2-044

Complete device UI preferences and Wi-Fi/reset semantics, then execute the
physical portions of the Phase 4 exit gate. Especially verify that the protected
AP remains available when the optional station connection fails.

### Step F — continue Phases 5–14 in TODO order

For each task:

1. inspect current code;
2. write/update failing tests or acceptance harness;
3. implement the smallest coherent change;
4. run narrow loop;
5. run phase gate;
6. inspect diff for obsolete behavior, silent fallback, and untested error paths;
7. commit implementation report/evidence;
8. require exact-SHA permanent CI before moving the phase boundary.

### Step G — execute physical storage/HID/network/UI matrices

Use the existing hardware harnesses and add missing fail-closed harnesses where
necessary. Physical manipulation that cannot be automated should be recorded as
manual evidence with exact firmware SHA and observed result.

### Step H — final release evidence

Do not call V2 complete until:

- `./scripts/check-all.sh` passes from a clean checkout;
- every required hardware-dependent item has real board evidence;
- every V2 specification acceptance criterion has evidence;
- the complete on-device Unity suite has actually run;
- secret-leak review is clean;
- all terminal send paths release keys;
- images/partitions fit with recorded margins;
- TODO/spec/status documents match actual behavior;
- no unchecked hardware-dependent item is described as complete.

---

## 14. Evidence and commit discipline

Every completed V2 phase/task should add or update an implementation report under
`docs/implementation-v2/` with:

- task/phase ID;
- exact commit SHA;
- changed files/subsystems;
- exact commands;
- test names/counts/results;
- hardware model and ports when used;
- firmware build ID / flash manifest identity when applicable;
- host OS;
- measured timing/storage/memory/reconnect values;
- unresolved limitations;
- explicit statement that no unchecked item is being claimed complete.

For physical evidence, prefer a sanitized machine-readable artifact plus a short
human-readable report when the harness supports it.

Do not commit raw secrets or flash contents merely because they are useful during
debugging.

When a permanent CI gate fails:

1. inspect the actual failure;
2. fix the root cause;
3. do not expand an exception list automatically;
4. do not hide a warning;
5. do not claim a different SHA as evidence;
6. rerun all required gates on the exact new candidate SHA.

---

## 15. Known trap history worth preserving

Several recent failures are instructive. Avoid repeating them.

### npm audit drift

A newly surfaced `nanoid` advisory caused Quality to fail. The project did not
expand an allowlist. It updated the lockfile without `--force`, obtained a clean
audit, then removed obsolete exceptions and made the npm audit policy strict-zero.
Keep it that way.

### formatter failures

Cutover A was functionally green in other suites but Quality found non-canonical
C formatting. The code was formatted with the repository formatter; the gate was
not bypassed.

### Clang-Tidy include ownership

`misc-include-cleaner` found that `app_core.c` used V2 device-settings declarations
and `memcpy()` through transitive includes. The repair added the owning headers.
Do not paper over future include-cleaner findings.

### transient diagnostic workflows

Temporary self-removing workflows were used only to obtain exact CI diagnostics
when necessary. They were removed from the final tree. Do not leave one-off
repair/diagnostic workflows behind.

### port confusion

The native ESP32-S3 USB port and the UART bridge are different interfaces. An
interactive command sent to the wrong CDC path may disappear with no useful
response. Always identify the ports before debugging firmware behavior.

---

## 16. Current success criterion for the next handoff milestone

The next coherent milestone is **Phase 4 closure on real hardware**, not merely
"the board flashes."

A strong Phase 4 completion state should include all of the following:

- V2-040 fully implemented, no setup `503` stub remains;
- V2-041 exact PBKDF2 count measured on ESP32-S3R8 and frozen;
- V2-042 remains green without regression;
- V2-043 complete;
- V2-044 complete;
- real first-run setup succeeds;
- wrong/malformed/reused setup-code behavior is proven;
- setup endpoints disappear/transition exactly as specified after provisioning;
- canonical settings survive reboot/power cycle;
- protected AP remains available when station connection fails;
- no secret appears in logs/diagnostics/HTTP responses;
- host, sanitizer, coverage, browser, firmware, formatting, analyzer, and
  authoritative Quality gates are green;
- physical evidence is committed in sanitized form;
- Phase 4 checkboxes are updated only after the evidence exists.

After that, continue the remaining phases in `docs/TODO_V2.md` rather than
inventing a shortcut to release.

---

## 17. Final instruction to Claude Code

Treat the physical ESP32-S3 as an opportunity to close the gaps that hosted CI
cannot close, **not** as permission to replace the existing disciplined test
strategy with manual experimentation.

Preserve the architecture already established by the V2 rebuild. Prefer explicit
failure over silent fallback. Prefer a real fix over an exception. Prefer exact
SHA evidence over "it passed earlier." Prefer byte/report captures over visual
claims. When physical evidence and software evidence disagree, investigate the
disagreement and fix the product or test; do not choose the result that is more
convenient.

The immediate code target is the intentional
`firmware/components/web_server/web_server_setup.c` setup-submission `503`, but
benchmark V2-041 on the attached reference board first so Cutover B can use the
measured password-verifier policy from the start.
