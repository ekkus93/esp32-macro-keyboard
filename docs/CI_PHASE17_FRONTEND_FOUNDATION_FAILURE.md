# Phase 17 frontend foundation failure

- Transform: failure
- Cleanup: skipped
- Node setup: skipped
- Frontend dependencies: skipped
- Format: skipped
- Frontend validation: skipped
- Host dependencies: skipped
- Firmware formatting/tests: skipped
- ESP-IDF install: skipped
- Authoritative gate: skipped

## phase17-foundation-transform.log

```text
11:binascii.Error: Excess data after padding

Traceback (most recent call last):
  File "/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/scripts/apply-phase17-foundation.py", line 34, in <module>
    source = gzip.decompress(decode_chunks(module_name)).decode("utf-8")
                             ^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/scripts/apply-phase17-foundation.py", line 22, in decode_chunks
    return base64.b64decode(encoded, validate=True)
           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/usr/lib/python3.12/base64.py", line 88, in b64decode
    return binascii.a2b_base64(s, strict_mode=validate)
           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
binascii.Error: Excess data after padding
```
