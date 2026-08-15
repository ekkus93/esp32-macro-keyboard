# H12-122 — Final hardware confirmation preparation

- **Date:** 2026-08-15
- **Task:** H12-122 — Final hardware confirmation on exact release SHA
- **Starting repository state:** user-supplied latest `master` archive; live `master` was observed at `457788759656c4d8c20feb9e32cae9c44d7ff96e` when this work began
- **Interim published preparation:** `0d2a43f9ab7ab191c1ab8226fcf5c113c67ff7d2`, followed by formatting-only `7205901db607465022c2f869d2ac1ddc3e8c54d1`
- **Disposition:** preparation complete; physical H12-122 acceptance remains open

## Scope

H12-122 requires one exact production release SHA to be built, flashed to the
reference ESP32-S3, identified unambiguously from live diagnostics, exercised
through the bounded final smoke sequence, and left running the same production
image at sign-off.

The pre-hardware audit found release-blocking correctness and provenance gaps.
Those defects required production/runtime and release-tooling changes. Therefore
the prior H12-120/H12-121 exact-SHA evidence is now historical only: a replacement
candidate must first pass H12-120 and H12-121 before this H12-122 hardware run can
truthfully close.

No H12-122 checkbox is checked by this preparation work alone.

The interrupted work published an initial setup-code/HIL repair as `0d2a43f9...`
and a formatting-only follow-up as `7205901d...`. The final preparation was
reconciled on top of that live state. It preserves the published fail-closed
logout/HID/cleanup corrections while superseding the interim automatic setup-code
disclosure and split hardware-smoke path with the explicit `setup-code` command,
manifest-driven release flasher, and unified H12-122 runner described below.

## Defects found and repaired

### 1. Factory-reset reprovisioning could dead-end

The current v2 setup contract uses a fresh random eight-digit setup code on every
unprovisioned boot. H9 correctly removed that secret from normal logs, but the
remaining startup guidance pointed the operator to the manufacturing label. The
label cannot contain a per-boot random code, so after factory reset there was no
supported way to recover the new setup code and complete reprovisioning.

The repair adds an explicit `setup-code` command to the trusted physical UART0
console. The code is published only after a valid setup code exists, is cleared
on setup-mode startup failure, and is retired immediately when setup succeeds.
It is not restored until a later unprovisioned boot creates a new code.
Serial-console startup now treats creation of the setup-code synchronization
mutex as a prerequisite; once a code could have been published, an impossible
`portMAX_DELAY` clear-lock failure is an assertion/panic condition instead of
silently retaining stale disclosure authority.

The secret-bearing response does not use normal `printf`/logging. Production
configuration mirrors ordinary stdout/stderr to USB-Serial-JTAG as a secondary
console, so the setup-code response writes directly to UART0 instead. The
credential-output policy and regression tests encode this as one narrow,
source-specific exception and reject stdout disclosure, duplicate disclosure,
and near-match output sinks.

The manufacturing label is correspondingly limited to the deterministic
bootstrap AP identity/passphrase; it no longer claims to carry the random setup
code.

### 2. Trusted-UART station configuration used a retired persistence store

`wifi-connect` still wrote station credentials through the retired
`provisioning` NVS repository even though current startup owns station settings
through `device_settings`. This could associate successfully for the current
boot while failing to persist the network expected after setup/restart.

The command now performs an atomic read/modify/write through the authoritative v2
settings store and treats persistence failure as command failure. The HIL helper
also refuses to accept an IP address unless the console explicitly confirms that
the station settings were durably saved.

The v2 settings validator was also corrected so an unprovisioned device may
carry a valid station configuration while still having no provisioned AP/auth
credentials. Setup preserves those station fields, and setup-mode startup now
uses a persisted station configuration so a trusted-UART network configured
before setup survives a reboot.

The obsolete `credential-reset` serial path and its stale acceptance helper were
removed instead of retaining a command backed by the retired store.

### 3. The HIL setup helper implemented a retired API flow

`tests/hardware/provision_device.py` still targeted the old multi-route setup
flow. It now uses the current `GET /api/v1/setup` / `POST /api/v1/setup`
contract, obtains the one-time setup code only through the physical UART command,
stores replacement AP/admin credentials outside the repository before the
transactional submission, waits through the mandatory restart, and verifies
that `setup-code` is unavailable after provisioning.

The shared UART helper now uses ESP-IDF console-compatible double-quote/backslash
escaping rather than POSIX shell quoting, so SSIDs and passwords containing
spaces, quotes, or backslashes are not split incorrectly.

### 4. Release flashing did not prove the complete flashed artifact set

