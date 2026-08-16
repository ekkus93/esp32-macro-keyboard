# Desktop and tablet browser matrix — 2026-08-16

## Status

**PASS**, stable across three consecutive runs. Covers the Phase 13 items a
phone cannot answer, plus the two V2-130 viewport items and V2-133's focus
order.

| | |
| --- | --- |
| Browser | Real Chromium 151.0.7922.34 (headless) on this host |
| Device | ESP32-S3R8 at `192.168.88.108`, real firmware serving real assets |
| Harness | `tests/hardware/run-v2-desktop-matrix.mjs` |
| Command | `node tests/hardware/run-v2-desktop-matrix.mjs` |

Nothing joins the device SoftAP; the host reaches the device over the LAN.

## 1. Device classification (UI_UX_SPEC_V2 §12.4)

The block must fire for phone-like displays and must **not** broaden to tablet,
laptop, or desktop landscape. Each case flips exactly one term of
`(orientation: landscape) and (pointer: coarse) and (max-height: 600px)`, so a
case cannot pass for the wrong reason. Both the rendered result and the media
query's own value are asserted.

| Case | pointer | Media query | Blocked | Expected |
| --- | --- | --- | --- | --- |
| desktop 1920x1080 | fine | false | no | no |
| desktop 1280x800 | fine | false | no | no |
| **desktop short 1000x500** | fine | false | no | **no** — short enough to match `max-height`, but `pointer: fine`, so it must not block |
| tablet 1024x768 | coarse | false | no | no — coarse and landscape, but taller than 600px |
| tablet 1280x800 | coarse | false | no | no |
| phone 800x400 | coarse | true | **yes** | yes — control, all three terms true |
| phone 640x360 | coarse | true | **yes** | yes — control |

No horizontal page scroll and no page errors in any case.

The two controls are what make this meaningful: without them a broken query
that never matches would score five green rows. Real phone-hardware
confirmation is separate, in
`V2_155_ANDROID_UI_WORKFLOW_MATRIX_2026-08-16.md`.

## 2. Narrowest supported viewport — 320 CSS px (V2-130)

At 320x568: `scrollWidth` equals `clientWidth` (320), so there is **no
horizontal scroll**, and no element under `main` extends past the viewport's
right edge.

## 3. Single-column phone layout (V2-130)

At 360x640, no two children of `main` share a horizontal band while being
horizontally separated — i.e. nothing sits side by side, so the layout is a
single column. This is measured from geometry rather than inferred from the
presence of a media query.

## 4. Focus order (V2-133)

Measured on the **macro editor**, deliberately: it carries the directive, chord
and delay toolbars, so a sparse page cannot make this pass trivially. On the
default shell only 6 controls are focusable; the editor has **47**.

- every one of **47 of 47** focusable elements was reached by Tab;
- the tab sequence follows DOM order and never jumps backwards;
- there is no positive `tabindex` anywhere in the document.

**Honest limit:** this is a real-browser keyboard test, which is materially
stronger than the previous grep-level "no known violation", but it is **not** a
screen-reader pass. Phase 13's "Manual keyboard and screen-reader checks are
recorded" remains open, and V2-133's focus-order item is left unchecked for that
reason.

## Bench note — `ERR_NETWORK_CHANGED`

This host continuously creates and destroys Docker `veth` interfaces
(NetworkManager logs a steady stream of them). Chromium treats each as a network
change and aborts in-flight navigations with `net::ERR_NETWORK_CHANGED`. The
Wi-Fi link, the default route and the device are all healthy throughout —
verified during the run: device `401`, internet `200`, `wlo1` up.

The harness therefore retries navigation, and retries a whole case up to three
times from a fresh context. That is safe as evidence because each attempt starts
clean, so a genuine product failure fails every attempt; only environmental
aborts are absorbed.

Two harness bugs found while stabilising this, both worth recording because both
produced *confusing but green-looking* output:

- waiting for the sign-in field to become `detached` and swallowing the result
  with `.catch(() => {})` meant a timeout sampled the sign-in screen and reported
  "not blocked" — a classification failure that had nothing to do with
  classification. Now the wait asserts on rendered text and is not swallowed.
- sampling before the app reached a settled surface had the same effect. The
  harness now waits for `.app-shell` or `.landscape-block` to exist.

## Scope observation — the block covers the operational shell only

The landscape block wraps the authenticated app shell. The standalone screens —
Sign In, First-Run Setup, "Create Your First Repository", and the
unreachable/loading/reconnect surfaces — are **not** blocked in phone landscape.
Measured: at 800x400 with a coarse pointer the media query is `true` while the
Sign In screen renders normally.

This is recorded as an observation, not a defect. SPEC_V2 §12.1 says "the
**operational UI** is portrait-only" and V2-131's own checked item is worded
"Show Rotate your phone instead of ordinary **operational** content", so scoping
the block to the operational shell is a defensible reading. §12.2's broader
phrase "replaces ordinary screen content" could be read the other way, and
"Create Your First Repository" is the genuinely debatable case since building a
repository is operational work. Worth a ruling; not worth changing first-run and
sign-in behaviour unilaterally.
