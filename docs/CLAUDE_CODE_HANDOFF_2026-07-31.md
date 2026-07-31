# Claude Code Handoff — ESP32 Macro Keyboard

**Prepared:** 2026-07-31  
**Repository:** `ekkus93/esp32-macro-keyboard`  
**Branch:** `master`  
**State snapshot:** `7b591a9a1a051694d2fe53933720200cb8145f4b` immediately before this handoff file was added

## 1. Purpose

This document is the handoff from ChatGPT to Claude Code. It explains:

- the actual current repository state;
- what has been implemented and validated;
- why `master` is not currently clean or Quality-green;
- the exact cleanup required before new feature work;
- the next software phase;
- all known physical-device and hardware-integration work still outstanding;
- the evidence that must be captured before any hardware checkbox is marked complete.

Read this file before modifying the repository. Also read the canonical FIX1 files:

- `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_SPEC.md`
- `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md`
- `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_PROGRESS.md`
- `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_RESPONSES.md`

The TODO is authoritative for completion. The progress file is useful historical evidence, but its Phase 18 status is stale and must be corrected as part of the cleanup described below.

---

## 2. Operator constraints

These constraints came directly from the repository owner and must be preserved:

1. Work directly on `master`.
2. Do **not** create a branch or pull request unless the user explicitly asks for one.
3. Do not rewrite or force-push history merely to remove the recent cleanup-trigger commits. Make one normal forward cleanup commit.
4. Device tests were previously deferred only because ChatGPT did not have physical hardware. Claude Code will have access to an ESP32-S3 and should perform the device work listed in this handoff.
5. Do not claim physical validation from CI builds. A firmware image compiling is not the same as running on a physical board.
6. Do not mark a TODO checkbox complete without reproducible evidence: exact commit, command/procedure, result, and relevant logs or measurements.
7. Never place real credentials in committed files, logs, screenshots, diagnostic output, backup artifacts, or CI output. Use generated test sentinels.

---

## 3. Executive state summary

### Product implementation

Phases 1 through 17 of FIX1 are implemented according to the canonical progress file. Phase 18.1 through Phase 18.4 are implemented. Phase 18.5 secret-sentinel testing is also implemented in code, although the canonical TODO/progress documentation has not yet been synchronized.

The product code is substantially ahead of the stale Phase 18 status table. The immediate blocker is repository hygiene and a formatting gate, not missing Phase 18.5 functionality.

### Current `master` state

The pre-handoff `master` SHA was:

```text
7b591a9a1a051694d2fe53933720200cb8145f4b
```

This handoff file is added in a new commit immediately after that SHA. Run:

```bash
git rev-parse HEAD
git log --oneline --decorate -20
```

and use the actual checked-out SHA in all subsequent evidence.

### Important repository-history fact

The seven commits after `91af97eb6eaa57cc7074366d8cc539a0903ceadd` changed only temporary GitHub Actions cleanup files. They did **not** change firmware, frontend, storage, API, or test implementation code.

The diff from `91af97e` to the pre-handoff head touches only:

```text
.github/workflows/host-tests.yml
.github/workflows/phase18-5-docs.yml
.github/workflows/phase18-5-direct-finalize.yml
.github/workflows/phase18-5-direct-finalize.trigger
.github/workflows/phase18-5-direct-finalize.yml.trigger
```

Do not reset or rewrite history. Remove the temporary files and restore the permanent workflow in a normal forward commit.

### Current CI status at pre-handoff head

| Workflow | Result | Run | Commit | Interpretation |
| --- | --- | --- | --- | --- |
| Quality | **failed** | `30648740232` | `7b591a9` | Authoritative checks failed because `tests/host/CMakeLists.txt` is not `cmake-format` compliant. |
| Browser Tests | passed | `30648740132` | `7b591a9` | Real Chrome workflow passed. |
| Device Test Build | passed | `30648740308` | `7b591a9` | ESP32-S3 device-test firmware linted and compiled with ESP-IDF v5.5.5. This is build evidence only. |
| Host Tests matrix | passed on latest fully reported run | `30646145471` | `f6c210c` | Five jobs passed: host, ASan/UBSan, native coverage, frontend tests, frontend coverage. The exact final cleanup SHA still needs its own run. |

The Quality failure artifact contained the concise failure:

