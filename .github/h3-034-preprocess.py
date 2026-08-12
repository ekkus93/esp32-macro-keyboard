#!/usr/bin/env python3
from pathlib import Path

path = Path('/tmp/h3-034-apply.py')
text = path.read_text()

# Preserve C preprocessor continuation backslashes in the CHECK_MISSING macro
# literals. Ordinary Python triple-quoted strings consume backslash-newline
# pairs before the modifier ever sees the target C source.
replacements = (
    (
        'old = """        TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,',
        'old = r"""        TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,',
    ),
    (
        'new = """        const device_controls_reset_settings_outcome_t reset_outcome =',
        'new = r"""        const device_controls_reset_settings_outcome_t reset_outcome =',
    ),
)
for old, new in replacements:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f'expected one macro-literal anchor for {old!r}, found {count}')
    text = text.replace(old, new, 1)

# The reset-settings backend test contains C JSON string escapes. The original
# exact whole-function matcher was brittle, and its ordinary Python string
# literal also consumed the C backslashes. Extract the actual current section
# from the target file at modifier runtime and make the replacement C block a
# raw Python literal so \" survives into the generated C source.
old_anchor = 'old = """static void test_reset_settings_backend_failure(void) {'
new_anchor = 'new = """static void test_reset_settings_backend_failure(void) {'
raw_new_anchor = 'new = r"""static void test_reset_settings_backend_failure(void) {'
old_start = text.find(old_anchor)
new_start = text.find(new_anchor, old_start + 1)
if old_start < 0 or new_start < 0:
    raise SystemExit('web-actions reset failure modifier anchors are missing')

replacement = '''reset_failure_start = text.index("static void test_reset_settings_backend_failure(void) {")
reset_failure_end = text.index(
    "\\n/* ---------------------------------------------------------------------- */\\n"
    "/* POST /api/v1/device/factory-reset",
    reset_failure_start,
)
old = text[reset_failure_start:reset_failure_end]
'''
text = text[:old_start] + replacement + text[new_start:]
if text.count(new_anchor) != 1:
    raise SystemExit('expected one web-actions replacement literal after section rewrite')
text = text.replace(new_anchor, raw_new_anchor, 1)

path.write_text(text)
