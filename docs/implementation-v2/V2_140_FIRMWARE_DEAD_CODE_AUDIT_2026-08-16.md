# V2-140 — firmware dead-code audit — 2026-08-16

The firmware half of V2-140 was never performed: the original pass was scoped to
the webapp, and `CLAUDE.md` has carried "the firmware-side half was never done —
`firmware/components/` may still contain unidentified v1-only paths" ever since.
This is that audit.

Audited at `735c67d`. Scope: everything compiled into the production image —
`firmware/components/**` and `firmware/main/**`, excluding `managed_components`
and `firmware/test_app`.

## 1. Files and build registrations

Every `.c` file on disk was compared against its component's
`idf_component_register(SRCS ...)` list across all 18 components.

**Result: no obsolete file, and no obsolete build registration.** Every
registered source exists, and every source on disk is registered — with exactly
two deliberate exceptions:

| File | Status |
| --- | --- |
| `web_server/web_setup_core.c` | on disk, **not** registered — not compiled into firmware |
| `web_server/web_setup_json.c` | on disk, **not** registered — not compiled into firmware |

Both carry an explicit `LEGACY / NOT SHIPPED` banner and are intentionally
retained (`7292ba3`); that decision stands and is not revisited here. They are
excluded from the reachability analysis below, since code that is not compiled
cannot be reached.

## 2. Reachability of compiled code

Every non-`static` function defined in compiled firmware (384 of them) was
checked for any mention in another compiled `.c` file. Mentions count, not just
calls, so a function used only as a pointer — `.handler = login_handler` — is
correctly treated as live. This matters: an earlier pass of this audit that
matched only `name(` produced 47 false positives, nearly all HTTP handlers.

**33 functions have no reference from any other compiled firmware source.** They
fall into four groups.

### 2a. Framework entry points — not dead

`app_main` and the TinyUSB weak callbacks (`tud_hid_descriptor_report_cb`,
`tud_hid_get_report_cb`, `tud_hid_set_report_cb`, `tud_resume_cb`,
`tud_suspend_cb`) are invoked by ESP-IDF and TinyUSB, never by first-party code.
Excluded from the count above.

### 2b. Superseded v1 provisioning store — the substantive finding

`provisioning.c`'s **entire public API is unreachable in shipped firmware**:

```text
provisioning_init            provisioning_deinit         provisioning_load
provisioning_commit          provisioning_settings_read  provisioning_settings_update
provisioning_get_station     provisioning_set_station    provisioning_clear_credentials
provisioning_factory_reset   provisioning_bootstrap_clear
```

The only apparent uses outside the component are in `web_setup_core.c/h`, and
they are **not** calls into this API — they are `web_setup_ops_t` *struct field
names* that happen to match (`operations->provisioning_commit`). That file is
also one of the two uncompiled legacy files above, so it cannot make anything
live.

`provisioning_core.c` is referenced only by `provisioning.c` and by host tests,
so it falls with it.

What *is* live in this component is the bootstrap half:
`provisioning_bootstrap_derive`, called by `app_core.c:82`.
`provisioning_bootstrap.c` includes only `provisioning_bootstrap_core.h` and has
no dependency on `provisioning.c`, so the dead half is cleanly separable.

This is genuinely unnoticed dead code rather than a deliberate keep: unlike
`web_setup_core.c`, neither `provisioning.c` nor `provisioning.h` carries a
retention banner. The v2 device-settings record (`device_settings` +
`app_contracts_v2/device_settings_v2.c`) is what actually stores settings now —
`web_server_setup.c` commits through `device_settings_replace`, not through this
API.

**Deleted** on the product owner's instruction, after this audit was reported:
`provisioning.c`, `provisioning_core.c/.h`, the dead half of `provisioning.h`,
the CMake `SRCS` entries, and the host suites whose subject no longer exists
(`test_provisioning.c`, `test_provisioning_settings.c`). `provisioning.h` keeps
only the two record shapes still named by the retained `LEGACY / NOT SHIPPED`
setup reference code.

### 2c. Test-only seams — intentional, not dead

Functions that exist so host tests can inject fakes, and are exercised by those
tests:

```text
storage_blob_upload_abort_with_ops     storage_blob_upload_commit_with_ops
web_api_json_parse_expected_revision   web_api_json_parse_settings_update
web_api_response_take_json             web_server_password_record_snapshot
macro_model_validate_revision          macro_model_validate_text
app_crc32                              app_lifecycle_health_record_degraded
app_uuid_parse                         app_uuid_equal
executor_shutdown_state_fault_latched  macro_compile
```