```text
Check failed: tests/host/CMakeLists.txt
```

The formatting gate is implemented in `scripts/check-format.sh` and runs:

```bash
cmake-format --check <first-party CMakeLists.txt files>
cmake-lint <first-party CMakeLists.txt files>
```

---

## 4. What is already implemented

### Phase 18.1 — bounded package reader

Implemented and host-tested:

- import byte ceiling is enforced before parsing;
- parser is non-recursive and allocation-bounded;
- unknown and duplicate fields are rejected;
- trailing data is rejected;
- object identities, references, and schema are validated;
- parsing is structurally isolated from repository mutation;
- a dedicated script regression gate compiles and exercises the production parser.

### Phase 18.2 — deterministic set export

Implemented and host/browser-tested:

- one repository-locked snapshot;
- deterministic output;
- all set-local macros included;
- only referenced global macros included;
- procedures and optional progress included;
- unreferenced globals excluded;
- credentials, sessions, provisioning material, and encryption stores excluded by construction;
- output bounded by `APP_IMPORT_PACKAGE_MAX_BYTES`;
- generated package revalidated before transfer;
- exact response framing and content length.

### Phase 18.3 — transactional set replacement

Implemented and host-tested:

- complete replacement staged before activation;
- staged readback validated;
- current set backed up;
- replacement activated transactionally;
- index updated;
- active set revalidated;
- backup and manifest removed only after success;
- recovery covers every durable phase;
- ambiguous and contradictory evidence fails closed.

### Phase 18.4 — full backup and restore

Implemented and extensively host-tested:

- backup includes all logical repository data: sets, local/global macros, procedures, ordering, and optional progress;
- credentials, sessions, CSRF material, provisioning state, encryption keys, schema markers, quarantine, staging, trash, and transaction evidence are excluded;
- backup is deterministic, bounded, and self-validated;
- restore validates the entire package before mutation;
- restore materializes and validates a complete staged repository;
- restore activation is all-or-nothing;
- startup resolves restore manifests before ordinary set transactions;
- six durable phases are tested;
- interruption, partial rename, contradictory evidence, and idempotent recovery are tested;
- linker-level fault injection verifies deterministic `APP_ERROR_STORAGE_FULL` during staging while preserving the old repository;
- API requires admin authentication, CSRF, same-origin policy, and physical confirmation;
- frontend requires strict package validation and the exact phrase `RESTORE FULL BACKUP`, then reloads after success.

### Phase 18.5 — secret-sentinel scanner

Implementation commit:

```text
5ae1aeb208716dc72679d3bf36bf4e56d4e5b627
```

Implemented files include:

```text
scripts/check-secret-sentinel.py
tests/scripts/test-secret-sentinel.py
scripts/check-scripts.sh
tests/host/test_storage_package_export.c
tests/host/test_storage_package_backup.c
```

The scanner detects a generated sentinel in these forms:

- raw UTF-8;
- JSON-escaped;
- URL encoded;
- base64;
- unpadded base64url;
- lowercase hexadecimal;
- uppercase hexadecimal.

The scanner deliberately does not echo the secret or encoded representation in failure output.

The regression fixture covers the five required boundary classes:

- set export;
- full backup;
- diagnostics;
- application logs;
- frontend persisted state.

Production-path host tests additionally assert that a resident sentinel is absent from serialized set-export and full-backup output.

Important nuance: the diagnostics/log/frontend fixtures currently prove the reusable scanner behavior against generated representative boundary artifacts. Phase 19 must continue using this scanner against the real diagnostics implementation and real persisted frontend output.

---

## 5. Immediate priority: restore a clean, documented, Quality-green repository

Do this before Phase 19 or device feature changes.

### 5.1 Start from a normal local clone

```bash
git clone https://github.com/ekkus93/esp32-macro-keyboard.git
cd esp32-macro-keyboard
git checkout master
git pull --ff-only
git status --short --branch
git log --oneline --decorate -20
```

Confirm there are no local modifications before beginning.

### 5.2 Inspect the cleanup-only diff

```bash
git diff --stat 91af97eb6eaa57cc7074366d8cc539a0903ceadd..HEAD
git diff 91af97eb6eaa57cc7074366d8cc539a0903ceadd..HEAD -- .github/workflows
```

Expected: only the temporary workflow files listed earlier.

