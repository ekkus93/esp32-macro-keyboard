"""Fake `esp_idf_size` for the check-release-budgets.sh regression tests.

Emits the JSON in FAKE_ESP_IDF_SIZE_JSON (or a small default) regardless of
the real arguments, ignoring the .map file entirely. This lets
check-release-budgets.sh's threshold logic be exercised deterministically
without a real ESP-IDF build.
"""

import os
import sys

DEFAULT_JSON = (
    '{"diram_total": 341760, "used_diram": 113571}'
)

sys.stdout.write(os.environ.get("FAKE_ESP_IDF_SIZE_JSON", DEFAULT_JSON))
sys.stdout.write("\n")