Two observations rather than actions:

- `macro_compile` is the **documented retained v1 parser entry point**
  (`macro_limits.h`: "Retained only for the legacy macro_compile entry point").
  It has no caller in compiled firmware; only host and device tests use it. It is
  a deletion candidate, but it is *documented* as retained, so it is listed here
  rather than removed.
- `macro_model_validate_revision` and `web_api_json_parse_expected_revision` use
  v1 "revision" vocabulary. Firmware no longer owns revisions, so these are
  likely v1 remnants kept alive only by their own tests.

### 2d. Unused accessors

Public accessors with no caller and no test:

```text
app_core_get_health                    device_controls_stack_high_water_mark
macro_executor_stack_high_water_mark   app_error_is_object_fault
storage_atomic_write                   wifi_ap_get_station_status
wifi_state_string                      web_api_handler_parser_error
web_api_handler_settings_json
```

The two `*_stack_high_water_mark` functions are worth a second look: stack
headroom is exactly the kind of thing diagnostics should report, and
`check-stack-usage.sh` ratchets *static* frame sizes only. If these were intended
to feed `/api/v1/diagnostics`, they are unwired rather than unwanted.

## 3. Compatibility and migration code

**None found.** Firmware does not migrate v1-shaped data; it *rejects* mismatched
versions:

- `device_settings_v2.c` returns an error when the stored record version is not
  `APP_V2_SETTINGS_RECORD_VERSION`, and likewise for credential and password
  algorithm versions;
- `provisioning_core.c` rejects a configuration whose `schema_version` is not
  `APP_SCHEMA_VERSION`.

That is strict version rejection, not a compatibility shim, which is what
V2-140's "no released v2 input" item asks for. The `/api/v1/...` route strings
throughout `api_routes_v2.h` are the v2 API's own version prefix, not v1
remnants.

## Conclusion

- **Files and build registrations: clean.** The checkbox's literal subject —
  obsolete files and registrations — found nothing to remove beyond the two
  already-known, deliberately-retained legacy files.
- **Compatibility/migration code: none exists.**
- **One substantive dead-code cluster** (the superseded v1 provisioning store)
  and two smaller v1 remnants (`macro_compile`, the "revision" validators) are
  identified with evidence and proposed for removal, pending a retention
  decision.

## Removal (2026-08-16)

Everything proposed above was removed on the product owner's instruction, plus
what fell out with it once the compiler had the last word.

| Removed | Why |
| --- | --- |
| `provisioning.c`, `provisioning_core.c/.h` | the superseded v1 NVS store; entire API unreachable |
| `web_api_json.c/.h` | **no live function at all** — `contains_embedded_nul_escape` looked live but is a `static` duplicated across six files |
| `web_api_handler_parser_error`, `web_api_handler_settings_json` | unreachable builders; `finish_json` and two includes fell with them |
| `macro_model_validate_revision` | v1 "revision" vocabulary, test-only |
| `macro_compile`, `macro_parser.c`, `macro_limits.h`, `macro_plan_free` | the retained v1 parser stack; production compiles only through `macro_compile_v2` (`web_send.c`) |

**Kept, against first appearances:** `macro_keymap_us.c/.h`. Deleting it broke
the build, because `macro_parser_v2.c` uses its `printable` and `modifier`
lookups — it is shared infrastructure, not a v1 remnant. Restored immediately.

**The host parser suite was migrated, not deleted.** Its fuzz corpus,
printable-ASCII sweep, named-key usages and error-location coverage are not in
the 21-case v2 conformance corpus, so deleting it with `macro_compile` would have
lost real coverage. It now runs against `macro_compile_v2`, and
`test_printable_ascii` asserts through the compiler rather than poking the keymap
table — a better test than the one it replaced.

Migrating surfaced two deliberate v1/v2 behaviour differences, now pinned by the
tests rather than lost with the old entry point:

- v2 accepts a **zero key-press time**; v1 rejected it.
- an over-long source reports the specific **`macro source exceeds the byte
  limit`** (`APP_ERROR_MACRO_LIMIT`) where v1 returned a generic
  `APP_ERROR_INVALID_ARGUMENT`.

**Result:** 24 files changed, **2,731 deletions** against 78 insertions. Host
test targets 66 → 63 (the three whose subjects no longer exist), all passing.
`check-all.sh` exit 0; application binary 47.8% of its budget, DIRAM 48.5%.
