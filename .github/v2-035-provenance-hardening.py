#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def replace_once(path: str, old: str, new: str) -> None:
    target = ROOT / path
    text = target.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{path}: expected one match, found {count}: {old[:80]!r}")
    target.write_text(text.replace(old, new, 1), encoding="utf-8")


def insert_after(path: str, anchor: str, addition: str) -> None:
    replace_once(path, anchor, anchor + addition)


# Extend the existing flash manifest with the exact application-image and ELF
# provenance needed to compare a physical board's diagnostics buildId.
insert_after(
    "scripts/generate-flash-manifest.sh",
    "command -v idf.py >/dev/null 2>&1 || {\n\tprintf 'error: idf.py not found; source the pinned ESP-IDF v5.5.5 export.sh first (see CLAUDE.md).\\n' >&2\n\texit 2\n}\n",
    "\ncommand -v esptool.py >/dev/null 2>&1 || {\n\tprintf 'error: esptool.py not found; source the pinned ESP-IDF v5.5.5 export.sh first (see CLAUDE.md).\\n' >&2\n\texit 2\n}\n",
)
insert_after(
    "scripts/generate-flash-manifest.sh",
    "idf_version=\"$(idf.py --version)\"\n[ -n \"${idf_version}\" ] || {\n\tprintf 'error: idf.py --version produced no output\\n' >&2\n\texit 2\n}\n",
    "\napp_relative=\"$(python3 - \"${flasher_args}\" <<'PY'\nimport json\nimport sys\n\nwith open(sys.argv[1], encoding=\"utf-8\") as handle:\n    flash_files = json.load(handle)[\"flash_files\"]\n\ncandidates = [\n    path for path in flash_files.values()\n    if path.endswith(\".bin\") and path.rsplit(\"/\", 1)[-1] == \"esp32_macro_keyboard.bin\"\n]\nif len(candidates) != 1:\n    raise SystemExit(\"expected exactly one esp32_macro_keyboard.bin application image\")\nprint(candidates[0])\nPY\n)\"\napp_image=\"${build_dir}/${app_relative}\"\n[ -f \"${app_image}\" ] || {\n\tprintf 'error: application image not found: %s\\n' \"${app_image}\" >&2\n\texit 2\n}\napp_image_sha256=\"$(sha256sum -- \"${app_image}\" | awk '{print $1}')\"\nimage_info=\"$(esptool.py image_info \"${app_image}\")\"\napp_elf_sha256=\"$(printf '%s\\n' \"${image_info}\" | python3 -c 'import re,sys; text=sys.stdin.read(); match=re.search(r\"ELF file SHA256:\\s*([0-9A-Fa-f]{64})\", text); print(match.group(1).lower() if match else \"\")')\"\nif ! printf '%s' \"${app_elf_sha256}\" | grep -Eq '^[0-9a-f]{64}$'; then\n\tprintf 'error: esptool.py image_info did not report a full ELF file SHA256\\n' >&2\n\texit 2\nfi\ndiagnostics_build_id=\"${app_elf_sha256:0:39}\"\n",
)
replace_once(
    "scripts/generate-flash-manifest.sh",
    "\t\"${build_type}\" \"${build_timestamp}\" \"${webfs_relative}\" \"${webfs_offset}\" <<'PY'\n",
    "\t\"${build_type}\" \"${build_timestamp}\" \"${webfs_relative}\" \"${webfs_offset}\" \\\n\t\"${app_relative}\" \"${app_image_sha256}\" \"${app_elf_sha256}\" \"${diagnostics_build_id}\" <<'PY'\n",
)
replace_once(
    "scripts/generate-flash-manifest.sh",
    " webfs_relative, webfs_offset) = sys.argv[1:12]\n",
    " webfs_relative, webfs_offset, app_relative, app_image_sha256,\n app_elf_sha256, diagnostics_build_id) = sys.argv[1:16]\n",
)
replace_once(
    "scripts/generate-flash-manifest.sh",
    "    \"buildTimestamp\": build_timestamp,\n    \"flashSettings\": flasher_args[\"flash_settings\"],\n",
    "    \"buildTimestamp\": build_timestamp,\n    \"appImage\": app_relative,\n    \"appImageSha256\": app_image_sha256,\n    \"appElfSha256\": app_elf_sha256,\n    \"diagnosticsBuildId\": diagnostics_build_id,\n    \"flashSettings\": flasher_args[\"flash_settings\"],\n",
)

