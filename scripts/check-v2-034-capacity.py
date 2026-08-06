#!/usr/bin/env python3
"""Fail-closed V2-034 LittleFS capacity and HTTP contract proof."""

from __future__ import annotations

import csv
import hashlib
import tempfile
from pathlib import Path

from littlefs import LittleFS

REPO_ROOT = Path(__file__).resolve().parents[1]
BLOCK_SIZE = 4096
NAME_MAX = 64
EXPECTED_CANDIDATE_BYTES = 128 * 1024
REQUIRED_MARGIN_BLOCKS = 1


def read_partition_size(name: str) -> int:
    path = REPO_ROOT / "firmware" / "partitions.csv"
    with path.open(encoding="utf-8") as handle:
        rows = (
            line
            for line in handle
            if line.strip() and not line.lstrip().startswith("#")
        )
        for row in csv.reader(rows):
            if len(row) == 6 and row[0].strip() == name:
                return int(row[4].strip(), 0)
    raise SystemExit(f"partition table has no {name} partition")


def require_once(text: str, needle: str, description: str) -> None:
    if text.count(needle) != 1:
        raise SystemExit(f"{description} missing or duplicated")


def verify_reporting_contract(maximum: int) -> None:
    status_types = (REPO_ROOT / "webapp/src/v2/apiTypes.ts").read_text(
        encoding="utf-8"
    )
    status_guards = (REPO_ROOT / "webapp/src/v2/apiGuards.ts").read_text(
        encoding="utf-8"
    )
    limits_json = (
        REPO_ROOT / "firmware/components/web_server/web_server_adapter_json.c"
    ).read_text(encoding="utf-8")
    blob_list = (
        REPO_ROOT / "firmware/components/web_server/web_server_blob.c"
    ).read_text(encoding="utf-8")

    for field in ("totalBytes", "usedBytes", "remainingBytes"):
        require_once(
            status_types,
            f"    {field}: number;",
            f"status storage field {field}",
        )
        if f'"{field}"' not in status_guards:
            raise SystemExit(f"status guard does not require {field}")

    require_once(
        limits_json,
        r'\"blobMaxBytes\":%lu',
        "limits endpoint blobMaxBytes field",
    )
    if "(unsigned long)APP_V2_BLOB_MAX_BYTES" not in limits_json:
        raise SystemExit("limits endpoint is not sourced from APP_V2_BLOB_MAX_BYTES")

    # The authoritative blob-list schema remains intentionally unchanged:
    # final payload bytes plus actual filesystem remaining bytes.
    for field in ('"usedBytes"', '"remainingBytes"', '"blobs"'):
        if field not in blob_list:
            raise SystemExit(f"blob list response lost {field}")
    for forbidden in ('"totalBytes"', '"maxBlobBytes"'):
        if forbidden in blob_list:
            raise SystemExit(
                f"blob list contract was silently expanded with {forbidden}"
            )

    if maximum != EXPECTED_CANDIDATE_BYTES:
        raise SystemExit(
            f"candidate changed from {EXPECTED_CANDIDATE_BYTES} to {maximum}; "
            "update the specification and V2-034 evidence intentionally"
        )


def verify_http_contract() -> None:
    blob_source = (
        REPO_ROOT / "firmware/components/web_server/web_server_blob.c"
    ).read_text(encoding="utf-8")
    api_source = (
        REPO_ROOT / "firmware/components/web_server/web_api_core.c"
    ).read_text(encoding="utf-8")
    upload_source = (
        REPO_ROOT / "firmware/components/storage/storage_blob_upload_core.c"
    ).read_text(encoding="utf-8")

    create_handler = blob_source.index("esp_err_t blob_create_handler")
    limit_check = "content_length > (size_t)APP_V2_BLOB_MAX_BYTES"
    authentication = "result = authenticate_blob_request(request);"
    upload_begin = "result = storage_blob_upload_begin(content_length, &upload);"
    limit_position = blob_source.index(limit_check, create_handler)
    auth_position = blob_source.index(authentication, create_handler)
    begin_position = blob_source.index(upload_begin, create_handler)
    if not create_handler < limit_position < auth_position < begin_position:
        raise SystemExit(
            "oversize rejection must precede authentication and storage mutation"
        )
    if "WEB_HTTP_STATUS_PAYLOAD_TOO_LARGE" not in blob_source[
        limit_position:auth_position
    ]:
        raise SystemExit("oversize blob request does not produce HTTP 413")

    if (
        "case APP_ERROR_LIMIT:" not in api_source
        or "WEB_HTTP_STATUS_PAYLOAD_TOO_LARGE" not in api_source
    ):
        raise SystemExit("APP_ERROR_LIMIT must map to HTTP 413")
    if (
        "case APP_ERROR_STORAGE_FULL:" not in api_source
        or "WEB_HTTP_STATUS_INSUFFICIENT_STORAGE" not in api_source
    ):
        raise SystemExit("APP_ERROR_STORAGE_FULL must map to HTTP 507")

    for error_name in ("ENOSPC", "EDQUOT", "EFBIG"):
        if error_name not in upload_source:
            raise SystemExit(f"storage exhaustion mapping is missing {error_name}")
    if "APP_ERROR_STORAGE_FULL" not in upload_source:
        raise SystemExit(
            "storage exhaustion is not surfaced as APP_ERROR_STORAGE_FULL"
        )


