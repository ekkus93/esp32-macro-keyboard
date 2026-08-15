#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import json
import struct
import subprocess
import tempfile
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
SCRIPT = REPO / "scripts" / "flash-release-manifest.py"
spec = importlib.util.spec_from_file_location("flash_release_manifest", SCRIPT)
module = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(module)

SHA = "a" * 40
ELF = "b" * 64


def fixture(root: Path) -> Path:
    files = {
        "0x0": "bootloader/bootloader.bin",
        "0x8000": "partition_table/partition-table.bin",
        "0xf000": "ota_data_initial.bin",
        "0x20000": "esp32_macro_keyboard.bin",
        "0x520000": "webfs.bin",
    }
    for offset, relative in files.items():
        path = root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes((offset + relative).encode())

    # Minimal ESP-IDF binary partition table entry for the actual webfs offset.
    label = b"webfs" + b"\0" * 11
    (root / "partition_table/partition-table.bin").write_bytes(
        struct.pack("<HBBII16sI", 0x50AA, 0x01, 0x83, 0x520000, 0x100000, label, 0)
        + b"\xff" * 32
    )
    idf_files = {offset: relative for offset, relative in files.items() if relative != "webfs.bin"}
    (root / "flasher_args.json").write_text(
        json.dumps(
            {
                "flash_settings": {"flash_mode": "dio", "flash_size": "8MB", "flash_freq": "80m"},
                "flash_files": idf_files,
            }
        ),
        encoding="utf-8",
    )
    hashes = {offset: module.sha256(root / relative) for offset, relative in files.items()}
    manifest = {
        "gitCommit": SHA,
        "gitDirty": False,
        "espIdfVersion": module.EXPECTED_IDF,
        "buildType": "production",
        "appImage": "esp32_macro_keyboard.bin",
        "appImageSha256": hashes["0x20000"],
        "appElfSha256": ELF,
        "diagnosticsBuildId": ELF[:39],
        "flashSettings": {"flash_mode": "dio", "flash_size": "8MB", "flash_freq": "80m"},
        "flashFiles": files,
        "flashFileSha256": hashes,
    }
    path = root / "flash-manifest.json"
    path.write_text(json.dumps(manifest), encoding="utf-8")
    return path


def expect_failure(path: Path, message: str) -> None:
    try:
        module.load_manifest(path, SHA)
    except SystemExit as error:
        assert message in str(error), (message, error)
    else:
        raise AssertionError(f"expected failure containing {message!r}")


def git(*arguments: str, cwd: Path) -> str:
    return subprocess.run(
        ["git", *arguments], cwd=cwd, check=True, capture_output=True, text=True
    ).stdout.strip()


def checkout_fixture(root: Path) -> tuple[dict, str]:
    (root / "firmware").mkdir(parents=True)
    (root / "webapp").mkdir(parents=True)
    component = root / "firmware" / "dependencies.lock"
    frontend = root / "webapp" / "package-lock.json"
    component.write_text("components\n", encoding="utf-8")
    frontend.write_text("frontend\n", encoding="utf-8")
    git("init", "-q", cwd=root)
    git("config", "user.name", "H12 Test", cwd=root)
    git("config", "user.email", "h12@example.invalid", cwd=root)
    git("add", ".", cwd=root)
    git("commit", "-qm", "fixture", cwd=root)
    head = git("rev-parse", "HEAD", cwd=root)
    manifest = {
        "managedComponentLockSha256": module.sha256(component),
        "frontendLockSha256": module.sha256(frontend),
    }
    return manifest, head


def main() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        path = fixture(root)
        manifest, files = module.load_manifest(path, SHA)
        assert len(files) == 5
        command = module.build_command(manifest, files, "/dev/ttyACM0", 460800, "esptool.py")
        assert command[-2:] == ["0x520000", str(root / "webfs.bin")]

        data = json.loads(path.read_text())
        data["gitDirty"] = True
        path.write_text(json.dumps(data))
        expect_failure(path, "clean source tree")

        path = fixture(root)
        data = json.loads(path.read_text())
        data["flashFiles"].pop("0x520000")
        data["flashFileSha256"].pop("0x520000")
        path.write_text(json.dumps(data))
        expect_failure(path, "webfs.bin")

        path = fixture(root)
        (root / "webfs.bin").write_bytes(b"mutated")
        expect_failure(path, "hash mismatch")

        path = fixture(root)
        data = json.loads(path.read_text())
        data["flashFiles"]["0x520000"] = "../escape.bin"
        data["flashFileSha256"]["0x520000"] = "0" * 64
        (root.parent / "escape.bin").write_bytes(b"escape")
        path.write_text(json.dumps(data))
        expect_failure(path, "escapes manifest directory")

        path = fixture(root)
        data = json.loads(path.read_text())
        data["flashFiles"]["0x00"] = data["flashFiles"].pop("0x0")
        data["flashFileSha256"]["0x00"] = data["flashFileSha256"].pop("0x0")
        path.write_text(json.dumps(data))
        expect_failure(path, "canonical lowercase hex")

        path = fixture(root)
        data = json.loads(path.read_text())
        data["flashSettings"]["flash_mode"] = "--erase-all"
        path.write_text(json.dumps(data))
        expect_failure(path, "invalid flash setting flash_mode")

        path = fixture(root)
        data = json.loads(path.read_text())
        data["appImage"] = "test_app.bin"
        path.write_text(json.dumps(data))
        expect_failure(path, "application image must be esp32_macro_keyboard.bin")

        path = fixture(root)
        data = json.loads(path.read_text())
        data["flashFiles"]["0x10000"] = data["flashFiles"].pop("0xf000")
        data["flashFileSha256"]["0x10000"] = data["flashFileSha256"].pop("0xf000")
        path.write_text(json.dumps(data))
        expect_failure(path, "does not match flasher_args.json")

        path = fixture(root)
        data = json.loads(path.read_text())
        data["flashFiles"]["0x530000"] = data["flashFiles"].pop("0x520000")
        data["flashFileSha256"]["0x530000"] = data["flashFileSha256"].pop("0x520000")
        path.write_text(json.dumps(data))
        expect_failure(path, "webfs offset does not match")

        path = fixture(root)
        flasher = json.loads((root / "flasher_args.json").read_text())
        flasher["flash_settings"]["flash_freq"] = "40m"
        (root / "flasher_args.json").write_text(json.dumps(flasher), encoding="utf-8")
        expect_failure(path, "settings do not match")

    with tempfile.TemporaryDirectory() as temp:
        checkout = Path(temp)
        provenance, head = checkout_fixture(checkout)
        module.validate_source_checkout(provenance, checkout, head)
        try:
            module.validate_source_checkout(provenance, checkout, "0" * 40)
        except SystemExit as error:
            assert "HEAD does not match" in str(error)
        else:
            raise AssertionError("wrong source SHA unexpectedly accepted")
        (checkout / "untracked.txt").write_text("dirty\n", encoding="utf-8")
        try:
            module.validate_source_checkout(provenance, checkout, head)
        except SystemExit as error:
            assert "checkout is dirty" in str(error)
        else:
            raise AssertionError("dirty source checkout unexpectedly accepted")

    print("flash-release-manifest regression tests passed: 14")


if __name__ == "__main__":
    main()