# Add a deterministic esptool fake and include it in the first-party script gate.
fake_esptool = ROOT / "tests/scripts/fakes/esptool.py"
fake_esptool.write_text(
    """#!/usr/bin/env bash
# Fake esptool.py for flash-manifest regression tests. It only implements
# `image_info`, returning a deterministic full ELF SHA-256.
set -euo pipefail

if [ "${1:-}" = "image_info" ] && [ -n "${2:-}" ]; then
\tprintf 'Application Information\\n'
\tprintf 'ELF file SHA256: %s\\n' "${FAKE_ELF_SHA256:-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa}"
\texit 0
fi

printf 'fake esptool.py: unexpected arguments: %s\\n' "$*" >&2
exit 2
""",
    encoding="utf-8",
)
replace_once(
    "scripts/check-scripts.sh",
    "\ttests/scripts/fakes/idf.py\n",
    "\ttests/scripts/fakes/idf.py tests/scripts/fakes/esptool.py\n",
)

# Expand flash-manifest regression fixtures and assertions.
insert_after(
    "tests/scripts/test-generate-flash-manifest.sh",
    "\t: >\"${build_dir}/partition_table/partition-table.bin\"\n",
    "\tprintf 'fixture application image\\n' >\"${build_dir}/esp32_macro_keyboard.bin\"\n",
)
insert_after(
    "tests/scripts/test-generate-flash-manifest.sh",
    "[ \"$(python3 -c \"import json; print(len(json.load(open('${output_file}'))['gitCommit']))\")\" = 40 ] || {\n\tprintf 'FAIL: gitCommit is not a 40-character SHA\\n' >&2\n\texit 1\n}\n",
    "[ \"$(field appImage)\" = \"esp32_macro_keyboard.bin\" ] || {\n\tprintf 'FAIL: unexpected appImage: %s\\n' \"$(field appImage)\" >&2\n\texit 1\n}\n[ \"$(field appElfSha256)\" = \"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\" ] || {\n\tprintf 'FAIL: unexpected appElfSha256: %s\\n' \"$(field appElfSha256)\" >&2\n\texit 1\n}\n[ \"$(field diagnosticsBuildId)\" = \"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\" ] || {\n\tprintf 'FAIL: unexpected diagnosticsBuildId: %s\\n' \"$(field diagnosticsBuildId)\" >&2\n\texit 1\n}\n[ \"$(python3 -c \"import json,re; value=json.load(open('${output_file}'))['appImageSha256']; print(bool(re.fullmatch(r'[0-9a-f]{64}', value)))\")\" = True ] || {\n\tprintf 'FAIL: appImageSha256 is not a full lowercase SHA-256\\n' >&2\n\texit 1\n}\n",
)
insert_after(
    "tests/scripts/test-generate-flash-manifest.sh",
    "[ \"$(field managedComponentLockSha256)\" != \"${baseline_hash}\" ] || {\n\tprintf 'FAIL: managedComponentLockSha256 did not change with lockfile content\\n' >&2\n\texit 1\n}\n",
    "\n# The board-visible diagnostics build ID must be the exact 39-character prefix\n# produced by esp_app_get_elf_sha256 with the production diagnostics buffer.\nwrite_fixtures production\nFAKE_ELF_SHA256=0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef expect_pass 'ELF build ID derivation'\n[ \"$(field diagnosticsBuildId)\" = \"0123456789abcdef0123456789abcdef0123456\" ] || {\n\tprintf 'FAIL: diagnosticsBuildId did not match the ELF SHA prefix\\n' >&2\n\texit 1\n}\n\nwrite_fixtures production\nrm -f -- \"${build_dir}/esp32_macro_keyboard.bin\"\nexpect_fail 'missing application image' 'application image not found'\n\nwrite_fixtures production\nFAKE_ELF_SHA256=not-a-sha expect_fail 'invalid ELF SHA' 'did not report a full ELF file SHA256'\n",
)

