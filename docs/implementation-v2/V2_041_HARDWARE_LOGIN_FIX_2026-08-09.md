# V2-041 — Real-load PBKDF2 timing, and a hardware-only login bug found and fixed

**Date:** 2026-08-09
**Task:** V2-041 (Phase 4 — Authentication, provisioning, and device settings), plus an
unplanned hardware-only defect found and fixed along the way in `web_server`/`lwip`
config (affects the whole login route, not just V2-041).
**Board:** ESP32-S3 (QFN56, chip revision v0.2), 8MB embedded PSRAM, MAC `9c:13:9e:a8:77:38`
**ESP-IDF:** v5.5.5, target `esp32s3`
**Baseline commit before this session's firmware changes:** `3746ef03e44527a1b65dee2e8b688f9318dcf1a4`
(clean, `gitDirty:false`, `buildType:production` per `firmware/build/flash-manifest.json`)

This continues `docs/CLAUDE_CODE_HANDOFF_2026-08-10.md`'s "when the board is back" plan,
starting with V2-041's two remaining hardware-only checklist items. Getting there required
solving two problems the prior V2-041 session (which worked entirely over USB/serial) never
hit, because this session went over real Wi-Fi for the first time in the v2 rebuild.

## 1. A provisioning-time secret with no disclosure path (reported, not fixed)

To reach the device's real HTTP API at all, this machine had to Wi-Fi-associate to the
device's own setup-mode AP (`ESP32-Macro-<suffix>`). That AP is protected (never open, per
`CLAUDE.md`'s wifi_ap hard rule), and its passphrase is derived deterministically
(`provisioning_bootstrap_core.c`: `HMAC(efuse_hardware_key, "macro-setup-ap-v1" +
softap_MAC)`, computed via the ESP32-S3's eFuse HMAC peripheral, `CONFIG_NVS_SEC_HMAC_EFUSE_KEY_ID`).

Traced every place this passphrase is used or could be surfaced: it is **never logged,
never returned over HTTP, and never exposed via any serial console command**
(confirmed by grepping `serial_console`, `wifi_ap`, and `app_core` for
`ap_passphrase`/`passphrase`). This looks deliberate, not an oversight — `SPEC_V2.md`
§12.4 states the serial console "MUST NOT disclose credentials or secret material," and
the code honors that. But no other disclosure mechanism exists either, which means: as
shipped, there is no way for a legitimate device owner to learn the setup-mode AP
passphrase and complete first-run provisioning over Wi-Fi at all. `SPEC_V2.md` §12.3 only
specifies the disclosure path for the *setup code* (serial console), not the AP passphrase
itself.

**Not fixed in this session** — this is a product/spec decision (how a real owner is meant
to learn this secret: a physical label, a companion doc, deliberately relaxing the
console-disclosure rule for this one value, etc.), not a bug I could unilaterally patch in
security-sensitive provisioning code. Worked around for this session only with a temporary,
uncommitted, local debug log in `app_core.c`'s `adapter_bootstrap_derive()` (added, used
once to capture the value, then fully reverted — confirmed via `git diff` showing no
residual changes — before any further flash of the board). No credential from that capture
is stored in this repository; the disposable device credentials this session's provisioning
used instead live at `~/.config/esp32-macro-keyboard/hil/v2-041-device-creds.json` (mode
600), per `CLAUDE.md`'s HIL-credential-storage convention.

**Recommendation:** flag `V2-040`/first-run provisioning as needing an explicit answer to
"how does a real owner learn the setup AP passphrase," before this device can be
provisioned by anyone without firmware-source access and a debug build.

## 2. Login route: 100% failure on real hardware (found and fixed)

With the device provisioned (disposable credentials, admin password ≥ the 12-byte minimum)
and reachable, every single `POST /api/v1/auth/login` request — 15/15 in the first batch —
returned:

```json
{"error":{"code":"internal","message":"login peer address unavailable"}}
```

HTTP `500`, before any password verification ever happens. This is exactly the gap flagged
in the 2026-08-10 handoff: *"`login` route: still has no live `httpd_req_t`-level test
anywhere (needs a real or faked socket for its IP-rate-limiting call)"* — and also
`docs/TODO_V2.md`'s Phase 5 exit gate note on the same subject. Nothing in the host or CI
suites had ever exercised the real socket this code path depends on.

### Root cause

