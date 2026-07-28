#!/usr/bin/env python3
"""Repair the generated cleanup wrapper quoting and execute it."""

from pathlib import Path
import runpy

ROOT = Path(__file__).resolve().parents[1]
WRAPPER = ROOT / "scripts/run-phase16-clang-tidy-cleanup.py"
text = WRAPPER.read_text(encoding="utf-8")
start_old = "response_test_write = '''write(\n"
start_new = 'response_test_write = """write(\n'
end_old = "\n'''\ntext = text[:response_test_start] + response_test_write + text[response_test_end:]\n"
end_new = '\n"""\ntext = text[:response_test_start] + response_test_write + text[response_test_end:]\n'
if text.count(start_old) != 1:
    raise SystemExit("Phase 16 wrapper response-test opening quote changed unexpectedly")
if text.count(end_old) != 1:
    raise SystemExit("Phase 16 wrapper response-test closing quote changed unexpectedly")
WRAPPER.write_text(text.replace(start_old, start_new, 1).replace(end_old, end_new, 1),
                   encoding="utf-8")
runpy.run_path(str(WRAPPER), run_name="__main__")