### 5.3 Restore the permanent Host Tests workflow

The exact permanent `host-tests.yml` blob at `91af97e` is the desired workflow definition. Restore it with Git itself:

```bash
git show 91af97eb6eaa57cc7074366d8cc539a0903ceadd:.github/workflows/host-tests.yml \
  > .github/workflows/host-tests.yml
```

Verify that the restored workflow:

- has the normal five permanent jobs only;
- has no `phase18-5-cleanup` or finalizer job;
- uses least-privilege read permissions;
- does not push commits;
- does not install `cmakelang` solely to mutate the repository.

### 5.4 Delete every temporary Phase 18.5 cleanup file

```bash
rm -f \
  .github/workflows/phase18-5-docs.yml \
  .github/workflows/phase18-5-direct-finalize.yml \
  .github/workflows/phase18-5-direct-finalize.trigger \
  .github/workflows/phase18-5-direct-finalize.yml.trigger
```

Then confirm:

```bash
find .github/workflows -maxdepth 1 -type f -print | sort
git status --short
```

No Phase 18.5 finalizer, trigger, inspection, or documentation-sync workflow should remain.

### 5.5 Format the actual failing file

Use the same pinned formatter version expected by CI:

```bash
python3 -m pip install --user 'cmakelang==0.6.13'
cmake-format -i tests/host/CMakeLists.txt
cmake-format --check tests/host/CMakeLists.txt
cmake-lint tests/host/CMakeLists.txt
```

Review the diff carefully:

```bash
git diff -- tests/host/CMakeLists.txt
```

The change should be formatting only. Do not alter targets, sources, link options, test names, fault-injection wrapping, or coverage behavior.

### 5.6 Synchronize the canonical Phase 18.5 documentation

Edit:

```text
docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md
docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_PROGRESS.md
```

In the TODO, change only the five Phase 18.5 items from `[ ]` to `[x]`:

```markdown
- [x] set export;
- [x] full backup;
- [x] diagnostics;
- [x] logs;
- [x] frontend persisted state.
```

Add a concise implementation note immediately after those items. It should identify:

- `scripts/check-secret-sentinel.py`;
- `tests/scripts/test-secret-sentinel.py`;
- raw/JSON/URL/base64/base64url/hex detection;
- non-echoing failure diagnostics;
- production-path set-export and full-backup assertions.

In the progress file:

1. Change the Phase 18 row from “in progress (§18.1–18.4 complete; §18.5 remains)” to complete.
2. Add a Phase 18.5 completion section.
3. Record implementation commit `5ae1aeb208716dc72679d3bf36bf4e56d4e5b627`.
4. Record known green Host matrix evidence, but state that final closure depends on the new cleanup commit’s exact CI run.
5. State explicitly that physical-device validation was previously deferred and is now handed to Claude Code.
6. Do not insert a Quality run ID until the final cleanup SHA actually passes.

### 5.7 Run the local software gates

At minimum:

```bash
./scripts/check-format.sh
./scripts/check-scripts.sh
python3 tests/scripts/test-secret-sentinel.py
./scripts/run-tests.sh
./scripts/run-tests.sh --sanitizers
./scripts/generate-native-coverage.sh
./scripts/generate-frontend-coverage.sh
./scripts/build-device-tests.sh
./scripts/check-all.sh
```

Notes:

- `check-all.sh` is the authoritative aggregate gate.
- Run it from a clean dependency/toolchain environment matching CI as closely as practical.
- ESP-IDF is pinned to v5.5.5.
- Do not ignore warnings or append `|| true` to make a gate pass.

### 5.8 Make one normal cleanup commit

Suggested message:

```text
chore: finalize Phase 18.5 validation
```

Before committing:

```bash
git diff --check
git status --short
git diff --stat
```

The expected changed set is approximately:

```text
M  tests/host/CMakeLists.txt
M  docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md
M  docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_PROGRESS.md
M  .github/workflows/host-tests.yml
D  .github/workflows/phase18-5-docs.yml
D  .github/workflows/phase18-5-direct-finalize.yml
D  .github/workflows/phase18-5-direct-finalize.trigger
D  .github/workflows/phase18-5-direct-finalize.yml.trigger
```

This handoff file may already be committed separately and should not be modified merely to squash history.

