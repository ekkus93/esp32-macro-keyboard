# Storage quality fixer failure

Apply outcome: failure
Apply status: 1
Quality outcome: skipped
Quality status: 

## Apply log

```text
Traceback (most recent call last):
  File "/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/scripts/fix-storage-quality-once.py", line 71, in <module>
    text = replace_once(text, old, new, path)
           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/scripts/fix-storage-quality-once.py", line 19, in replace_once
    raise RuntimeError(f'{path}: expected one occurrence, found {count}: {old[:100]!r}')
RuntimeError: firmware/components/storage/storage_repository_procedures.c: expected one occurrence, found 0: 'static app_error_code_t validate_macro_reference_locked(const app_uuid_t *set_id,\n                  '
```

## Authoritative-check log

```text
```