# Require a clean production flash manifest and exact board build-ID match in
# the V2-035 collector before any blob mutation.
replace_once(
    "scripts/run-v2-035-hardware.py",
    "STATE_SCHEMA = 1\nFIRMWARE_COMMIT_PATTERN = re.compile(r\"^[0-9a-f]{40}$\")\n",
    "STATE_SCHEMA = 2\nFIRMWARE_COMMIT_PATTERN = re.compile(r\"^[0-9a-f]{40}$\")\nSHA256_PATTERN = re.compile(r\"^[0-9a-f]{64}$\")\nBUILD_ID_PATTERN = re.compile(r\"^[0-9a-f]{39}$\")\n",
)
insert_after(
    "scripts/run-v2-035-hardware.py",
    "def password_from_environment(name: str) -> str:\n    password = os.environ.get(name, \"\")\n    require(bool(password), f\"set {name} instead of placing the password on the command line\")\n    return password\n",
    "\n\ndef load_flash_manifest(path: Path) -> dict[str, str]:\n    manifest = read_json(path)\n    git_commit = manifest.get(\"gitCommit\")\n    app_image_sha256 = manifest.get(\"appImageSha256\")\n    app_elf_sha256 = manifest.get(\"appElfSha256\")\n    diagnostics_build_id = manifest.get(\"diagnosticsBuildId\")\n    require(isinstance(git_commit, str)\n            and FIRMWARE_COMMIT_PATTERN.fullmatch(git_commit) is not None,\n            \"flash manifest gitCommit must be an exact 40-character SHA\")\n    require(manifest.get(\"gitDirty\") is False,\n            \"flash manifest records a dirty build; rebuild from a clean checkout\")\n    require(manifest.get(\"buildType\") == \"production\",\n            \"flash manifest is not a production build\")\n    require(isinstance(app_image_sha256, str)\n            and SHA256_PATTERN.fullmatch(app_image_sha256) is not None,\n            \"flash manifest appImageSha256 is invalid\")\n    require(isinstance(app_elf_sha256, str)\n            and SHA256_PATTERN.fullmatch(app_elf_sha256) is not None,\n            \"flash manifest appElfSha256 is invalid\")\n    require(isinstance(diagnostics_build_id, str)\n            and BUILD_ID_PATTERN.fullmatch(diagnostics_build_id) is not None,\n            \"flash manifest diagnosticsBuildId is invalid\")\n    require(app_elf_sha256.startswith(diagnostics_build_id),\n            \"flash manifest diagnosticsBuildId is not the ELF SHA prefix\")\n    return {\n        \"gitCommit\": git_commit,\n        \"appImageSha256\": app_image_sha256,\n        \"appElfSha256\": app_elf_sha256,\n        \"diagnosticsBuildId\": diagnostics_build_id,\n        \"espIdfVersion\": str(manifest.get(\"espIdfVersion\", \"\")),\n        \"flashManifestSha256\": sha256_file(path),\n    }\n\n\ndef verify_firmware_provenance(manifest: dict[str, str], diagnostics: dict[str, Any]) -> None:\n    require(diagnostics[\"buildId\"] == manifest[\"diagnosticsBuildId\"],\n            \"board buildId does not match the exact application image in the flash manifest\")\n",
)
replace_once(
    "scripts/run-v2-035-hardware.py",
    "    require(isinstance(build_id, str) and bool(build_id), \"diagnostics buildId is invalid\")\n",
    "    require(isinstance(build_id, str)\n            and BUILD_ID_PATTERN.fullmatch(build_id) is not None,\n            \"diagnostics buildId must be a 39-character lowercase ELF SHA prefix\")\n",
)
replace_once(
    "scripts/run-v2-035-hardware.py",
    "    api = DeviceApi(args.base_url, args.timeout)\n    api.login(password_from_environment(args.password_env))\n    firmware_commit = args.firmware_sha.strip().lower()\n    require(\n        FIRMWARE_COMMIT_PATTERN.fullmatch(firmware_commit) is not None,\n        \"firmware SHA must be exactly 40 hexadecimal characters\",\n    )\n    baseline = snapshot(api)\n    initial_diagnostics = parse_diagnostics(api.diagnostics())\n",
    "    manifest = load_flash_manifest(args.flash_manifest)\n    api = DeviceApi(args.base_url, args.timeout)\n    api.login(password_from_environment(args.password_env))\n    initial_diagnostics = parse_diagnostics(api.diagnostics())\n    verify_firmware_provenance(manifest, initial_diagnostics)\n    baseline = snapshot(api)\n",
)
replace_once(
    "scripts/run-v2-035-hardware.py",
    "        \"firmwareCommit\": firmware_commit,\n        \"targetHardware\": \"ESP32-S3R8\",\n",
    "        \"firmwareCommit\": manifest[\"gitCommit\"],\n        \"firmwareBuildId\": manifest[\"diagnosticsBuildId\"],\n        \"appImageSha256\": manifest[\"appImageSha256\"],\n        \"appElfSha256\": manifest[\"appElfSha256\"],\n        \"flashManifestSha256\": manifest[\"flashManifestSha256\"],\n        \"espIdfVersion\": manifest[\"espIdfVersion\"],\n        \"targetHardware\": \"ESP32-S3R8\",\n",
)
insert_after(
    "scripts/run-v2-035-hardware.py",
    "    require(\n        state.get(\"targetHardware\") == \"ESP32-S3R8\",\n        \"evidence target hardware is not ESP32-S3R8\",\n    )\n",
    "    firmware_build_id = state.get(\"firmwareBuildId\")\n    app_image_sha256 = state.get(\"appImageSha256\")\n    app_elf_sha256 = state.get(\"appElfSha256\")\n    manifest_sha256 = state.get(\"flashManifestSha256\")\n    require(isinstance(firmware_build_id, str)\n            and BUILD_ID_PATTERN.fullmatch(firmware_build_id) is not None,\n            \"evidence is not bound to the board-visible firmware build ID\")\n    for label, value in ((\"app image\", app_image_sha256),\n                         (\"app ELF\", app_elf_sha256),\n                         (\"flash manifest\", manifest_sha256)):\n        require(isinstance(value, str) and SHA256_PATTERN.fullmatch(value) is not None,\n                f\"evidence {label} SHA-256 is invalid\")\n    require(app_elf_sha256.startswith(firmware_build_id),\n            \"evidence firmware build ID is not the recorded ELF SHA prefix\")\n",
)
replace_once(
    "scripts/run-v2-035-hardware.py",
    "    start.add_argument(\"--firmware-sha\", required=True)\n",
    "    start.add_argument(\"--flash-manifest\", type=Path, required=True)\n",
)

