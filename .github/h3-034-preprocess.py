#!/usr/bin/env python3
from pathlib import Path

path = Path('/tmp/h3-034-apply.py')
text = path.read_text()

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

path.write_text(text)
