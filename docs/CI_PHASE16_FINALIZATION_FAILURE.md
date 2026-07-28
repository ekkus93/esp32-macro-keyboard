# Phase 16 finalization failure

- Transform: failure
- Cleanup: skipped
- Format: skipped
- Focused tests: skipped
- Authoritative gate: skipped
- Coverage/device evidence: skipped

## phase16-finalization-transform.log

```text
6:    raise RuntimeError(f"{path}: expected one match, found {count}: {old!r}")
7:RuntimeError: firmware/components/storage/storage_repository_procedures.c: expected one match, found 0: 'static app_error_code_t procedure_list_locked(const app_uuid_t *set_id,\n                                               storage_procedure_list_t *out_list) {\n'

Phase 16 request JSON hardening transform applied
Traceback (most recent call last):
  File "/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/scripts/apply-phase16-finalization.py", line 539, in <module>
    replace_once(
  File "/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/scripts/apply-phase16-finalization.py", line 24, in replace_once
    raise RuntimeError(f"{path}: expected one match, found {count}: {old!r}")
RuntimeError: firmware/components/storage/storage_repository_procedures.c: expected one match, found 0: 'static app_error_code_t procedure_list_locked(const app_uuid_t *set_id,\n                                               storage_procedure_list_t *out_list) {\n'
```