Push directly to `master` as instructed by the owner.

### 5.9 Closure definition for housekeeping

Housekeeping is complete only when the **same final SHA** has:

- Quality passed;
- all five Host Tests jobs passed;
- Browser Tests passed;
- Device Test Build passed;
- no temporary Phase 18.5 workflow or trigger files;
- the permanent Host Tests workflow restored;
- Phase 18.5 TODO checkboxes and progress evidence synchronized;
- physical tests still accurately described as unexecuted until Claude runs them.

After the runs finish, update the progress entry with the exact final SHA and run IDs if the project’s evidence convention requires a follow-up documentation commit. If a documentation-only evidence commit is added, validate that commit too; do not call an earlier code SHA the final repository state.

---

## 6. Next software work after cleanup: Phase 19

Do not skip directly to release closure. Phase 19 is the next canonical implementation phase.

### 6.1 Phase 19.1 — subsystem health records

Add stable, thread-safe snapshots for:

- app lifecycle;
- storage mount and recovery;
- repository;
- authentication;
- USB;
- executor;
- device controls;
- Wi-Fi;
- HTTP server.

Requirements:

- retain primary and cleanup errors separately;
- do not collapse cleanup failure into the primary result;
- do not report “healthy” while resource ownership or cleanup is incomplete;
- make reads bounded and race-safe;
- use stable enums/fields suitable for API serialization;
- do not include secret material, macro source, credentials, tokens, or raw storage contents.

Some subsystem health structures already exist or were partially implemented in earlier phases. Reuse them rather than creating parallel state. Search for existing `*_get_health()` APIs and lifecycle result structures.

### 6.2 Phase 19.2 — redacted diagnostics route

The diagnostics response must include only allowlisted fields such as:

- build ID;
- firmware version;
- schema version;
- reset reason;
- uptime;
- heap metrics;
- task stack high-water marks;
- webfs/userdata capacity;
- quarantine count;
- current execution state;
- subsystem health snapshots.

Requirements:

- stable bounded schema;
- authenticated/authorized policy consistent with the API spec;
- no secret material;
- no raw macro source;
- no session token, CSRF token, credential hash, NVS key, setup secret, Wi-Fi password, or unredacted request data;
- explicit degraded/error states when a subsystem query fails.

### 6.3 Phase 19.3 — diagnostics tests

Test:

- exact field allowlist;
- unknown/unexpected field absence;
- fresh sentinel absent in raw and encoded forms;
- bounded output;
- subsystem query failure behavior;
- primary and cleanup error preservation;
- no false healthy state;
- API framing and frontend response validation;
- frontend persisted state through the Phase 18.5 scanner.

The Phase 18.5 scanner should become a reusable acceptance gate for real Phase 19 output, not remain only a synthetic fixture.

---

## 7. Physical ESP32-S3 work: current status

### What CI has proven

The device-test application currently:

- lints successfully;
- compiles for ESP32-S3;
- uses ESP-IDF v5.5.5;
- has a green Device Test Build at pre-handoff head (`30648740308`).

### What has **not** been proven

No physical serial output has been reviewed. The current device-test README explicitly says the application is build-tested only.

The test application currently covers:

- hardware-RNG UUID generation;
- UUID validation;
- macro parsing and compilation;
- parser failure atomicity;
- authoritative firmware limits;
- authentication adapters;
- executor idle/USB-not-ready behavior;
- USB keyboard state initialization.

It does not by itself prove:

- USB enumeration with a real host;
- real HID keystrokes;
- disconnect/reconnect;
- host suspend/resume;
- SoftAP clients and browser workflows;
- physical button timing;
- encrypted NVS persistence on actual flash;
- power-loss recovery;
- storage-full behavior on actual flash;
- absence of stuck modifiers after interruption;
- timing budgets.

Claude Code has access to a physical ESP32-S3 and should now close these gaps.

---

## 8. Device-test preparation and evidence discipline

### 8.1 Record the environment before flashing

Capture:

```bash
git rev-parse HEAD
git status --short --branch
idf.py --version
python3 --version
uname -a
sha256sum firmware/dependencies.lock
```

Also record:

- exact ESP32-S3 board/model;
- USB-to-UART interface, if any;
- native USB port used for HID;
- serial device path;
- Linux distribution/kernel;
- ChromeOS device/version when ChromeOS tests are run;
- cable type and whether the cable supports data;
- external power arrangement, if used.

