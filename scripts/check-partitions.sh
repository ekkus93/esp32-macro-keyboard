#!/usr/bin/env bash
set -euo pipefail

readonly csv="${1:-firmware/partitions.csv}"
readonly flash_bytes=$((8 * 1024 * 1024))

python3 - "${csv}" "${flash_bytes}" <<'PY2'
import csv
import sys
from dataclasses import dataclass
from pathlib import Path

APP_ALIGNMENT = 0x10000
DATA_ALIGNMENT = 0x1000
NVS_KEYS_SIZE = 0x1000


@dataclass(frozen=True)
class Partition:
    name: str
    partition_type: str
    subtype: str
    offset: int
    size: int
    flags: frozenset[str]

    @property
    def end(self) -> int:
        return self.offset + self.size


def fail(message: str) -> None:
    raise SystemExit(f"error: {message}")


def parse_number(value: str, field: str, name: str) -> int:
    try:
        number = int(value, 0)
    except ValueError:
        fail(f"partition {name} has invalid {field}: {value!r}")
    if number <= 0:
        fail(f"partition {name} has non-positive {field}: {value!r}")
    return number


def read_partitions(path: Path) -> list[Partition]:
    if not path.is_file():
        fail(f"partition table not found: {path}")

    partitions: list[Partition] = []
    previous_end = 0
    source_lines = (
        line for line in path.read_text(encoding="utf-8").splitlines()
        if line.strip() and not line.lstrip().startswith("#")
    )
    for line_number, row in enumerate(csv.reader(source_lines), start=1):
        if len(row) != 6:
            fail(f"partition row {line_number} must contain exactly 6 columns")
        name, partition_type, subtype, raw_offset, raw_size, raw_flags = (
            item.strip() for item in row
        )
        if not name or not partition_type or not subtype or not raw_size:
            fail(f"partition row {line_number} has an empty required field")
        size = parse_number(raw_size, "size", name)
        alignment = APP_ALIGNMENT if partition_type == "app" else DATA_ALIGNMENT
        if raw_offset:
            offset = parse_number(raw_offset, "offset", name)
        else:
            offset = (previous_end + alignment - 1) & ~(alignment - 1)
        if offset % alignment != 0:
            fail(
                f"partition {name} offset 0x{offset:x} is not aligned to 0x{alignment:x}"
            )
        if offset < previous_end:
            fail(
                f"partition {name} overlaps the previous partition: "
                f"0x{offset:x} < 0x{previous_end:x}"
            )
        flags = frozenset(flag.strip() for flag in raw_flags.split(":") if flag.strip())
        partition = Partition(name, partition_type, subtype, offset, size, flags)
        partitions.append(partition)
        previous_end = partition.end
    if not partitions:
        fail("partition table contains no partitions")
    return partitions


def require_unique_name(partitions: list[Partition], name: str) -> Partition:
    matches = [partition for partition in partitions if partition.name == name]
    if len(matches) != 1:
        fail(f"partition table must contain exactly one {name} partition")
    return matches[0]


def validate(partitions: list[Partition], flash_size: int) -> None:
    names = [partition.name for partition in partitions]
    if len(names) != len(set(names)):
        fail("partition names must be unique")

    key_partitions = [
        partition
        for partition in partitions
        if partition.partition_type == "data" and partition.subtype == "nvs_keys"
    ]
    if len(key_partitions) != 1:
        fail("partition table must contain exactly one data/nvs_keys partition")
    key_partition = key_partitions[0]
    if key_partition.name != "nvs_keys":
        fail("the data/nvs_keys partition must be named nvs_keys")
    if key_partition.size != NVS_KEYS_SIZE:
        fail("nvs_keys partition must be exactly 0x1000 bytes")
    if "encrypted" not in key_partition.flags:
        fail("nvs_keys partition must include the encrypted flag")

    ota_0 = require_unique_name(partitions, "ota_0")
    ota_1 = require_unique_name(partitions, "ota_1")
    if ota_0.partition_type != "app" or ota_0.subtype != "ota_0":
        fail("ota_0 must use type app and subtype ota_0")
    if ota_1.partition_type != "app" or ota_1.subtype != "ota_1":
        fail("ota_1 must use type app and subtype ota_1")
    if ota_0.size != ota_1.size:
        fail("ota_0 and ota_1 must have equal sizes")

    final_end = max(partition.end for partition in partitions)
    if final_end > flash_size:
        fail(
            f"partition table ends at 0x{final_end:x}, beyond "
            f"{flash_size // (1024 * 1024)} MiB flash"
        )
    print(
        f"partition table fits: final end 0x{final_end:x}; "
        f"OTA slots 0x{ota_0.size:x} bytes; nvs_keys protected"
    )


partition_path = Path(sys.argv[1])
configured_flash_size = int(sys.argv[2])
validate(read_partitions(partition_path), configured_flash_size)
PY2
