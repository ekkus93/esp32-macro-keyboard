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
9:zlib.error: Error -3 while decompressing data: invalid code -- missing end-of-block

Phase 17 backend session foundation applied
Traceback (most recent call last):
  File "/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/scripts/apply-phase17-foundation.py", line 71, in <module>
    source = gzip.decompress(decode_chunks(module_name)).decode("utf-8")
             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/usr/lib/python3.12/gzip.py", line 632, in decompress
    decompressed = do.decompress(data[fp.tell():])
                   ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
zlib.error: Error -3 while decompressing data: invalid code -- missing end-of-block
```