Use a non-secret evidence directory outside the repository or under an ignored artifact directory. Suggested layout:

```text
artifacts/device-validation/<YYYY-MM-DD>/<commit>/
  environment.txt
  unity-device-tests.log
  production-boot.log
  usb-linux.md
  usb-chromeos.md
  softap-browser.md
  power-interruption.md
  physical-controls.md
  metrics.md
```

Do not commit large binary dumps or logs containing secrets. Commit concise redacted summaries and reference retained local evidence as appropriate.

### 8.2 Identify ports safely

Typical commands:

```bash
ls -l /dev/ttyACM* /dev/ttyUSB* 2>/dev/null || true
udevadm monitor --udev --property
```

Many ESP32-S3 boards expose separate native-USB and UART/JTAG ports. Confirm which port is used for flashing/monitoring and which is used for HID. Do not assume `/dev/ttyUSB0`.

### 8.3 Activate the pinned toolchain

```bash
./scripts/install-esp-idf.sh
. "$HOME/esp/esp-idf-v5.5.5/export.sh"
idf.py --version
```

If ESP-IDF is already installed elsewhere, still verify that the active version is exactly v5.5.5 before recording results.

---

## 9. Run the physical Unity device-test application

### 9.1 Build

From repository root:

```bash
bash ./scripts/build-device-tests.sh
```

### 9.2 Flash and monitor

From `firmware/test_app`:

```bash
idf.py -B build -p "$PORT" flash monitor
```

Press Enter to display the Unity menu. Run all tests with:

```text
*
```

Also run the individual tags if needed to isolate failures:

```text
[device]
[uuid]
[macro_parser]
[limits]
[auth]
[executor]
[usb]
```

### 9.3 Acceptance

Record the complete serial output. Required result:

- zero failed Unity tests;
- no panic, watchdog reset, abort, heap corruption, or unexpected reboot;
- deterministic repeated run, preferably at least three consecutive full runs;
- reboot and rerun once to catch initialization residue;
- exact commit and IDF version in evidence.

If any physical Unity test fails, stop and fix it before relying on broader integration results.

---

## 10. Production firmware clean build and flash

Run from a clean checkout or after removing generated build directories:

```bash
./scripts/check-all.sh
idf.py -C firmware fullclean
idf.py -C firmware build
./scripts/build-device-tests.sh
```

Flash production firmware:

```bash
idf.py -C firmware -p "$PORT" flash monitor
```

Capture the complete first boot and at least one warm reboot.

Verify immediately:

- no automatic LittleFS format on mount failure;
- no fallback to an open Wi-Fi network;
- expected setup/normal-mode selection;
- no plaintext credential output;
- no panic or watchdog reset;
- expected USB and Wi-Fi subsystem startup state;
- explicit visible failure if a subsystem does not start.

---

## 11. Phase 20.1 — clean production-build metrics

Record all canonical metrics:

- exact commit;
- ESP-IDF version;
- `firmware/dependencies.lock` hash;
- application binary size;
- OTA slot size and remaining headroom;
- webfs image size and partition headroom;
- static RAM usage;
- peak heap on device;
- task stack high-water marks.

Useful commands include:

```bash
idf.py -C firmware size
idf.py -C firmware size-components
idf.py -C firmware partition-table
ls -lh firmware/build/*.bin
```

Heap and stack metrics may require Phase 19 diagnostics or temporary test-only instrumentation. Do not invent values. If Phase 19 is not complete, record those items as blocked by Phase 19 rather than marking them passed.

---

## 12. Phase 20.2 — USB host matrix

Run the full matrix on Linux and ChromeOS. Use a safe text editor or dedicated HID test page so keyboard shortcuts cannot damage data.

### 12.1 Linux evidence tools

Useful monitoring commands:

```bash
sudo dmesg -w
lsusb
lsusb -t
sudo evtest
```

Record VID/PID, product strings, interface enumeration, and reconnect behavior.

### 12.2 Required cases

#### Enumeration

- cold-plug device;
- boot with host already connected;
- verify HID keyboard interface appears;
- verify no repeated enumeration loop;
- record host logs.

#### Disconnect/reconnect

