"""Hashing, JSON state I/O, the blob journal, and firmware provenance."""

from __future__ import annotations

import hashlib
import json
import os
import shutil
import subprocess
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

from .core import (
    BUILD_ID_PATTERN,
    ELF_SHA_OUTPUT_PATTERN,
    ESP_IDF_VERSION,
    EvidenceError,
    FIRMWARE_COMMIT_PATTERN,
    SHA256_PATTERN,
    STATE_SCHEMA,
    require,
)


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat().replace("+00:00", "Z")


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(65_536), b""):
            digest.update(chunk)
    return digest.hexdigest()


def resolve_manifest_app_image(manifest_path: Path, manifest: dict[str, Any]) -> Path:
    app_image = manifest.get("appImage")
    require(isinstance(app_image, str) and bool(app_image),
            "flash manifest appImage is missing")
    relative = Path(app_image)
    require(not relative.is_absolute() and ".." not in relative.parts,
            "flash manifest appImage must remain inside the build directory")
    build_directory = manifest_path.parent.resolve()
    resolved = (build_directory / relative).resolve()
    require(resolved.is_relative_to(build_directory),
            "flash manifest appImage escapes the build directory")
    require(resolved.is_file(), f"flash manifest application image is missing: {resolved}")
    return resolved


def read_app_elf_sha256(app_image: Path) -> str:
    esptool = shutil.which("esptool.py")
    require(esptool is not None,
            "esptool.py is required to verify the flash manifest application image")
    try:
        result = subprocess.run(
            [esptool, "image_info", "--version", "2", str(app_image)],
            check=False,
            capture_output=True,
            text=True,
        )
    except OSError as error:
        raise EvidenceError(f"could not run esptool.py image_info: {error}") from error
    output = result.stdout + "\n" + result.stderr
    require(result.returncode == 0,
            f"esptool.py image_info failed with exit {result.returncode}: {output.strip()}")
    match = ELF_SHA_OUTPUT_PATTERN.search(output)
    require(match is not None,
            "esptool.py image_info did not report a full ELF file SHA256")
    return match.group(1).lower()



def read_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise EvidenceError(f"could not read {path}: {error}") from error
    require(isinstance(value, dict), f"{path} must contain a JSON object")
    return value


def write_json(path: Path, value: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    os.replace(temporary, path)


def owned_blobs(state: dict[str, Any]) -> list[dict[str, str]]:
    value = state.setdefault("ownedBlobs", [])
    require(isinstance(value, list), "evidence ownedBlobs must be a list")
    seen: set[str] = set()
    for item in value:
        require(isinstance(item, dict)
                and isinstance(item.get("id"), str)
                and isinstance(item.get("sha256"), str)
                and SHA256_PATTERN.fullmatch(item["sha256"]) is not None,
                "evidence ownedBlobs contains an invalid entry")
        require(item["id"] not in seen, f"duplicate collector-owned blob ID {item['id']}")
        seen.add(item["id"])
    return value


def journal_created_blob(state_path: Path, state: dict[str, Any], item: dict[str, str],
                         stage_key: str | None = None) -> None:
    current = owned_blobs(state)
    require(all(existing["id"] != item["id"] for existing in current),
            f"collector-owned blob ID {item['id']} was already recorded")
    current.append(dict(item))
    if stage_key is not None:
        stage = state.setdefault(stage_key, [])
        require(isinstance(stage, list), f"evidence {stage_key} must be a list")
        stage.append(dict(item))
    write_json(state_path, state)


def journal_deleted_blob(state_path: Path, state: dict[str, Any], blob_id: str) -> None:
    state["ownedBlobs"] = [item for item in owned_blobs(state) if item["id"] != blob_id]
    write_json(state_path, state)


def load_state(path: Path, expected_phase: str | None = None) -> dict[str, Any]:
    state = read_json(path)
    require(state.get("schemaVersion") == STATE_SCHEMA, "unsupported evidence state schema")
    if expected_phase is not None:
        require(state.get("phase") == expected_phase,
                f"expected phase {expected_phase!r}, found {state.get('phase')!r}")
    return state


def password_from_environment(name: str) -> str:
    password = os.environ.get(name, "")
    require(bool(password), f"set {name} instead of placing the password on the command line")
    return password


def load_flash_manifest(path: Path) -> dict[str, str]:
    manifest = read_json(path)
    git_commit = manifest.get("gitCommit")
    app_image_sha256 = manifest.get("appImageSha256")
    app_elf_sha256 = manifest.get("appElfSha256")
    diagnostics_build_id = manifest.get("diagnosticsBuildId")
    esp_idf_version = manifest.get("espIdfVersion")
    require(isinstance(git_commit, str)
            and FIRMWARE_COMMIT_PATTERN.fullmatch(git_commit) is not None,
            "flash manifest gitCommit must be an exact 40-character SHA")
    require(manifest.get("gitDirty") is False,
            "flash manifest records a dirty build; rebuild from a clean checkout")
    require(manifest.get("buildType") == "production",
            "flash manifest is not a production build")
    require(esp_idf_version == ESP_IDF_VERSION,
            f"flash manifest must use {ESP_IDF_VERSION}, found {esp_idf_version!r}")
    require(isinstance(app_image_sha256, str)
            and SHA256_PATTERN.fullmatch(app_image_sha256) is not None,
            "flash manifest appImageSha256 is invalid")
    require(isinstance(app_elf_sha256, str)
            and SHA256_PATTERN.fullmatch(app_elf_sha256) is not None,
            "flash manifest appElfSha256 is invalid")
    require(isinstance(diagnostics_build_id, str)
            and BUILD_ID_PATTERN.fullmatch(diagnostics_build_id) is not None,
            "flash manifest diagnosticsBuildId is invalid")
    app_image = resolve_manifest_app_image(path, manifest)
    actual_image_sha256 = sha256_file(app_image)
    require(actual_image_sha256 == app_image_sha256,
            "flash manifest application-image SHA-256 does not match the actual image")
    actual_elf_sha256 = read_app_elf_sha256(app_image)
    require(actual_elf_sha256 == app_elf_sha256,
            "flash manifest ELF SHA-256 does not match esptool.py image_info")
    require(actual_elf_sha256.startswith(diagnostics_build_id),
            "flash manifest diagnosticsBuildId is not the verified ELF SHA prefix")
    return {
        "gitCommit": git_commit,
        "appImage": str(manifest["appImage"]),
        "appImageSha256": actual_image_sha256,
        "appElfSha256": actual_elf_sha256,
        "diagnosticsBuildId": diagnostics_build_id,
        "espIdfVersion": esp_idf_version,
        "flashManifestSha256": sha256_file(path),
    }


def verify_firmware_provenance(manifest: dict[str, str], diagnostics: dict[str, Any]) -> None:
    # The board's live buildId is a short prefix (CONFIG_APP_RETRIEVE_LEN_ELF_SHA
    # hex chars); the manifest's diagnosticsBuildId is the fuller 39-character
    # prefix esptool verified against the actual flashed image. Provenance is
    # established by the shorter one being a prefix of the longer one, not
    # exact equality of two different-length strings.
    require(manifest["diagnosticsBuildId"].startswith(diagnostics["buildId"]),
            "board buildId is not a prefix of the verified application image in the flash manifest")


