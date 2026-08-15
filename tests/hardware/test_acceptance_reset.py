#!/usr/bin/env python3
"""Retired reset harness.

This file previously exercised deleted v1-shaped setup routes and revisioned
settings fields, so keeping it runnable would create false hardware evidence.
Use ``scripts/run-h12-122-hardware.py`` for current exact-release restart,
factory-reset, and reprovision acceptance.
"""

import sys


def main() -> int:
    print(
        "error: test_acceptance_reset.py is retired; run "
        "scripts/run-h12-122-hardware.py with the exact production firmware "
        "SHA and flash manifest instead",
        file=sys.stderr,
    )
    return 2



if __name__ == "__main__":
    raise SystemExit(main())
