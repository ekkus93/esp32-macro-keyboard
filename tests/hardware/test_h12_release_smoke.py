#!/usr/bin/env python3
"""Retired interim H12-122 hardware smoke.

The final H12-122 path is ``scripts/run-h12-122-hardware.py``. That harness
atomically validates and flashes every manifest artifact before combining
provenance, HID send/confirmation/cancel, snapshot, password, restart, factory
reset, and reprovision evidence. Keeping this older split harness runnable would
allow incomplete or stale release evidence.
"""

import sys


def main() -> int:
    print(
        "error: test_h12_release_smoke.py is retired; run "
        "scripts/run-h12-122-hardware.py instead",
        file=sys.stderr,
    )
    return 2



if __name__ == "__main__":
    raise SystemExit(main())