# Update collector tests for schema v2 and fail-closed manifest/board matching.
replace_once(
    "tests/scripts/test-v2-035-hardware.py",
    "        \"firmwareCommit\": \"0123456789abcdef0123456789abcdef01234567\",\n        \"targetHardware\": \"ESP32-S3R8\",\n",
    "        \"firmwareCommit\": \"0123456789abcdef0123456789abcdef01234567\",\n        \"firmwareBuildId\": \"a\" * 39,\n        \"appImageSha256\": \"b\" * 64,\n        \"appElfSha256\": \"a\" * 64,\n        \"flashManifestSha256\": \"c\" * 64,\n        \"targetHardware\": \"ESP32-S3R8\",\n",
)
replace_once(
    "tests/scripts/test-v2-035-hardware.py",
    "            \"buildId\": \"abc123\",\n",
    "            \"buildId\": \"a\" * 39,\n",
)
replace_once(
    "tests/scripts/test-v2-035-hardware.py",
    "            \"buildId\": \"abc123\",\n",
    "            \"buildId\": \"a\" * 39,\n",
)
insert_after(
    "tests/scripts/test-v2-035-hardware.py",
    "def test_diagnostics_schema() -> None:\n",
    "    assert MODULE.BUILD_ID_PATTERN.fullmatch(\"a\" * 39) is not None\n",
)
insert_after(
    "tests/scripts/test-v2-035-hardware.py",
    "    expect_failure(\n        MODULE.parse_diagnostics,\n        {\n            \"buildId\": \"a\" * 39,\n            \"resetReason\": \"power-on\",\n            \"uptimeMs\": 12,\n            \"blobScan\": {\"temporaryFileCount\": 1, \"temporaryFiles\": []},\n        },\n    )\n",
    "\n\ndef test_flash_manifest_provenance() -> None:\n    with tempfile.TemporaryDirectory() as temporary_directory:\n        manifest_path = Path(temporary_directory) / \"flash-manifest.json\"\n        manifest_path.write_text(\n            json.dumps(\n                {\n                    \"gitCommit\": \"0123456789abcdef0123456789abcdef01234567\",\n                    \"gitDirty\": False,\n                    \"buildType\": \"production\",\n                    \"espIdfVersion\": \"ESP-IDF v5.5.5\",\n                    \"appImageSha256\": \"b\" * 64,\n                    \"appElfSha256\": \"a\" * 64,\n                    \"diagnosticsBuildId\": \"a\" * 39,\n                }\n            ),\n            encoding=\"utf-8\",\n        )\n        manifest = MODULE.load_flash_manifest(manifest_path)\n        MODULE.verify_firmware_provenance(manifest, {\"buildId\": \"a\" * 39})\n        expect_failure(\n            MODULE.verify_firmware_provenance,\n            manifest,\n            {\"buildId\": \"d\" * 39},\n        )\n\n        dirty = json.loads(manifest_path.read_text(encoding=\"utf-8\"))\n        dirty[\"gitDirty\"] = True\n        manifest_path.write_text(json.dumps(dirty), encoding=\"utf-8\")\n        expect_failure(MODULE.load_flash_manifest, manifest_path)\n",
)
replace_once(
    "tests/scripts/test-v2-035-hardware.py",
    "    test_diagnostics_schema()\n",
    "    test_diagnostics_schema()\n    test_flash_manifest_provenance()\n",
)

