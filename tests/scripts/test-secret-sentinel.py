#!/usr/bin/env python3
"""Regression coverage for every Phase 18.5 secret-exclusion boundary."""

from __future__ import annotations

import base64
import json
import secrets
import subprocess
import tempfile
from pathlib import Path
from urllib.parse import quote


REPO_ROOT = Path(__file__).resolve().parents[2]
SCANNER = REPO_ROOT / "scripts" / "check-secret-sentinel.py"
BOUNDARIES = {
    "set-export.json": '{"package_type":"set","sets":[]}',
    "full-backup.json": '{"package_type":"backup","sets":[]}',
    "diagnostics.json": '{"status":"healthy","storage":{"mounted":true}}',
    "application.log": 'subsystem=storage primary_error=none cleanup_error=none',
    "frontend-persisted-state.json": '{"theme":"system","lastSetId":null}',
}


def run(scanner_args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["python3", str(SCANNER), *scanner_args],
        cwd=REPO_ROOT,
        text=True,
        capture_output=True,
        check=False,
    )


def variants(secret: str) -> dict[str, str]:
    raw = secret.encode("utf-8")
    return {
        "raw": secret,
        "json": json.dumps(secret)[1:-1],
        "url": quote(secret, safe=""),
        "base64": base64.b64encode(raw).decode("ascii"),
        "base64url": base64.urlsafe_b64encode(raw).decode("ascii").rstrip("="),
        "hex": raw.hex(),
    }


def main() -> int:
    secret = "phase18_5_" + secrets.token_urlsafe(32)
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        secret_file = root / "sentinel.txt"
        secret_file.write_text(secret + "\n", encoding="utf-8")
        outputs: list[Path] = []
        for name, content in BOUNDARIES.items():
            path = root / name
            path.write_text(content, encoding="utf-8")
            outputs.append(path)

        clean = run(["--secret-file", str(secret_file), *map(str, outputs)])
        assert clean.returncode == 0, clean.stdout + clean.stderr

        for boundary in outputs:
            original = boundary.read_text(encoding="utf-8")
            for label, encoded in variants(secret).items():
                boundary.write_text(original + "\n" + encoded, encoding="utf-8")
                leaked = run(["--secret-file", str(secret_file), str(boundary)])
                assert leaked.returncode == 1, (boundary.name, label)
                report = leaked.stdout + leaked.stderr
                assert secret not in report
                assert encoded not in report
                assert boundary.name in report
                boundary.write_text(original, encoding="utf-8")

        short_file = root / "short.txt"
        short_file.write_text("too-short", encoding="utf-8")
        rejected = run(["--secret-file", str(short_file), str(outputs[0])])
        assert rejected.returncode == 2

    print("secret sentinel scanner tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
