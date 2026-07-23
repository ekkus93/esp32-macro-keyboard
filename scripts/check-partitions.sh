#!/usr/bin/env bash
set -euo pipefail

readonly csv="${1:-firmware/partitions.csv}"
readonly flash_bytes=$((8 * 1024 * 1024))

python3 - "${csv}" "${flash_bytes}" <<'PY2'
import csv
import sys
from pathlib import Path

path = Path(sys.argv[1])
flash_size = int(sys.argv[2])
offset = 0
for row in csv.reader(line for line in path.read_text().splitlines() if line.strip() and not line.lstrip().startswith('#')):
    name, _type, _subtype, raw_offset, raw_size, *_ = [item.strip() for item in row]
    size = int(raw_size, 0)
    if raw_offset:
        offset = int(raw_offset, 0)
    else:
        alignment = 0x10000 if _type == 'app' else 0x1000
        offset = (offset + alignment - 1) & ~(alignment - 1)
    end = offset + size
    if end > flash_size:
        raise SystemExit(f'partition {name} ends at 0x{end:x}, beyond 8 MiB flash')
    offset = end
print(f'partition table fits: final end 0x{offset:x}')
PY2