# Update the physical runbook to build, flash, and pass the exact manifest.
replace_once(
    "docs/implementation-v2/V2_035_HARDWARE_EVIDENCE_HARNESS_2026-08-06.md",
    "1. Flash the production firmware from the exact commit being validated.\n2. Complete first-run provisioning and connect the test computer to the board.\n3. Confirm `GET /api/v1/status` is reachable through the production network\n   path.\n4. Use ESP-IDF v5.5.5 for the destructive partition stage.\n5. Keep the working state outside the repository until finalization.\n",
    "1. Check out the exact clean commit being validated and source ESP-IDF v5.5.5.\n2. Build the production firmware and generate `firmware/build/flash-manifest.json`\n   with `scripts/generate-flash-manifest.sh`. The manifest must report\n   `gitDirty: false` and `buildType: production`.\n3. Flash the application image represented by that exact manifest.\n4. Complete first-run provisioning and connect the test computer to the board.\n5. Confirm `GET /api/v1/status` is reachable through the production network\n   path.\n6. Keep the working state outside the repository until finalization.\n\nExample build provenance commands:\n\n```bash\ngit status --short\nidf.py -C firmware build\nbash scripts/generate-flash-manifest.sh\npython3 -m json.tool firmware/build/flash-manifest.json\n```\n\nThe collector rejects dirty or development manifests and refuses to start if\nthe board's 39-character diagnostics `buildId` differs from the ELF SHA prefix\nrecorded by `esptool.py image_info` for the manifest's application image. This\ncomparison happens before any V2-035 blob is created or deleted.\n",
)
replace_once(
    "docs/implementation-v2/V2_035_HARDWARE_EVIDENCE_HARNESS_2026-08-06.md",
    "export FIRMWARE_SHA=\"$(git rev-parse HEAD)\"\n",
    "export FLASH_MANIFEST=\"${PWD}/firmware/build/flash-manifest.json\"\n",
)
replace_once(
    "docs/implementation-v2/V2_035_HARDWARE_EVIDENCE_HARNESS_2026-08-06.md",
    "  --firmware-sha \"${FIRMWARE_SHA}\" \\\n",
    "  --flash-manifest \"${FLASH_MANIFEST}\" \\\n",
)
insert_after(
    "docs/implementation-v2/V2_035_HARDWARE_EVIDENCE_HARNESS_2026-08-06.md",
    "- Final evidence is bound to an exact 40-character firmware commit and the\n  `ESP32-S3R8` target.\n",
    "- The exact commit is accepted only from a clean production flash manifest;\n  the manifest's application-image SHA-256, full ELF SHA-256, 39-character\n  diagnostics build ID, and manifest SHA-256 are preserved in the evidence.\n",
)

print("V2-035 provenance hardening patch applied")
