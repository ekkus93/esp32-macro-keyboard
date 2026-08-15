"""Constants, the evidence error type, and the assertion helper."""

from __future__ import annotations

import re

BLOB_MAX_BYTES = 131_072
USERDATA_BYTES = 524_288
STATE_SCHEMA = 3
ESP_IDF_VERSION = "ESP-IDF v5.5.5"
FIRMWARE_COMMIT_PATTERN = re.compile(r"^[0-9a-f]{40}$")
SHA256_PATTERN = re.compile(r"^[0-9a-f]{64}$")
BUILD_ID_PATTERN = re.compile(r"^[0-9a-f]{39}$")
# The live board's diagnostics.buildId is a much shorter prefix than the flash
# manifest's 39-character diagnosticsBuildId: esp_app_get_elf_sha256() only
# returns CONFIG_APP_RETRIEVE_LEN_ELF_SHA hex characters (9 on this project's
# current sdkconfig), not the full 39-character prefix. It's still the same
# hex prefix of the same ELF SHA-256, just shorter - accept any non-empty hex
# string up to 39 characters rather than requiring an exact 39-character match.
LIVE_BUILD_ID_PATTERN = re.compile(r"^[0-9a-f]{1,39}$")
ELF_SHA_OUTPUT_PATTERN = re.compile(r"ELF file SHA256:\s*([0-9A-Fa-f]{64})")
AUTH_LOGIN_PATH = "/api/v1/auth/login"
BLOB_COLLECTION_PATH = "/api/v1/blob"
DIAGNOSTICS_PATH = "/api/v1/diagnostics"
REQUIRED_SCENARIOS = (
    "power_cycle_persistence",
    "numeric_ordering",
    "delete_preservation",
    "interrupted_upload_no_partial_final",
    "reboot_temporary_cleanup",
    "storage_full_507_preservation",
    "mount_failure_no_format",
)


class EvidenceError(RuntimeError):
    """Raised when a hardware observation fails a required invariant."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise EvidenceError(message)
