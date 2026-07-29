# Phase 17.5 macro editor failure

- Transform: failure
- Cleanup: skipped
- Node setup: skipped
- Frontend dependencies: skipped
- Format: skipped
- Frontend validation: skipped
- Host dependencies: skipped
- ESP-IDF install: skipped
- Authoritative gate: skipped

## phase17-5-macro-editor-transform.log

```text
7:    with self.open(mode='r', encoding=encoding, errors=errors) as f:
10:    return io.open(self, mode, buffering, encoding, errors, newline)
12:FileNotFoundError: [Errno 2] No such file or directory: '/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/scripts/webapp/src/types/models.ts'

Traceback (most recent call last):
  File "/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/scripts/apply-phase17-5-macro-editor.py", line 32, in <module>
    exec(
  File "/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/scripts/phase17-5-macro-editor/payload.py", line 1291, in <module>
  File "/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/scripts/phase17-5-macro-editor/payload.py", line 12, in replace_once
  File "/usr/lib/python3.12/pathlib.py", line 1029, in read_text
    with self.open(mode='r', encoding=encoding, errors=errors) as f:
         ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/usr/lib/python3.12/pathlib.py", line 1015, in open
    return io.open(self, mode, buffering, encoding, errors, newline)
           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
FileNotFoundError: [Errno 2] No such file or directory: '/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/scripts/webapp/src/types/models.ts'
```
