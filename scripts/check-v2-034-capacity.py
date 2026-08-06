#!/usr/bin/env python3
"""Fail-closed V2-034 LittleFS capacity and HTTP contract proof."""

from __future__ import annotations

import csv
import hashlib
import tempfile
from importlib.metadata import PackageNotFoundError, version
from pathlib import Path

try:
    from littlefs import LittleFS
except ModuleNotFoundError as error:
    raise SystemExit(
        "littlefs-python==0.15.0 is required for the V2-034 capacity gate"
    ) from error

REPO_ROOT = Path(__file__).resolve().parents[1]
BLOCK_SIZE = 4096
NAME_MAX = 64
EXPECTED_LITTLEFS_VERSION = "0.15.0"
EXPECTED_PARTITION_BYTES = 524_288
EXPECTED_CANDIDATE_BYTES = 131_072
EXPECTED_PAYLOAD_BYTES = 393_216
EXPECTED_USED_BYTES = 421_888
EXPECTED_OVERHEAD_BYTES = 28_672
EXPECTED_REMAINING_BYTES = 102_400
REQUIRED_MARGIN_BLOCKS = 1


def read_text(relative_path: str) -> str:
    return (REPO_ROOT / relative_path).read_text(encoding="utf-8")


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


def require_contains(text: str, needle: str, description: str) -> None:
    if needle not in text:
        raise SystemExit(f"{description} missing")


def section(text: str, start: str, end: str, description: str) -> str:
    start_position = text.find(start)
    if start_position < 0:
        raise SystemExit(f"{description} start marker missing")
    end_position = text.find(end, start_position + len(start))
    if end_position < 0:
        raise SystemExit(f"{description} end marker missing")
    return text[start_position:end_position]


def normalized(text: str) -> str:
    return " ".join(text.split())


def verify_dependency_version() -> None:
    try:
        installed = version("littlefs-python")
    except PackageNotFoundError as error:
        raise SystemExit(
            "littlefs-python==0.15.0 is required for the V2-034 capacity gate"
        ) from error
    if installed != EXPECTED_LITTLEFS_VERSION:
        raise SystemExit(
            "V2-034 capacity evidence requires littlefs-python=="
            f"{EXPECTED_LITTLEFS_VERSION}; found {installed}"
        )


def verify_limit_constant() -> int:
    limits_header = read_text(
        "firmware/components/app_contracts_v2/include/app_limits_v2.h"
    )
    require_once(
        limits_header,
        "#define APP_V2_BLOB_MAX_BYTES UINT32_C(131072)",
        "reviewed APP_V2_BLOB_MAX_BYTES constant",
    )
    return EXPECTED_CANDIDATE_BYTES


def verify_reporting_contract(maximum: int) -> None:
    status_types = read_text("webapp/src/v2/apiTypes.ts")
    status_guards = read_text("webapp/src/v2/apiGuards.ts")
    limits_json = read_text(
        "firmware/components/web_server/web_server_adapter_json.c"
    )
    blob_list = read_text("firmware/components/web_server/web_server_blob.c")

    for field in ("totalBytes", "usedBytes", "remainingBytes"):
        require_once(
            status_types,
            f"    {field}: number;",
            f"status storage field {field}",
        )
        require_contains(
            status_guards,
            f'"{field}"',
            f"status response guard field {field}",
        )

    require_once(
        status_types,
        "  blobMaxBytes: number;",
        "limits response blobMaxBytes field",
    )
    require_once(
        limits_json,
        r'\"blobMaxBytes\":%lu',
        "limits endpoint blobMaxBytes field",
    )
    require_contains(
        limits_json,
        "(unsigned long)APP_V2_BLOB_MAX_BYTES",
        "limits endpoint authoritative blob maximum",
    )

    blob_response = section(
        status_types,
        "export interface BlobListResponse",
        "export type BlobCreatedResponse",
        "TypeScript blob-list response",
    )
    for field in ("blobs", "usedBytes", "remainingBytes"):
        require_contains(blob_response, field, f"blob-list field {field}")
    for forbidden in ("totalBytes", "maxBlobBytes", "blobMaxBytes"):
        if forbidden in blob_response:
            raise SystemExit(
                f"blob-list TypeScript contract was silently expanded with {forbidden}"
            )

    for field in ('"usedBytes"', '"remainingBytes"', '"blobs"'):
        require_contains(blob_list, field, f"firmware blob-list field {field}")
    for forbidden in ('"totalBytes"', '"maxBlobBytes"', '"blobMaxBytes"'):
        if forbidden in blob_list:
            raise SystemExit(
                f"firmware blob-list contract was silently expanded with {forbidden}"
            )

    if maximum != EXPECTED_CANDIDATE_BYTES:
        raise SystemExit(
            f"candidate changed from {EXPECTED_CANDIDATE_BYTES} to {maximum}; "
            "update the specification and V2-034 evidence intentionally"
        )