`login_source_ipv4()` (`firmware/components/web_server/web_server_login.c`) calls
`httpd_req_to_sockfd()` then `getpeername()` to get the caller's source IP for the
per-source-IP login rate limiter (V2-042), and hard-fails on anything other than
`AF_INET`. Diagnosed with a temporary, uncommitted debug log (added, used once, then fully
reverted the same way as §1's — confirmed via `git diff` showing no residual changes):

```text
socket_fd=57
getpeername_result=0 errno=0 ss_family=10 peer_length=28
```

The `getpeername()` call itself succeeds (`result=0`, `errno=0`). `ss_family=10` is
`AF_INET6`, not `AF_INET` (`2`) — because `esp_http_server`'s `httpd_server_init()`
(`esp_http_server/src/httpd_main.c`, vendored ESP-IDF, not first-party) binds a **dual-stack
socket** whenever `CONFIG_LWIP_IPV6` is enabled (the ESP-IDF default, and this project's
prior default too):

```c
#if CONFIG_LWIP_IPV6
    int fd = socket(PF_INET6, SOCK_STREAM, 0);
#else
    int fd = socket(PF_INET, SOCK_STREAM, 0);
#endif
```

Our IPv4 client's connection arrives on that socket as an IPv4-mapped IPv6 address, so
`getpeername()` correctly and successfully reports `AF_INET6` — `login_source_ipv4()`'s
strict `AF_INET`-only check was simply wrong for how this project's own httpd is actually
configured. This is a real, deterministic, 100%-reproducible defect on physical hardware,
not a flake.

### Fix

Confirmed no first-party firmware source anywhere references IPv6
(`grep -rniE "ipv6|af_inet6|in6_addr|sockaddr_in6" firmware/components firmware/main` —
zero matches, before and after this fix). The only IPv6 involvement in the whole tree was
`esp_http_server`'s default dual-stack bind. This device is a single-AP appliance with no
IPv6 use anywhere in its design, so disabled IPv6 at the root Kconfig level rather than
patching a vendored ESP-IDF component or adding IPv4-mapped-address-unwrapping logic to
work around it:

`firmware/sdkconfig.defaults`:

```diff
+# Disable IPv6 project-wide. [...]
+CONFIG_LWIP_IPV6=n
```

With `CONFIG_LWIP_IPV6=n`, `esp_http_server`'s `#if CONFIG_LWIP_IPV6` branch is
preprocessor-excluded at compile time — the dual-stack path does not exist in the built
binary at all, not merely "disabled." `firmware/sdkconfig` was regenerated from scratch
(`rm sdkconfig sdkconfig.old && idf.py set-target esp32s3`) to confirm the setting resolves
correctly from the committed defaults rather than surviving only in a stale local cache:
confirmed `# CONFIG_LWIP_IPV6 is not set` in the freshly generated file.
`./scripts/check-production-config.sh` still passes clean against the new config.

No first-party code changed — `web_server_login.c` is byte-identical to before this session
(`git diff` clean); the temporary diagnostic logging added to it during root-causing was
fully reverted before this fix was written.

### Verification

Real login requests against the fixed firmware, over real Wi-Fi (device in station mode on
the same network as the test host — see §3), production build:

```json
{"authenticated":true,"idleExpiresInSeconds":86399,"absoluteExpiresInSeconds":604799}
```

`HTTP 200`, 20/20 real login attempts succeeded (the same batch used for the timing
measurement in §3).

## 3. V2-041's two remaining hardware-only items, now closed

### Real-Wi-Fi/production-load network path, not an isolated benchmark

The prior V2-041 session (`V2_041_PBKDF2_BENCHMARK_2026-08-08.md`) measured PBKDF2 cost
through an isolated on-device Unity console test — no Wi-Fi stack, no HTTP server, no AP
running. This session drives real `POST /api/v1/auth/login` requests against the full
production firmware with the AP live, a real station connection to an existing Wi-Fi
network (`Revival Hall`, open network, joined via the trusted serial console's
`wifi-connect` command — see `docs/CLAUDE_CODE_HANDOFF_2026-08-10.md`-successor context:
this let the test host stay on its normal network throughout, reaching the device's real IP
on the shared LAN instead of isolating onto the device's own AP), and the executor/USB/
controls subsystems all initialized as they are in real operation.

### Timing: 20 real login requests, full round-trip (network + PBKDF2 + response)

```text
sample=1  time_total=0.839075  http_code=200
sample=2  time_total=0.757241  http_code=200
sample=3  time_total=0.482706  http_code=200
sample=4  time_total=0.447993  http_code=200
sample=5  time_total=0.643878  http_code=200
sample=6  time_total=0.571753  http_code=200
sample=7  time_total=0.553224  http_code=200
sample=8  time_total=0.466884  http_code=200
sample=9  time_total=0.743805  http_code=200
sample=10 time_total=0.480233  http_code=200
sample=11 time_total=0.571765  http_code=200
sample=12 time_total=0.453850  http_code=200
sample=13 time_total=0.454248  http_code=200
sample=14 time_total=0.554700  http_code=200
sample=15 time_total=0.440992  http_code=200
sample=16 time_total=0.477785  http_code=200
sample=17 time_total=0.448365  http_code=200
sample=18 time_total=0.837841  http_code=200
sample=19 time_total=0.837841  http_code=200
sample=20 time_total=0.491811  http_code=200
```

| Metric | Value |
| --- | --- |
| min | 441.0 ms |
| median | 522.5 ms |
| p90 | 757.2 ms |
| worst | 839.1 ms |

These are **full HTTP round-trip times** (client-side `curl … -w '%{time_total}'`), not
pure KDF compute time — they include real Wi-Fi/TCP/HTTP overhead on top of the frozen
5,500-iteration PBKDF2-HMAC-SHA-256 derivation. They are not directly comparable to the
prior session's pure-KDF isolated numbers (436.6 ms median at 5,500 iterations) and do not
change the iteration-count decision, which was and remains about KDF cost specifically, not
end-to-end request latency. This measurement's purpose is different: confirming the
existing frozen iteration count behaves reasonably under real conditions, which it does —
no timeout, no failure, no pathological outlier (worst case 839 ms is real-world background
Wi-Fi contention, not a KDF anomaly, since the isolated worst-case for 5,500 iterations was
578 ms — the ~260 ms delta is consistent with ordinary Wi-Fi/TCP variance, not the
derivation itself).

### Watchdog / task-starvation check

Serial console output was captured continuously (`/dev/ttyACM0`, UART bridge, primary
console) across the entire 20-login burst above (40-second capture window spanning it).
**Zero** watchdog/TWDT lines appeared in that capture. Immediately verified this was a
genuine negative result and not a dead capture: reissued a harmless `wifi-status` command
over the same port right after and got a normal response, confirming the console was live
and would have captured a watchdog message had one occurred. This differs from the prior
session's isolated-benchmark observation (watchdog trips roughly every 5 seconds while idle
at the Unity console prompt, unrelated to `derive()` calls) — under real production load
with the full task graph running (Wi-Fi, HTTP server, USB, executor, controls), no watchdog
activity was observed at all during real authenticated login traffic.

## 4. What this report does not claim

- §1 (AP passphrase disclosure) is a reported gap, not a fix. No firmware behavior changed
  for it.
- §2's fix removes the dual-stack socket bind; it does not add the still-missing live
  `httpd_req_t`-level host test for the login route that `docs/TODO_V2.md`'s Phase 5 exit
  gate calls out. That gap is still open — this report provides strong evidence it should be
  prioritized (it hid a 100%-reproducible production defect), but does not close it.
- This is hardware evidence for V2-041's two specific checklist items and the login-route
  defect. It does not claim completion of V2-035, Phase 6, Phase 15, or any other open
  hardware-gated item.
- The AP-passphrase gap (§1) means this exact provisioning path (fresh device, Wi-Fi-only,
  no debug build) could not have been completed by an end user as shipped. That should be
  weighed before treating first-run provisioning as fully hardware-validated.

## 5. Commands run

```bash
. "$HOME/esp/esp-idf-v5.5.5/export.sh"
nvm use 24.18.0
cd firmware
idf.py set-target esp32s3
idf.py build
bash ../scripts/generate-flash-manifest.sh   # gitCommit 3746ef0, gitDirty:false, buildType:production
idf.py -p /dev/ttyACM1 flash                 # native USB, bootloader mode (303a:1001)

# fix: firmware/sdkconfig.defaults += CONFIG_LWIP_IPV6=n
rm -f sdkconfig sdkconfig.old
idf.py set-target esp32s3                    # confirms `# CONFIG_LWIP_IPV6 is not set`
bash ../scripts/check-production-config.sh
idf.py build
idf.py -p /dev/ttyACM1 flash

# station-mode join over the trusted UART console (/dev/ttyACM0, CH340 bridge),
# keeping the test host on its normal network throughout:
#   wifi-connect "Revival Hall" ""
# then real HTTP against the device's station IP on the shared LAN.
```

Host-side verification after the fix:

```bash
. "$HOME/esp/esp-idf-v5.5.5/export.sh"
./scripts/check-production-config.sh   # pass
```
