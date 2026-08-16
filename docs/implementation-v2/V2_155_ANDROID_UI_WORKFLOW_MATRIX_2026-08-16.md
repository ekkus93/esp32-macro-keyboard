# V2-155 — Android UI workflow matrix — 2026-08-16

## Status

**PASS — all twelve workflows verified on a real Android phone against real
firmware.** This is the first time the shipped web application has been run on a
phone against a real device, and it found **two P0 defects that made the product
unusable**. Both were fixed before the matrix was completed; see "What this
found" below.

## Exact conditions

| | |
| --- | --- |
| Phone | LG G6 (LG-H872), Android 8.0.0, 1440x2880 at density 640 |
| Browser | Chrome 132.0.6834.164, viewport **360 x 588 CSS px**, devicePixelRatio 4 |
| Device | ESP32-S3R8, MAC `9c:13:9e:a8:77:38`, firmware `f089d5b` web assets |
| Network | Phone `192.168.88.107`, device `192.168.88.108`, both on the LAN |
| Harness | `tests/hardware/android_browser.mjs` (Playwright over CDP via `adb forward`) |
| HID capture | `tests/hardware/hid_capture.py` reading this host's `hidraw` |

**No SoftAP association was used and this host's Wi-Fi was never touched.** The
phone and the device are both station clients on the same LAN, so the phone
reaches the device directly. For the first-run workflow the device was returned
to the LAN over the trusted UART console while still unprovisioned, exactly as
the H12-122 harness does, rather than making the phone join the device's AP.

## Results

| # | Workflow | Result |
| --- | --- | --- |
| 1 | First-ever launch and setup | **PASS** — factory reset, setup code read from the trusted console, the whole first-run form completed *on the phone*; "Setup complete" returned (not a timeout), device provisioned, `/api/v1/setup` then `404` |
| 2 | Configured-device Sign In | **PASS** — after fixing the defect below |
| 3 | First sign-in from a new Android phone | **PASS** — Chrome data cleared to a first-run profile, then signed in to a freshly provisioned device |
| 4 | Already-authenticated refresh | **PASS** — reload kept the session, no sign-in prompt |
| 5 | Automatic newest-snapshot loading | **PASS** — reload auto-loaded blob 17 (2 macros), clean/not dirty |
| 6 | Manual loading of an older snapshot | **PASS** — loading blob 16 replaced the working copy with its 0-macro content |
| 7 | Quick Send while remaining on the Macros page | **PASS** — inline progress, route stayed `#/macros`, and **this host's HID captured exactly `zqxjvw`** |
| 8 | Inline acknowledgement and cancellation | **PASS** — "Cancel and release all keys" during flight; after cancel, "Send Bench Slow was cancelled." and HID shows exactly one report, `[0,0,0,0,0,0,0,0]`, with nothing typed |
| 9 | Hidden macro source | **PASS** — "Source hidden"; the source is absent from visible text, `aria-label`s, `title`s **and the DOM entirely** until Reveal |
| 10 | Dirty-state warnings and manual Save snapshot | **PASS** — "Unsaved changes" → Save snapshot → "Saved", device confirms the new blob |
| 11 | Manual snapshot deletion and advisory retention | **PASS** — "retention target 5" shown; delete requires typing the snapshot ID and Confirm stays disabled for a wrong ID; after confirming, the device confirms only blob 17 remains |
| 12 | Portrait enforcement and landscape cancellation | **PASS** — see below |

### Workflow 12 in detail

SPEC_V2 §14.7 makes phones "operationally portrait-only" but carves out one
safety exception: "An active send's progress and **Cancel and release all keys**
control MUST remain accessible from that surface."

Both halves were verified. Rotating to landscape (668 x 280) shows
*"Rotate your phone — ESP32 Macro Keyboard is designed for portrait mode."*
Rotating **during an active send** shows that same surface plus
*"Sending Bench Slow… action 0 of 7"* and the Cancel control — and tapping
Cancel there really did stop the send: HID captured one all-zero release report
and the macro's `mnbvc` was never typed.

## What this found

### Two P0 defects, fixed in `f089d5b`

Full analysis in the commit; in brief, each was invisible to all 543 frontend
tests for the same reason — **the tests do not run in the product's deployment
context**.

1. **Sign-in was impossible.** `isSessionStatus` required the session lifetimes
   to equal the configured maxima exactly, but firmware sends the *remaining*
   lifetime, so a real login response (`86399`/`604799`) was rejected as
   `invalid_response`. Unit tests always returned the exact maxima. A
   pre-existing test asserted the bug.
2. **The first screen that mints an ID crashed.** `crypto.randomUUID` is
   secure-context-only and the device serves plain HTTP over a LAN address;
   measured on the phone, `isSecureContext === false` and both
   `crypto.randomUUID` and `crypto.subtle` are `undefined`. "Create Your First
   Repository" died with a blank page. Browser tests run on `localhost`, which
   *is* a secure context.

### One finding recorded, not fixed

**The sticky bottom navigation does overlap content.** V2-130's checked item
"Ensure bottom navigation does not cover final actions" reasons that a sticky
element "can only sit below `main`'s content box, never overlap it". That
reasoning is wrong, and this run measured a counter-example: on the Snapshots
page at `scrollY 36.75`, the nav occupied CSS y 519-588 while "Load snapshot 17"
occupied 542-588 — **entirely covered**, with `elementFromPoint` at the button's
centre returning the nav.

**Follow-up measurement settles the severity: this is not a functional defect.**
Scrolling to the very bottom of the same page (`scrollY == maxScroll == 367`)
leaves **no** control covered by the nav — `covered: []`. The overlap is
transient, occurring only at intermediate scroll positions, and no action is ever
trapped behind the navigation. What is wrong is only the *justification* recorded
in `TODO_V2.md`: a sticky element does overlap content scrolled under it, so
"can only sit below `main`'s content box, never overlap it" is not why the
requirement holds. The requirement holds because the page scrolls far enough to
clear the nav.

The practical consequence is for automation, not users: a tap computed at a
control's centre can land on the nav, which is why this harness centres elements
before tapping.
Recorded here rather than silently fixed, since V2-130 is a checked item and
re-deciding it is the product owner's call.

### One non-defect worth recording

`{DELAY:20000}` is rejected with "delay is outside the allowed range" and Save
stays disabled — correct, `delayDirectiveMaxMs` is 10,000. The workflow used two
chained 10 s delays instead. Noted because it looked like a save failure at
first and is not one.

## Reproducing

Chrome must be running on the phone with a page open on the device, then:

```bash
adb -s <serial> forward tcp:9222 localabstract:chrome_devtools_remote
```

`tests/hardware/android_browser.mjs` provides `attach()`, a calibrated `tap()`
that issues a real `adb shell input tap`, `hideKeyboard()`, and
`setOrientation()`. Its header documents the two non-obvious parts: Playwright's
own `click()` does not land reliably at devicePixelRatio 4, and BACK must only be
sent when the IME is actually open or it dismisses whatever panel is showing.

Bench credentials were regenerated during workflow 1 and stored in
`~/.config/esp32-macro-keyboard/hil/` (dir 700, files 600). No credential,
passphrase, or setup code appears in this document or in the repository.