def verify_http_contract() -> None:
    blob_source = read_text("firmware/components/web_server/web_server_blob.c")
    api_source = read_text("firmware/components/web_server/web_api_core.c")
    upload_source = read_text(
        "firmware/components/storage/storage_blob_upload_core.c"
    )
    upload_test = read_text("tests/host/test_storage_blob_upload.c")
    api_test = read_text("tests/host/test_web_api_core.c")

    create_handler = section(
        blob_source,
        "esp_err_t blob_create_handler",
        "esp_err_t blob_load_handler",
        "blob create handler",
    )
    required_order = (
        "const size_t content_length = request->content_len;",
        "if (content_length > (size_t)APP_V2_BLOB_MAX_BYTES)",
        "WEB_HTTP_STATUS_PAYLOAD_TOO_LARGE",
        "web_server_get_header(request, \"Content-Type\"",
        "result = authenticate_blob_request(request);",
        "result = storage_blob_upload_begin(content_length, &upload);",
    )
    previous = -1
    for marker in required_order:
        position = create_handler.find(marker)
        if position < 0:
            raise SystemExit(f"blob create handler contract marker missing: {marker}")
        if position <= previous:
            raise SystemExit(
                "oversize rejection must precede content-type parsing, "
                "authentication, and upload initialization"
            )
        previous = position

    for root_name in ("firmware", "tests"):
        for path in sorted((REPO_ROOT / root_name).rglob("*.[ch]")):
            if "APP_ERROR_LIMIT" in path.read_text(encoding="utf-8"):
                relative = path.relative_to(REPO_ROOT)
                raise SystemExit(
                    f"failed APP_ERROR_LIMIT attempt remains in {relative}"
                )

    require_contains(
        api_source,
        "case APP_ERROR_STORAGE_FULL:",
        "APP_ERROR_STORAGE_FULL HTTP mapping",
    )
    require_contains(
        api_source,
        "return WEB_HTTP_STATUS_INSUFFICIENT_STORAGE;",
        "HTTP 507 mapping",
    )
    require_contains(
        api_test,
        "TEST_CHECK_EQ_U64(507U, web_api_http_status_for_error(APP_ERROR_STORAGE_FULL));",
        "host test for APP_ERROR_STORAGE_FULL to HTTP 507",
    )

    require_contains(
        normalized(upload_source),
        normalized(
            "return error_number == ENOSPC ? APP_ERROR_STORAGE_FULL : APP_ERROR_IO;"
        ),
        "ENOSPC storage-full mapping",
    )
    require_contains(
        normalized(upload_source),
        normalized(
            "operations->open_temporary(operations->context, "
            "out_upload->temporary_path)"
        ),
        "temporary-path-only upload open",
    )
    require_contains(
        normalized(upload_source),
        normalized(
            "operations->rename_path(operations->context, upload->temporary_path, "
            "upload->final_path)"
        ),
        "temporary-to-final rename commit point",
    )
    require_contains(
        normalized(upload_source),
        normalized(
            "const app_error_code_t cleanup = "
            "unlink_temporary(operations, upload->temporary_path);"
        ),
        "abort cleanup restricted to the temporary path",
    )

    unlink_fake = section(
        upload_test,
        "static int fake_unlink_path",
        "static int fake_sync_parent",
        "host fake unlink operation",
    )
    require_contains(
        unlink_fake,
        "TEST_CHECK(path_is_temporary(path));",
        "host guard forbidding final-path unlink",
    )
    enospc_test = section(
        upload_test,
        "static void test_write_failures_abort_cleanly",
        "static void run_precommit_failure",
        "ENOSPC upload host test",
    )
    for marker, description in (
        ("fake.fail_errno = ENOSPC;", "ENOSPC injection"),
        ("APP_ERROR_STORAGE_FULL", "storage-full expectation"),
        ("TEST_CHECK(!fake.final_exists);", "final file preservation"),
        ("TEST_CHECK(!fake.temporary_exists);", "temporary cleanup"),
    ):
        require_contains(enospc_test, marker, description)


def build_and_verify_image(
    partition_bytes: int, maximum: int
) -> tuple[int, int, int]:
    if partition_bytes != EXPECTED_PARTITION_BYTES:
        raise SystemExit(
            f"userdata partition changed from {EXPECTED_PARTITION_BYTES} "
            f"to {partition_bytes}; re-review V2-034 capacity"
        )
    if partition_bytes % BLOCK_SIZE != 0:
        raise SystemExit("userdata partition is not a whole number of LittleFS blocks")

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

    used_bytes = filesystem.used_block_count * BLOCK_SIZE
    payload_bytes = maximum * len(filenames)
    overhead_bytes = used_bytes - payload_bytes
    remaining_bytes = partition_bytes - used_bytes
    if payload_bytes != EXPECTED_PAYLOAD_BYTES:
        raise SystemExit(f"unexpected V2-034 payload total: {payload_bytes}")
    if overhead_bytes <= 0:
        raise SystemExit("capacity proof did not include positive filesystem overhead")
    if remaining_bytes < REQUIRED_MARGIN_BLOCKS * BLOCK_SIZE:
        raise SystemExit(
            f"candidate leaves only {remaining_bytes} bytes; "
            "at least one LittleFS block is required"
        )

    observed = (used_bytes, overhead_bytes, remaining_bytes)
    expected = (
        EXPECTED_USED_BYTES,
        EXPECTED_OVERHEAD_BYTES,
        EXPECTED_REMAINING_BYTES,
    )
    if observed != expected:
        raise SystemExit(
            "LittleFS geometry changed: "
            f"observed used/overhead/remaining={observed}, expected={expected}; "
            "re-review the candidate rather than silently accepting drift"
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

    return observed


def main() -> int:
    verify_dependency_version()
    maximum = verify_limit_constant()
    partition_bytes = read_partition_size("userdata")
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
