# Code-review fixes validation failure

**Stage:** `deterministic source transform`

**Exit status:** `1`

The production changes were not published. The one-shot workflow and verified payload remain on `master` for deterministic correction.

## Log tail

```text
Traceback (most recent call last):
  File "/tmp/apply-code-review-fixes.py", line 963, in <module>
    main()
  File "/tmp/apply-code-review-fixes.py", line 954, in main
    update_procedure_authority()
  File "/tmp/apply-code-review-fixes.py", line 34, in update_procedure_authority
    replace_once(
  File "/tmp/apply-code-review-fixes.py", line 22, in replace_once
    content = read(path)
              ^^^^^^^^^^
  File "/tmp/apply-code-review-fixes.py", line 12, in read
    return (ROOT / path).read_text(encoding="utf-8")
           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/usr/lib/python3.12/pathlib.py", line 1029, in read_text
    with self.open(mode='r', encoding=encoding, errors=errors) as f:
         ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/usr/lib/python3.12/pathlib.py", line 1015, in open
    return io.open(self, mode, buffering, encoding, errors, newline)
           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
FileNotFoundError: [Errno 2] No such file or directory: '/firmware/components/web_server/web_api_procedures.c'
HEAD is now at f17713c ci(code-review-fixes): validate fixes once

```