The previous flash manifest recorded application-image provenance but did not
SHA-256 hash every image in the flash set, including `webfs.bin`. Ordinary
`idf.py flash` also did not itself prove that the separately generated webfs
image in the release manifest was the one written during final acceptance.

`generate-flash-manifest.sh` now records a SHA-256 for every flash-file offset.
The new `scripts/flash-release-manifest.py` validates before writing:

- exact 40-character requested Git SHA;
- manifest Git SHA equality;
- `gitDirty=false` and `buildType=production`;
- exact `ESP-IDF v5.5.5` provenance;
- a clean source checkout at the requested exact SHA;
- managed-component and frontend lockfile hashes;
- canonical unique flash offsets and valid flash settings;
- path containment beneath the manifest directory;
- exact SHA-256 coverage and equality for every flash artifact;
- inclusion of `webfs.bin`;
- the production `esp32_macro_keyboard.bin` application image; and
- application ELF/build-ID provenance.

Only after all of those checks pass does it invoke `esptool`. `--dry-run` exists
for non-writing provenance checks but does not count as H12-122 acceptance.

### 5. Live diagnostics matching was weaker than the actual build-ID contract

ESP-IDF controls the resident application ELF-SHA string length with
`CONFIG_APP_RETRIEVE_LEN_ELF_SHA`; the v5.5.5 SDK default is shorter than the
firmware's 40-byte diagnostics destination buffer. Older project hardware
evidence therefore observed a shorter board-visible prefix even though the
manifest recorded 39 characters. Depending on that SDK default would weaken the
final equality check.

Production `sdkconfig.defaults` now pins `CONFIG_APP_RETRIEVE_LEN_ELF_SHA=39`,
which fills the existing 40-byte diagnostics buffer with 39 lowercase hex
characters plus NUL. `check-production-config.sh` requires that value, and
`generate-flash-manifest.sh` independently rejects a resolved build that
overrides it. The final H12 harness therefore requires exact 39-character
equality with the manifest's `diagnosticsBuildId`. The manifest separately
records the full application ELF SHA-256 and requires the 39-character ID to be
its prefix.

### 6. No single fail-closed H12-122 acceptance sequence existed

The new `scripts/run-h12-122-hardware.py` owns the final board sequence. It first
invokes the exact-manifest release flasher itself and then verifies, in one
bounded run:

1. initial authenticated diagnostics match the flashed manifest;
2. an ordinary active send types the exact expected HID text and releases keys;
3. a confirmation-required send produces no key-down before UART `confirm`, then
   types exactly once and releases keys;
4. cancellation during an active delay types nothing and ends released;
5. a gzip snapshot can be saved and loaded byte-identically;
6. password change invalidates the active session, rejects the old password, and
   accepts the new password;
7. software restart returns to the same production build;
8. factory reset explicitly requires reprovisioning;
9. fresh reprovisioning succeeds using the new physical-UART setup code;
10. the H12 snapshot is absent after reset; and
11. the same production build ID remains present at final sign-off.

The emitted JSON evidence deliberately excludes passwords, passphrases, setup
codes, session cookies, Wi-Fi credentials, and macro source text.

## Follow-up fail-closed acceptance audit

A second pre-device audit found several ways the unified runner could still
overclaim a destructive acceptance result even though the production behavior
was wrong. Those acceptance-path defects are now closed before a real board run:

- restart acceptance no longer treats simple HTTP reachability as proof that a
  reboot occurred; diagnostics must report a software reset and an uptime
  discontinuity large enough to rule out continuous execution;
- the restart path may not recover a missing station connection by issuing
  UART `wifi-connect`, because doing so would mask a station-persistence or
  bounded-reconnect regression;
- H12 reprovisioning disables the general provisioning helper's post-setup UART
  Wi-Fi recovery, so the station configuration written before setup must survive
  setup's mandatory restart;
- factory-reset acceptance must observe a fresh setup code on the trusted UART
  before any reprovisioning attempt, and the provisioning helper is told to
  reject an already-provisioned device instead of returning "nothing to do";
- after reprovisioning, the administrator password that existed before factory
  reset must be rejected;
- the flash/native-USB and UART-console device arguments must be distinct, an
  existing evidence output file is never overwritten, and the release manifest
  must remain byte-identical through the flash operation; and
- the JSON restart step records the pre/post uptime values, elapsed observation
  time, and reset reason so the reboot proof is independently reviewable without
  recording any secret.

These are harness/evidence integrity changes only. They do not constitute a
physical H12-122 pass, and they intentionally keep every H12-122 checkbox open.

## Permanent regression coverage

The preparation adds or extends permanent coverage for the affected boundaries,
including:

