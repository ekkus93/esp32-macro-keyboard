#!/usr/bin/env python3
"""Fake gen_esp32part.py for the generate-flash-manifest.sh regression
tests. Ignores the real binary partition-table content (the test never
constructs a real one) and prints a fixed resolved CSV whose "webfs" row
offset is controlled by FAKE_WEBFS_OFFSET, so the manifest generator's
offset-resolution and JSON-assembly logic can be exercised deterministically
without a real ESP-IDF environment or a real partition-table.bin.
"""

import os
import sys

offset = os.environ.get("FAKE_WEBFS_OFFSET", "0x520000")

print("# ESP-IDF Partition Table")
print("# Name, Type, SubType, Offset, Size, Flags")
print("nvs,data,nvs,0x9000,24K,")
print(f"webfs,data,littlefs,{offset},1M,")
sys.exit(0)