def build_and_verify_image(
    partition_bytes: int, maximum: int
) -> tuple[int, int, int]:
    if partition_bytes <= 0 or partition_bytes % BLOCK_SIZE != 0:
        raise SystemExit(
            "userdata partition must be a positive whole number of LittleFS blocks"
        )

    filesystem = LittleFS(
        block_size=BLOCK_SIZE,
        block_count=partition_bytes // BLOCK_SIZE,
        name_max=NAME_MAX,
    )
    filesystem.mkdir("/repository")
    filenames = (
        "00000000000000000001.gz",
        "00000000000000000002.gz",
        "00000000000000000003.gz.tmp",
    )
    expected_hashes: dict[str, str] = {}
    for seed, filename in enumerate(filenames, start=1):
        payload = bytes((offset + seed) % 251 for offset in range(maximum))
        expected_hashes[filename] = hashlib.sha256(payload).hexdigest()
        with filesystem.open(f"/repository/{filename}", "wb") as handle:
            written = handle.write(payload)
        if written != maximum:
            raise SystemExit(
                f"short LittleFS image write for {filename}: {written}"
            )

    used_blocks = filesystem.used_block_count
    used_bytes = used_blocks * BLOCK_SIZE
    payload_bytes = maximum * len(filenames)
    overhead_bytes = used_bytes - payload_bytes
    remaining_bytes = partition_bytes - used_bytes
    if overhead_bytes <= 0:
        raise SystemExit(
            "capacity proof did not account for positive filesystem overhead"
        )
    if remaining_bytes < REQUIRED_MARGIN_BLOCKS * BLOCK_SIZE:
        raise SystemExit(
            f"candidate leaves only {remaining_bytes} bytes; "
            "at least one block is required"
        )

    for filename, expected_hash in expected_hashes.items():
        with filesystem.open(f"/repository/{filename}", "rb") as handle:
            stored = handle.read()
        if len(stored) != maximum:
            raise SystemExit(
                f"stored size mismatch for {filename}: {len(stored)}"
            )
        if hashlib.sha256(stored).hexdigest() != expected_hash:
            raise SystemExit(f"byte identity mismatch for {filename}")

    with tempfile.TemporaryDirectory(prefix="v2-034-") as temporary:
        image_path = Path(temporary) / "userdata.bin"
        image_path.write_bytes(bytes(filesystem.context.buffer))
        if image_path.stat().st_size != partition_bytes:
            raise SystemExit(
                "generated image size does not equal the userdata partition"
            )

    return used_bytes, overhead_bytes, remaining_bytes


def main() -> int:
    partition_bytes = read_partition_size("userdata")
    app_limits_text = (
        REPO_ROOT
        / "firmware/components/app_contracts_v2/include/app_limits_v2.h"
    ).read_text(encoding="utf-8")
    if "#define APP_V2_BLOB_MAX_BYTES (128u * 1024u)" not in app_limits_text:
        raise SystemExit(
            "APP_V2_BLOB_MAX_BYTES is not the reviewed 128 KiB candidate"
        )
    maximum = EXPECTED_CANDIDATE_BYTES

    verify_reporting_contract(maximum)
    verify_http_contract()
    used_bytes, overhead_bytes, remaining_bytes = build_and_verify_image(
        partition_bytes, maximum
    )
    print(
        "V2-034 LittleFS capacity passed: "
        f"partition={partition_bytes} maxBlob={maximum} "
        f"payload={maximum * 3} used={used_bytes} "
        f"overhead={overhead_bytes} remaining={remaining_bytes}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