- `tests/scripts/test-flash-release-manifest.py` — fail-closed release-flash
  manifest/source/artifact validation;
- `tests/scripts/test-h12-122-hardware.py` — final-HIL sequencing, provenance,
  no-secret evidence, and UART-only setup-code policy;
- `tests/scripts/test-generate-flash-manifest.sh` — complete flash-file hash
  coverage;
- `tests/scripts/test-check-credential-logging.sh` — UART-only setup-code output
  exception and leak rejection;
- `tests/scripts/test-generate-setup-label.sh` — manufacturing-label scope;
- host contract tests for setup-code lifecycle, setup submission, unprovisioned
  station validity, durable station updates, setup-mode station reuse, and
  provisioning bootstrap behavior; and
- `scripts/check-v2-device-settings-policy.py` / related architecture and route
  policies for the current ownership boundaries.

During the full host-suite rerun, the new `app_core` assertions exposed a missing
CMake dependency: `app_core_tests` used the v2 device-settings validator but did
not link `device_settings_v2.c`. `tests/host/CMakeLists.txt` now declares that
source explicitly rather than relying on isolated/manual test compilation.

## Local validation

The following checks passed in the local sandbox after the H12-122 repairs:

- normal host suite: **66/66 passed, 0 failed**;
- ASan/UBSan host suite: **66/66 passed, 0 failed**;
- coverage-instrumented host suite: **66/66 passed, 0 failed**;
- `test-flash-release-manifest.py`: **14 cases passed**;
- `test-h12-hardware-harness.py`: published/interim H12 contract guard passed after reconciliation;
- `test-h12-122-hardware.py`: **33 policy/unit assertions passed**;
- flash-manifest regression: **15 cases passed**;
- credential-output regression: **22 cases passed**;
- firmware-check regression: **9 cases passed**;
- partition policy regression: **9 cases passed**;
- production-config regression: **12 cases passed**;
- H9 production-audit regression: **7 cases passed**;
- setup-route isolation regression: **6 cases passed**;
- frontend persisted-state regression: **6 cases passed**;
- release-budget regression: **7 cases passed**;
- webfs-image regression: **9 cases passed**;
- stack-usage regression: **11 cases passed**;
- native-coverage policy regression passed;
- setup-label regression passed;
- route/dispatch synchronization passed;
- v2 limits/settings/device-settings/setup-route/API/auth policies passed;
- H2/H3/H9 architecture policies passed;
- spec traceability check passed after regeneration;
- npm-audit policy regression passed;
- secret-sentinel regression passed; and
- CI-status publishing unit tests: **7 passed**.

The sandbox has the cJSON 1.7.18 runtime but not its development header/pkg-config
metadata. For the three complete host-suite runs above, an external, uncommitted
sandbox-only cJSON 1.7.18 header/pkg-config shim was used solely to compile
against that installed runtime. No repository source, dependency lock, warning
policy, sanitizer setting, or test behavior was changed for that workaround.

The complete `check-scripts.sh` wrapper cannot execute in this sandbox because
its required external lint tools (including `actionlint`, `shellcheck`, `shfmt`,
and the real clang tooling) are not installed. The repository also requires Node
24.18.0 while this sandbox has Node 22.16.0, and ESP-IDF 5.5.5 is not installed
here for a real firmware build. Those requirements were not bypassed or
weakened. The applicable script regressions and Python/shell syntax checks were
run individually instead.

## Required closure sequence

H12-122 remains open. The next acceptable release sequence is:

1. publish these repairs as a replacement candidate SHA;
2. repeat H12-120 from a genuinely clean checkout of that exact SHA;
3. repeat all H12-121 authoritative software gates on that same exact SHA;
4. build production firmware/web assets/webfs/manifest from that exact clean SHA;
5. run the reference board with the two physically distinct USB connections
   required by the HIL harness; and
6. execute:

```bash
python3 scripts/run-h12-122-hardware.py \
  --flash-manifest firmware/build/flash-manifest.json \
  --firmware-sha <exact-40-character-git-sha> \
  --flash-port /dev/ttyACM0 \
  --console /dev/ttyACM1 \
  --output docs/implementation-v2/hardware/H12_122_<sha>.json
```

The actual device names must match the reference bench: the flash/native-USB port
and UART-console bridge are different physical USB functions. Record the board,
host, exact SHA, ports, complete non-secret command output, and generated H12 JSON
for sign-off.

Only a successful physical run on the exact H12-120/H12-121 candidate may check
the H12-122 boxes. A build-only CI job, dry-run manifest validation, prior-board
evidence, or host simulation is not a substitute.