- disconnect while idle;
- reconnect without rebooting device;
- reconnect after device reboot;
- verify the executor and USB health state recover.

#### Suspend/resume

- suspend host with device attached;
- resume host;
- verify enumeration and keystrokes still work;
- verify no stuck key or modifier.

#### Printable text

Exercise representative printable characters, whitespace, punctuation, and line breaks. Compare exact expected and received text.

#### Chords

Exercise safe modifier chords and confirm press/release ordering. Use harmless targets. Verify Ctrl/Alt/Shift/GUI release reliably.

#### Delay cancellation

Run a macro containing a 10-second delay. Cancel it with the physical control. Verify:

- cancellation occurs within the specified latency budget;
- no action after the cancellation point executes;
- terminal state is cancellation, not success;
- all HID reports are released.

#### Rapid typing cancellation

Run a long rapid-typing macro and cancel mid-stream. Verify:

- bounded cancellation latency;
- no continued typing after cancellation settles;
- no stuck modifier;
- executor returns to an operable idle state.

#### Disconnect during execution

Disconnect USB while a macro is running. Verify:

- failure is visible;
- executor terminates or cancels according to the specification;
- reconnect does not resume stale execution;
- release-all is attempted/observed;
- the next execution can start cleanly.

#### Release-all observation

Use `evtest` or another host event monitor to prove that all pressed keys/modifiers receive releases after:

- normal completion;
- cancellation;
- USB failure;
- device shutdown/reboot path, where observable.

### 12.3 ChromeOS

Repeat the canonical matrix on ChromeOS, not just Linux. Record ChromeOS version and hardware. At minimum cover enumeration, disconnect/reconnect, suspend/resume, printable text, chords, cancellation, disconnect during execution, and release-all behavior.

---

## 13. Phase 20.3 — SoftAP and browser integration

Use generated non-sensitive test credentials.

### Required workflow

1. Erase or reset to a documented first-run state.
2. Connect a client to the setup SoftAP.
3. Complete first-run setup.
4. Reboot and power-cycle.
5. Verify provisioning persists.
6. Verify the device does not re-enter setup unexpectedly.
7. Log in through the production frontend.
8. Exercise rate limiting with controlled invalid logins.
9. Verify Host and Origin rejection behavior.
10. Verify session expiry and post-expiry UI behavior.
11. Exercise set CRUD and ordering.
12. Exercise macro CRUD, validation, and ordering.
13. Exercise procedure CRUD and progress.
14. Execute and cancel macros.
15. Exercise import/export.
16. Exercise full backup and restore with physical confirmation and typed phrase.
17. Disconnect the browser/client network and reconnect.
18. Verify UI state refreshes from the device rather than using stale mock state.

### Encrypted persistence on real flash

Behavioral reboot persistence is necessary but not sufficient. Use a generated sentinel SSID/password and verify it is not present as plaintext in:

- serial logs;
- diagnostics;
- backup/export artifacts;
- a raw NVS partition dump, where practical.

A possible test approach:

1. Record partition offsets from the built partition table.
2. Provision a unique generated test credential.
3. Power-cycle and prove the credential still works.
4. Read only the relevant flash partition with `esptool.py`.
5. Search the dump for the raw sentinel and encoded forms without printing the secret.
6. Securely delete the dump after recording a redacted pass/fail result.

Do not use a real household or production Wi-Fi password for this test.

---

## 14. Phase 20.4 — real power-interruption testing

Randomly pulling power is not enough. The test must target each durable transaction phase.

### Transactions to cover

At minimum:

- atomic object write/recovery;
- set transactional replacement;
- full repository restore;
- any additional manifest-based repository transaction introduced by later work.

### Recommended deterministic method

Add test-only instrumentation or a dedicated hardware-test build that can pause or intentionally reboot after a named durable phase. The instrumentation must be excluded from production configuration and guarded by a compile-time test flag.

For each phase:

1. Start from a known repository state A.
2. Prepare replacement state B.
3. Trigger the transaction.
4. Stop immediately after the target durable phase is committed.
5. Remove power abruptly or trigger a hardware reset that accurately models the intended fault.
6. Reboot production recovery code.
7. Inspect active repository state and preserved evidence.
8. Repeat enough times to establish deterministic behavior.

### Required assertions

For every phase:

