#!/usr/bin/env python3
"""Flash exactly the production artifacts named and hashed by a release manifest.

This is intentionally stricter than ``idf.py flash`` for final acceptance: the
manifest must identify the requested clean Git SHA, production build type, pinned
ESP-IDF, include ``webfs.bin``, and hash every image that will be written.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import NoReturn

SHA40_RE = re.compile(r"^[0-9a-f]{40}$")
SHA256_RE = re.compile(r"^[0-9a-f]{64}$")
EXPECTED_IDF = "ESP-IDF v5.5.5"
EXPECTED_APP_IMAGE = "esp32_macro_keyboard.bin"
EXPECTED_SETTINGS = {"flash_mode", "flash_size", "flash_freq"}
FLASH_MODE_RE = re.compile(r"^(?:keep|qio|qout|dio|dout)$")
FLASH_SIZE_RE = re.compile(r"^(?:keep|detect|[1-9][0-9]*(?:KB|MB))$")
FLASH_FREQ_RE = re.compile(r"^(?:keep|[1-9][0-9]*m)$")


def fail(message: str) -> NoReturn:
    raise SystemExit(f"error: {message}")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def git_output(repo_root: Path, *arguments: str) -> str:
    try:
        result = subprocess.run(
            ["git", "-C", str(repo_root), *arguments],
            check=True,
            capture_output=True,
            text=True,
        )
    except (OSError, subprocess.CalledProcessError) as error:
        fail(f"could not verify source checkout with git: {error}")
    return result.stdout.strip()


def validate_source_checkout(manifest: dict, repo_root: Path, expected_sha: str) -> None:
    head = git_output(repo_root, "rev-parse", "HEAD")
    if head != expected_sha:
        fail("current checkout HEAD does not match --firmware-sha")
    dirty = git_output(repo_root, "status", "--porcelain", "--untracked-files=all")
    if dirty:
        fail("current checkout is dirty; final release flashing requires a clean exact-SHA tree")

    lockfiles = {
        "managedComponentLockSha256": repo_root / "firmware" / "dependencies.lock",
        "frontendLockSha256": repo_root / "webapp" / "package-lock.json",
    }
    for field, path in lockfiles.items():
        expected = manifest.get(field)
        if not isinstance(expected, str) or not SHA256_RE.fullmatch(expected):
            fail(f"flash manifest {field} is invalid")
        if not path.is_file() or sha256(path) != expected:
            fail(f"flash manifest {field} does not match the exact source checkout")



def partition_offset(table_path: Path, label: str) -> int:
    """Read one named ESP-IDF partition offset from the flashed table bytes."""
    try:
        data = table_path.read_bytes()
    except OSError as error:
        fail(f"could not read partition table: {error}")
    matches: list[int] = []
    entry_bytes = 32
    for start in range(0, len(data) - entry_bytes + 1, entry_bytes):
        entry = data[start : start + entry_bytes]
        magic = int.from_bytes(entry[0:2], "little")
        if magic in (0xFFFF, 0xEBEB):
            break
        if magic != 0x50AA:
            fail("partition table contains an invalid entry magic")
        offset = int.from_bytes(entry[4:8], "little")
        raw_label = entry[12:28].split(b"\0", 1)[0]
        try:
            entry_label = raw_label.decode("ascii")
        except UnicodeDecodeError:
            fail("partition table contains a non-ASCII label")
        if entry_label == label:
            matches.append(offset)
    if len(matches) != 1:
        fail(f"partition table must contain exactly one {label} partition")
    return matches[0]


def validate_generated_layout(
    manifest_path: Path, manifest: dict, resolved: list[tuple[str, Path]]
) -> None:
    """Tie manifest addresses back to ESP-IDF output and the flashed partition table."""
    root = manifest_path.resolve().parent
    flasher_args_path = root / "flasher_args.json"
    try:
        flasher_args = json.loads(flasher_args_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        fail(f"could not read ESP-IDF flasher_args.json: {error}")
    if not isinstance(flasher_args, dict):
        fail("ESP-IDF flasher_args.json root must be an object")
    idf_files = flasher_args.get("flash_files")
    idf_settings = flasher_args.get("flash_settings")
    if not isinstance(idf_files, dict) or not idf_files:
        fail("ESP-IDF flasher_args.json has no flash_files")
    if idf_settings != manifest.get("flashSettings"):
        fail("manifest flash settings do not match ESP-IDF flasher_args.json")

    manifest_files = manifest["flashFiles"]
    webfs_entries = [
        (offset, relative)
        for offset, relative in manifest_files.items()
        if Path(relative).name == "webfs.bin"
    ]
    if len(webfs_entries) != 1:
        fail("release manifest must contain exactly one webfs.bin")
    manifest_idf_files = {
        offset: relative
        for offset, relative in manifest_files.items()
        if Path(relative).name != "webfs.bin"
    }
    if manifest_idf_files != idf_files:
        fail("manifest ESP-IDF flash layout does not match flasher_args.json")

    partition_tables = [path for _, path in resolved if path.name == "partition-table.bin"]
    if len(partition_tables) != 1:
        fail("release flash set must contain exactly one partition-table.bin")
    expected_webfs_offset = partition_offset(partition_tables[0], "webfs")
    observed_webfs_offset = int(webfs_entries[0][0], 16)
    if observed_webfs_offset != expected_webfs_offset:
        fail("manifest webfs offset does not match the flashed partition table")

def load_manifest(path: Path, expected_sha: str) -> tuple[dict, list[tuple[str, Path]]]:
    if not SHA40_RE.fullmatch(expected_sha):
        fail("--firmware-sha must be exactly 40 lowercase hexadecimal characters")
    try:
        manifest = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        fail(f"could not read flash manifest: {error}")
    if not isinstance(manifest, dict):
        fail("flash manifest root must be an object")
    if manifest.get("gitCommit") != expected_sha:
        fail("flash manifest gitCommit does not match --firmware-sha")
    if manifest.get("gitDirty") is not False:
        fail("flash manifest does not describe a clean source tree")
    if manifest.get("buildType") != "production":
        fail("flash manifest is not a production build")
    if manifest.get("espIdfVersion") != EXPECTED_IDF:
        fail(f"flash manifest must use {EXPECTED_IDF}")

    flash_files = manifest.get("flashFiles")
    flash_hashes = manifest.get("flashFileSha256")
    settings = manifest.get("flashSettings")
    if not isinstance(flash_files, dict) or not flash_files:
        fail("flash manifest has no flashFiles")
    if not isinstance(flash_hashes, dict) or set(flash_hashes) != set(flash_files):
        fail("flashFileSha256 must cover every flashFiles offset exactly")
    if not isinstance(settings, dict) or set(settings) != EXPECTED_SETTINGS:
        fail("flashSettings must contain exactly flash_mode, flash_size, and flash_freq")
    validators = {
        "flash_mode": FLASH_MODE_RE,
        "flash_size": FLASH_SIZE_RE,
        "flash_freq": FLASH_FREQ_RE,
    }
    for name, pattern in validators.items():
        value = settings.get(name)
        if not isinstance(value, str) or pattern.fullmatch(value) is None:
            fail(f"invalid flash setting {name}")

    validated_entries: list[tuple[int, str, str]] = []
    seen_numeric_offsets: set[int] = set()
    for offset, relative in flash_files.items():
        if not isinstance(offset, str) or re.fullmatch(r"0x[0-9a-f]+", offset) is None:
            fail(f"invalid flash offset: {offset!r}")
        numeric_offset = int(offset, 16)
        if offset != hex(numeric_offset) or numeric_offset in seen_numeric_offsets:
            fail(f"flash offset is not unique canonical lowercase hex: {offset!r}")
        seen_numeric_offsets.add(numeric_offset)
        if not isinstance(relative, str) or not relative:
            fail(f"invalid flash file entry at {offset!r}")
        validated_entries.append((numeric_offset, offset, relative))

    root = path.resolve().parent
    resolved: list[tuple[str, Path]] = []
    saw_webfs = False
    for _, offset, relative in sorted(validated_entries):
        candidate = (root / relative).resolve()
        try:
            candidate.relative_to(root)
        except ValueError:
            fail(f"flash file escapes manifest directory: {relative}")
        if not candidate.is_file():
            fail(f"flash file not found: {candidate}")
        expected_digest = flash_hashes[offset]
        if not isinstance(expected_digest, str) or not SHA256_RE.fullmatch(expected_digest):
            fail(f"invalid SHA-256 for flash file at {offset}")
        actual_digest = sha256(candidate)
        if actual_digest != expected_digest:
            fail(f"flash file hash mismatch at {offset}: {relative}")
        if candidate.name == "webfs.bin":
            saw_webfs = True
        resolved.append((offset, candidate))

    if not saw_webfs:
        fail("release manifest does not include webfs.bin")

    validate_generated_layout(path, manifest, resolved)

    app_image = manifest.get("appImage")
    app_image_sha = manifest.get("appImageSha256")
    app_elf_sha = manifest.get("appElfSha256")
    build_id = manifest.get("diagnosticsBuildId")
    if not isinstance(app_image, str) or not isinstance(app_image_sha, str):
        fail("manifest application-image provenance is incomplete")
    if Path(app_image).name != EXPECTED_APP_IMAGE:
        fail(f"manifest application image must be {EXPECTED_APP_IMAGE}")
    if not SHA256_RE.fullmatch(app_image_sha):
        fail("manifest appImageSha256 is invalid")
    app_path = (root / app_image).resolve()
    try:
        app_path.relative_to(root)
    except ValueError:
        fail("manifest application image escapes manifest directory")
    app_matches = [candidate for _, candidate in resolved if candidate == app_path]
    if len(app_matches) != 1 or sha256(app_path) != app_image_sha:
        fail("manifest application image hash does not match flashFiles")
    if not isinstance(app_elf_sha, str) or not SHA256_RE.fullmatch(app_elf_sha):
        fail("manifest appElfSha256 is invalid")
    if not isinstance(build_id, str) or not re.fullmatch(r"[0-9a-f]{39}", build_id):
        fail("manifest diagnosticsBuildId is invalid")
    if not app_elf_sha.startswith(build_id):
        fail("diagnosticsBuildId is not the ELF SHA prefix")
    return manifest, resolved


def build_command(
    manifest: dict, files: list[tuple[str, Path]], port: str, baud: int, esptool: str
) -> list[str]:
    settings = manifest["flashSettings"]
    command = [
        esptool,
        "--chip",
        "esp32s3",
        "--port",
        port,
        "--baud",
        str(baud),
        "--before",
        "default_reset",
        "--after",
        "hard_reset",
        "write_flash",
        "--flash_mode",
        settings["flash_mode"],
        "--flash_size",
        settings["flash_size"],
        "--flash_freq",
        settings["flash_freq"],
    ]
    for offset, path in files:
        command.extend((offset, str(path)))
    return command


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--flash-manifest", type=Path, required=True)
    parser.add_argument("--firmware-sha", required=True)
    parser.add_argument("--port", required=True)
    parser.add_argument("--baud", type=int, default=460800)
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    if args.baud <= 0:
        fail("--baud must be positive")
    manifest, files = load_manifest(args.flash_manifest, args.firmware_sha)
    validate_source_checkout(manifest, Path(__file__).resolve().parents[1], args.firmware_sha)
    esptool = shutil.which("esptool.py") or shutil.which("esptool")
    if esptool is None:
        fail("esptool.py/esptool not found; source ESP-IDF v5.5.5 first")
    command = build_command(manifest, files, args.port, args.baud, esptool)
    if args.dry_run:
        print("release manifest validation passed")
        print("flash command:", " ".join(command))
        return 0
    subprocess.run(command, check=True)
    print(f"release flash completed for {args.firmware_sha}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