- state is complete A or complete B;
- no mixed active set/repository;
- no automatic filesystem format;
- recovery failure is visible;
- ambiguous corruption preserves evidence;
- no silent fallback to an empty repository;
- startup does not proceed as healthy when recovery is incomplete;
- subsequent valid operation remains possible after successful recovery.

The “recoverable diagnostics” item depends on Phase 19. Do not mark it complete before the real diagnostics route reports the recovery state correctly.

---

## 15. Actual-flash storage-full testing

Host fault injection already proves the software rollback path. Physical validation must exercise real partition capacity.

Recommended procedure:

1. Use a test-only helper or repeated valid imports to fill userdata close to capacity.
2. Record free-space metrics before the transaction.
3. Attempt a set replacement and full restore whose staging data cannot fit.
4. Verify the operation reports storage full, not generic corruption or success.
5. Reboot.
6. Verify the old complete repository remains active.
7. Verify schema markers remain intact.
8. Verify no unexpected auto-format occurred.
9. Verify staging/trash/manifest evidence is cleaned or preserved exactly according to the transaction state.
10. Confirm a smaller valid operation succeeds after space is reclaimed.

Never fill the flash with arbitrary corrupt files unless the specific test is for corruption handling. Prefer valid repository objects so the failure is truly capacity-related.

---

## 16. Phase 20.5 — physical controls

Measure, do not estimate.

### Required measurements

- cancellation latency during a 10-second delay;
- cancellation latency during rapid typing;
- physical confirmation timeout;
- reset gesture duration;
- accidental short-press rejection.

### Measurement guidance

Use serial timestamps, host HID event timestamps, or synchronized video. For cancellation latency, record:

- physical press time;
- firmware observation time, if logged safely;
- final HID release event time;
- terminal executor state time.

Run multiple trials and record minimum, maximum, median, and any outlier. The canonical TODO does not define all numeric thresholds in the excerpt, so consult the spec and implementation constants before judging pass/fail.

Verify that accidental short presses do not trigger destructive reset or privileged confirmation.

---

## 17. Device tasks dependent on Phase 19

The following physical tasks should be completed after real health aggregation and diagnostics exist:

- peak heap reported through the production diagnostics path;
- task stack high-water marks;
- subsystem health snapshots under real fault conditions;
- reset reason and uptime;
- webfs/userdata capacity;
- quarantine count;
- current execution state;
- recovery diagnostics after power interruption;
- proof that cleanup failure is not reported as healthy;
- real diagnostics sentinel scan;
- frontend diagnostics rendering and persisted-state scan.

Do not add temporary debug output that leaks secrets merely to collect these metrics. Prefer the redacted Phase 19 API.

---

## 18. Remaining non-device phases after Phase 20

### Phase 21 — release budgets and immutable CI

Still open:

- application OTA-slot budget gate;
- webfs partition budget gate;
- minimum userdata free-space feasibility gate;
- static RAM budget;
- task stack margin threshold;
- pin all GitHub Actions to full commit SHAs with human-readable version comments;
- runner/tool version documentation;
- least-privilege permissions;
- production-configuration gate rejecting credential logging, disabled NVS encryption, missing security configuration, setup bypasses, debug servers, and remote assets.

### Phase 22 — documentation synchronization

Still open across README/API/development/status/security/recovery/hardware/release/TODO documents. Correct stale claims and clearly distinguish:

- implemented;
- host-tested;
- browser-tested;
- device-build-tested;
- physically device-tested;
- release-ready.

### Phase 23 — final regression and acceptance

Must run all software gates and all hardware/browser workflows on the final candidate. Do not declare FIX1 complete while any checkbox remains open.

---

## 19. Important files for orientation

### Canonical planning and status

```text
docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_SPEC.md
docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md
docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_PROGRESS.md
docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_RESPONSES.md
```

### API and frontend

```text
docs/API.md
webapp/src/api/packages.ts
webapp/src/features/packages/PackageOperationsPage.tsx
webapp/src/features/diagnostics/
```

Search for the actual diagnostics page path if it differs; Phase 19 may currently contain a partial or placeholder surface.

### Storage package and recovery

```text
firmware/components/storage/
tests/host/test_storage_package_export.c
tests/host/test_storage_package_backup.c
tests/host/test_storage_package_restore.c
tests/host/cmake/phase18_4_restore_tests.cmake
```

### Secret scanning

```text
scripts/check-secret-sentinel.py
tests/scripts/test-secret-sentinel.py
scripts/check-scripts.sh
```

### Device tests

```text
firmware/test_app/README.md
firmware/test_app/main/
scripts/build-device-tests.sh
.github/workflows/device-tests-build.yml
```

### Permanent quality gates

```text
scripts/check-all.sh
scripts/check-format.sh
scripts/check-firmware.sh
scripts/run-tests.sh
scripts/generate-native-coverage.sh
scripts/generate-frontend-coverage.sh
.github/workflows/quality.yml
.github/workflows/host-tests.yml
.github/workflows/browser-tests.yml
.github/workflows/device-tests-build.yml
```

---

## 20. Known pitfalls

1. **Do not confuse build success with physical device success.** Device Test Build is green, but physical Unity tests have not been reviewed.
2. **Do not preserve the temporary finalizer workflows.** They were failed workarounds and are not product infrastructure.
3. **Do not restore `host-tests.yml` from `HEAD^`.** Several recent ancestors already contain temporary cleanup jobs. Restore the exact file from `91af97e` or compare carefully against that blob.
4. **Do not force-push to erase cleanup commits.** Use a normal forward cleanup commit.
5. **Do not globally replace matching Phase 18.5 checklist text.** Restrict edits to the `### 18.5 Secret scanner tests` section.
6. **Do not claim Phase 18 complete until the canonical docs and exact final CI are green.**
7. **Do not print sentinel secrets in failing tests.** Existing scanner diagnostics intentionally print only representation type and output path.
8. **Do not add recursive or unbounded diagnostics serialization.** ESP32 memory and response size are constrained.
9. **Do not expose raw macro source in diagnostics.**
10. **Do not report cleanup failure as the primary error or as healthy.** Preserve primary and cleanup errors independently.
11. **Do not test destructive keyboard chords in an unsafe application.** Use a dedicated text editor/test environment.
12. **Do not test encrypted persistence with real credentials.** Use generated sentinels.
13. **Do not perform nondeterministic power-cut testing without phase identification.** Add test-only fault hooks so each durable phase is actually covered.
14. **Do not mark a hardware checkbox based on one trial.** Repeat timing and interruption tests and retain evidence.

---

## 21. Recommended execution order

1. Clone and inspect current `master`.
2. Restore permanent `host-tests.yml` from `91af97e`.
3. Delete all four Phase 18.5 temporary workflow/trigger files.
4. Format `tests/host/CMakeLists.txt` with `cmakelang==0.6.13`.
5. Synchronize Phase 18.5 TODO and progress documentation.
6. Run all local software gates.
7. Commit and push one cleanup commit directly to `master`.
8. Confirm Quality, five-job Host matrix, Browser Tests, and Device Test Build all pass on the exact final SHA.
9. Run the physical Unity device tests and record serial evidence.
10. Begin Phase 19 health and diagnostics implementation.
11. Re-run software gates and real diagnostics secret scanning.
12. Execute the Phase 20 physical matrix on Linux and ChromeOS.
13. Implement Phase 21 release gates.
14. Synchronize Phase 22 documentation.
15. Run Phase 23 final acceptance on one release-candidate SHA.

The first action should be repository cleanup, not new functionality. The first physical action after cleanup should be running the existing Unity device-test image on the ESP32-S3 so the project obtains its first reviewed physical test evidence.

---

## 22. Evidence template

Use a consistent format when closing a task:

```markdown
### <task name>

- Commit: `<full SHA>`
- Date/time: `<ISO-8601 with timezone>`
- Hardware: `<board/revision>`
- Host: `<OS/version>`
- ESP-IDF: `<exact version>`
- Commands/procedure:
  - `<command or numbered procedure>`
- Expected result: `<objective criterion>`
- Actual result: `<measured result>`
- Repetitions: `<count>`
- Evidence:
  - `<redacted log/artifact path or CI run>`
- Secrets reviewed: `<yes/no and scanner command>`
- Deviations/limitations: `<none or explicit limitation>`
- TODO checkbox updated: `<path and section>`
```

A task is not complete merely because the code appears correct. It is complete when implementation, automated validation, physical validation where applicable, documentation, and reproducible evidence agree.
