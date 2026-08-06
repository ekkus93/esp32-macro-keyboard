#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def write(path: str, text: str) -> None:
    (ROOT / path).write_text(text, encoding="utf-8")


def replace_once(path: str, old: str, new: str) -> None:
    text = read(path)
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{path}: expected one match, found {count}: {old[:100]!r}")
    write(path, text.replace(old, new, 1))


def replace_block(path: str, start: str, end: str, replacement: str) -> None:
    text = read(path)
    if text.count(start) != 1:
        raise SystemExit(f"{path}: start anchor cardinality drifted: {start!r}")
    start_index = text.index(start)
    end_index = text.find(end, start_index)
    if end_index < 0:
        raise SystemExit(f"{path}: end anchor not found after {start!r}: {end!r}")
    write(path, text[:start_index] + replacement + text[end_index:])


def insert_before(path: str, anchor: str, addition: str) -> None:
    replace_once(path, anchor, addition + anchor)


SCRIPT = "scripts/run-v2-035-hardware.py"
TEST = "tests/scripts/test-v2-035-hardware.py"
RUNBOOK = "docs/implementation-v2/V2_035_HARDWARE_EVIDENCE_HARNESS_2026-08-06.md"

replace_once(
    SCRIPT,
    "import re\nimport socket\nimport ssl\n",
    "import re\nimport shutil\nimport socket\nimport ssl\nimport subprocess\n",
)
replace_once(
    SCRIPT,
    "STATE_SCHEMA = 2\nFIRMWARE_COMMIT_PATTERN = re.compile(r\"^[0-9a-f]{40}$\")\n",
    "STATE_SCHEMA = 3\nESP_IDF_VERSION = \"ESP-IDF v5.5.5\"\n"
    "FIRMWARE_COMMIT_PATTERN = re.compile(r\"^[0-9a-f]{40}$\")\n",
)
replace_once(
    SCRIPT,
    "BUILD_ID_PATTERN = re.compile(r\"^[0-9a-f]{39}$\")\n",
    "BUILD_ID_PATTERN = re.compile(r\"^[0-9a-f]{39}$\")\n"
    "ELF_SHA_OUTPUT_PATTERN = re.compile(r\"ELF file SHA256:\\\\s*([0-9A-Fa-f]{64})\")\n",
)

insert_before(
    SCRIPT,
    "def deterministic_bytes(label: str, length: int) -> bytes:\n",
    '''def resolve_manifest_app_image(manifest_path: Path, manifest: dict[str, Any]) -> Path:
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
            [esptool, "image_info", str(app_image)],
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


''',
)

replace_block(
    SCRIPT,
    "def load_flash_manifest(path: Path) -> dict[str, str]:\n",
    "def verify_firmware_provenance",
    '''def load_flash_manifest(path: Path) -> dict[str, str]:
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


''',
)

insert_before(
    SCRIPT,
    "def load_state(path: Path, expected_phase: str | None = None) -> dict[str, Any]:\n",
    '''def owned_blobs(state: dict[str, Any]) -> list[dict[str, str]]:
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


''',
)

replace_block(
    SCRIPT,
    "def command_start(args: argparse.Namespace) -> None:\n",
    "def api_from_state",
    '''def command_start(args: argparse.Namespace) -> None:
    require(not args.state.exists(), f"refusing to overwrite existing state {args.state}")
    manifest = load_flash_manifest(args.flash_manifest)
    api = DeviceApi(args.base_url, args.timeout)
    api.login(password_from_environment(args.password_env))
    initial_diagnostics = parse_diagnostics(api.diagnostics())
    verify_firmware_provenance(manifest, initial_diagnostics)
    baseline = snapshot(api)

    state: dict[str, Any] = {
        "schemaVersion": STATE_SCHEMA,
        "task": "V2-035",
        "createdAt": utc_now(),
        "baseUrl": args.base_url.rstrip("/"),
        "firmwareCommit": manifest["gitCommit"],
        "firmwareBuildId": manifest["diagnosticsBuildId"],
        "appImage": manifest["appImage"],
        "appImageSha256": manifest["appImageSha256"],
        "appElfSha256": manifest["appElfSha256"],
        "flashManifestSha256": manifest["flashManifestSha256"],
        "espIdfVersion": manifest["espIdfVersion"],
        "targetHardware": "ESP32-S3R8",
        "phase": "start_in_progress",
        "baseline": baseline,
        "sentinels": [],
        "ownedBlobs": [],
        "startCreated": [],
        "initialDiagnostics": initial_diagnostics,
        "scenarios": {},
    }
    write_json(args.state, state)

    created: list[dict[str, str]] = []
    for label in ("ordering-a", "ordering-b", "ordering-c"):
        payload = small_payload(label)
        status, data = api.create_blob(payload)
        require(status == 201 and isinstance(data, dict) and isinstance(data.get("id"), str),
                f"blob creation returned HTTP {status}: {data!r}")
        item = {"id": data["id"], "sha256": sha256_bytes(payload)}
        created.append(item)
        journal_created_blob(args.state, state, item, "startCreated")

    numeric = [int(item["id"]) for item in created]
    require(numeric == sorted(numeric) and len(set(numeric)) == 3,
            f"created IDs are not strictly increasing: {numeric}")
    listed = blob_ids(api)
    listed_numeric = [int(value) for value in listed]
    require(listed_numeric == sorted(listed_numeric, reverse=True),
            f"blob list is not newest-first numeric order: {listed}")
    for item in created:
        require(sha256_bytes(api.load_blob(item["id"])) == item["sha256"],
                f"new blob {item['id']} did not round-trip byte-identically")

    removed = created[1]
    delete_owned_blob(api, state, args.state, removed)
    expected = dict(baseline)
    for item in (created[0], created[2]):
        expected[item["id"]] = item["sha256"]
    verify_snapshot(api, expected, exact_ids=True)

    state["sentinels"] = [created[0], created[2]]
    state.pop("startCreated", None)
    add_scenario(state, "numeric_ordering", {"listedIds": listed, "createdIds": [x["id"] for x in created]})
    add_scenario(state, "delete_preservation", {"deletedId": removed["id"], "preservedHashes": expected})
    state["phase"] = "awaiting_power_cycle"
    write_json(args.state, state)
    print(f"PASS: ordering and deletion evidence written to {args.state}")
    print("Physically remove power from the ESP32-S3, restore power, wait for Wi-Fi, then run verify-power-cycle.")


''',
)

insert_before(
    SCRIPT,
    "def command_verify_power_cycle(args: argparse.Namespace) -> None:\n",
    '''def owned_snapshot(state: dict[str, Any]) -> dict[str, str]:
    return {item["id"]: item["sha256"] for item in owned_blobs(state)}


def delete_owned_blob(api: DeviceApi, state: dict[str, Any], state_path: Path,
                      item: dict[str, str]) -> None:
    current_ids = set(blob_ids(api))
    if item["id"] in current_ids:
        require(sha256_bytes(api.load_blob(item["id"])) == item["sha256"],
                f"collector-owned blob {item['id']} changed before cleanup")
        api.delete_blob(item["id"])
    journal_deleted_blob(state_path, state, item["id"])


def verify_recoverable_snapshot(api: DeviceApi, state: dict[str, Any]) -> None:
    baseline = state.get("baseline")
    require(isinstance(baseline, dict), "evidence baseline is invalid")
    collector = owned_snapshot(state)
    actual_ids = set(blob_ids(api))
    allowed_ids = set(baseline) | set(collector)
    unknown = actual_ids - allowed_ids
    require(not unknown,
            f"refusing recovery because unowned blob IDs appeared: {sorted(unknown)}")
    for blob_id, expected_hash in baseline.items():
        require(blob_id in actual_ids, f"baseline blob {blob_id} disappeared")
        require(sha256_bytes(api.load_blob(blob_id)) == expected_hash,
                f"baseline blob {blob_id} changed byte-for-byte")
    for blob_id, expected_hash in collector.items():
        if blob_id in actual_ids:
            require(sha256_bytes(api.load_blob(blob_id)) == expected_hash,
                    f"collector-owned blob {blob_id} changed byte-for-byte")


def command_recover_cleanup(args: argparse.Namespace) -> None:
    state = load_state(args.state)
    require(state.get("phase") != "complete", "complete evidence cannot be recovered or discarded")
    api = api_from_state(state, args)
    verify_recoverable_snapshot(api, state)
    for item in list(reversed(owned_blobs(state))):
        delete_owned_blob(api, state, args.state, item)
    verify_snapshot(api, state["baseline"], exact_ids=True)
    state["phase"] = "recovered"
    state["recoveredAt"] = utc_now()
    write_json(args.state, state)
    try:
        args.state.unlink()
    except OSError as error:
        raise EvidenceError(f"cleanup succeeded but state file could not be removed: {error}") from error
    print("PASS: collector-owned blobs were removed and the original baseline was restored")


''',
)

replace_block(
    SCRIPT,
    "def command_fill_storage(args: argparse.Namespace) -> None:\n",
    "def require_image",
    '''def command_fill_storage(args: argparse.Namespace) -> None:
    state = load_state(args.state, "ready_for_storage_full")
    api = api_from_state(state, args)
    original = snapshot(api)
    require(original == expected_live_snapshot(state), "live blobs changed before storage-full test")
    payload = exact_gzip_payload("storage-full")
    state["phase"] = "storage_full_in_progress"
    state["fillCreated"] = []
    write_json(args.state, state)

    fill_created: list[dict[str, str]] = []
    failure: Any = None
    for attempt in range(1, 9):
        status, data = api.create_blob(payload)
        if status == 201:
            require(isinstance(data, dict) and isinstance(data.get("id"), str),
                    "successful fill upload returned an invalid response")
            item = {"id": data["id"], "sha256": sha256_bytes(payload)}
            fill_created.append(item)
            journal_created_blob(args.state, state, item, "fillCreated")
            continue
        require(status == 507, f"within-limit storage exhaustion returned HTTP {status}: {data!r}")
        failure = {"attempt": attempt, "httpStatus": status, "response": data}
        break
    require(failure is not None, "storage never returned HTTP 507 within eight maximum-size uploads")
    preserved = dict(original)
    for item in fill_created:
        preserved[item["id"]] = item["sha256"]
    verify_snapshot(api, preserved, exact_ids=True)
    for item in reversed(fill_created):
        delete_owned_blob(api, state, args.state, item)
    verify_snapshot(api, original, exact_ids=True)
    add_scenario(
        state,
        "storage_full_507_preservation",
        {
            "maximumUploadBytes": len(payload),
            "committedBefore507": fill_created,
            "failure": failure,
            "preservedHashes": preserved,
            "cleanupVerified": True,
        },
    )
    state.pop("fillCreated", None)
    state["phase"] = "ready_for_mount_failure_record"
    write_json(args.state, state)
    print("PASS: HTTP 507 observed and every committed final blob remained byte-identical")
    print("Run the documented backed-up userdata corruption procedure, then record-mount-failure.")


''',
)

replace_once(
    SCRIPT,
    "    require(state.get(\"phase\") in (\"ready_to_finalize\", \"complete\"),\n            f\"evidence phase is incomplete: {state.get('phase')!r}\")\n",
    "    require(state.get(\"phase\") in (\"ready_to_finalize\", \"finalize_in_progress\", \"complete\"),\n"
    "            f\"evidence phase is incomplete: {state.get('phase')!r}\")\n"
    "    recorded_owned = state.get(\"ownedBlobs\")\n"
    "    require(isinstance(recorded_owned, list), \"evidence is missing the owned-blob journal\")\n"
    "    if state.get(\"phase\") == \"complete\":\n"
    "        require(recorded_owned == [], \"complete evidence still owns test blobs\")\n",
)

replace_block(
    SCRIPT,
    "def command_finalize(args: argparse.Namespace) -> None:\n",
    "def command_validate",
    '''def command_finalize(args: argparse.Namespace) -> None:
    state = load_state(args.state)
    require(state.get("phase") in ("ready_to_finalize", "finalize_in_progress"),
            f"cannot finalize from phase {state.get('phase')!r}")
    api = api_from_state(state, args)
    if state["phase"] == "ready_to_finalize":
        validate_complete_state(state)
        verify_snapshot(api, expected_live_snapshot(state), exact_ids=True)
        state["phase"] = "finalize_in_progress"
        write_json(args.state, state)
    else:
        expected = dict(state["baseline"])
        expected.update(owned_snapshot(state))
        verify_snapshot(api, expected, exact_ids=True)

    for item in list(reversed(owned_blobs(state))):
        delete_owned_blob(api, state, args.state, item)
    verify_snapshot(api, state["baseline"], exact_ids=True)
    state["phase"] = "complete"
    state["completedAt"] = utc_now()
    state["testBlobCleanup"] = {"status": "pass", "verifiedAt": utc_now()}
    validate_complete_state(state)
    write_json(args.state, state)
    output = dict(state)
    output["evidenceSha256"] = sha256_bytes(
        json.dumps(state, sort_keys=True, separators=(",", ":")).encode()
    )
    write_json(args.output, output)
    print(f"PASS: complete V2-035 evidence written to {args.output}")


''',
)

insert_before(
    SCRIPT,
    "    finalize = subparsers.add_parser(\"finalize\")\n",
    '''    recover = subparsers.add_parser(
        "recover-cleanup",
        help="remove journaled collector blobs after a failed or interrupted stage",
    )
    add_online_arguments(recover)
    recover.set_defaults(function=command_recover_cleanup)

''',
)

replace_once(TEST, "import importlib.util\n", "import importlib.util\nimport os\nimport subprocess\n")
replace_once(
    TEST,
    '        "targetHardware": "ESP32-S3R8",\n        "phase": "ready_to_finalize",\n',
    '        "targetHardware": "ESP32-S3R8",\n        "espIdfVersion": MODULE.ESP_IDF_VERSION,\n'
    '        "ownedBlobs": [],\n        "phase": "ready_to_finalize",\n',
)

replace_block(
    TEST,
    "def test_flash_manifest_provenance() -> None:\n",
    "def test_interrupted_upload_request_headers",
    '''def test_flash_manifest_provenance() -> None:
    with tempfile.TemporaryDirectory() as temporary_directory:
        root = Path(temporary_directory)
        manifest_path = root / "flash-manifest.json"
        app_image = root / "esp32_macro_keyboard.bin"
        app_image.write_bytes(b"verified application image")
        image_sha256 = MODULE.sha256_file(app_image)
        manifest_path.write_text(
            json.dumps(
                {
                    "gitCommit": "0123456789abcdef0123456789abcdef01234567",
                    "gitDirty": False,
                    "buildType": "production",
                    "espIdfVersion": MODULE.ESP_IDF_VERSION,
                    "appImage": app_image.name,
                    "appImageSha256": image_sha256,
                    "appElfSha256": "a" * 64,
                    "diagnosticsBuildId": "a" * 39,
                }
            ),
            encoding="utf-8",
        )
        original_which = MODULE.shutil.which
        original_run = MODULE.subprocess.run
        MODULE.shutil.which = lambda name: "/fake/esptool.py" if name == "esptool.py" else None
        MODULE.subprocess.run = lambda *args, **kwargs: subprocess.CompletedProcess(
            args=args[0], returncode=0, stdout=f"ELF file SHA256: {'a' * 64}\n", stderr=""
        )
        try:
            manifest = MODULE.load_flash_manifest(manifest_path)
            MODULE.verify_firmware_provenance(manifest, {"buildId": "a" * 39})
            expect_failure(
                MODULE.verify_firmware_provenance,
                manifest,
                {"buildId": "d" * 39},
            )

            dirty = json.loads(manifest_path.read_text(encoding="utf-8"))
            dirty["gitDirty"] = True
            manifest_path.write_text(json.dumps(dirty), encoding="utf-8")
            expect_failure(MODULE.load_flash_manifest, manifest_path)

            dirty["gitDirty"] = False
            dirty["espIdfVersion"] = "ESP-IDF v5.5.4"
            manifest_path.write_text(json.dumps(dirty), encoding="utf-8")
            expect_failure(MODULE.load_flash_manifest, manifest_path)

            dirty["espIdfVersion"] = MODULE.ESP_IDF_VERSION
            app_image.write_bytes(b"tampered application image")
            manifest_path.write_text(json.dumps(dirty), encoding="utf-8")
            expect_failure(MODULE.load_flash_manifest, manifest_path)
        finally:
            MODULE.shutil.which = original_which
            MODULE.subprocess.run = original_run


''',
)

insert_before(
    TEST,
    "def main() -> int:\n",
    '''class FakeRecoveryApi:
    def __init__(self, blobs: dict[str, bytes], fail_delete_once: str | None = None) -> None:
        self.blobs = dict(blobs)
        self.fail_delete_once = fail_delete_once

    def list_blobs(self) -> list[dict]:
        return [
            {"id": blob_id, "sizeBytes": len(self.blobs[blob_id])}
            for blob_id in sorted(self.blobs, key=int, reverse=True)
        ]

    def load_blob(self, blob_id: str) -> bytes:
        if blob_id not in self.blobs:
            raise MODULE.EvidenceError(f"missing blob {blob_id}")
        return self.blobs[blob_id]

    def delete_blob(self, blob_id: str) -> None:
        if self.fail_delete_once == blob_id:
            self.fail_delete_once = None
            raise MODULE.EvidenceError(f"injected delete failure for {blob_id}")
        self.blobs.pop(blob_id, None)


def test_recover_cleanup_uses_owned_journal() -> None:
    with tempfile.TemporaryDirectory() as temporary_directory:
        root = Path(temporary_directory)
        state_path = root / "state.json"
        baseline_payload = b"baseline"
        owned_payload = b"collector"
        state = {
            "schemaVersion": MODULE.STATE_SCHEMA,
            "task": "V2-035",
            "phase": "start_in_progress",
            "baseUrl": "http://device.test",
            "baseline": {"0000000001": MODULE.sha256_bytes(baseline_payload)},
            "ownedBlobs": [
                {"id": "0000000002", "sha256": MODULE.sha256_bytes(owned_payload)}
            ],
        }
        state_path.write_text(json.dumps(state), encoding="utf-8")
        fake = FakeRecoveryApi(
            {"0000000001": baseline_payload, "0000000002": owned_payload}
        )
        original_api_from_state = MODULE.api_from_state
        MODULE.api_from_state = lambda current, args: fake
        try:
            MODULE.command_recover_cleanup(
                Namespace(state=state_path, timeout=1.0, password_env="UNUSED")
            )
        finally:
            MODULE.api_from_state = original_api_from_state
        assert fake.blobs == {"0000000001": baseline_payload}
        assert not state_path.exists()


def test_finalize_resumes_partial_cleanup() -> None:
    with tempfile.TemporaryDirectory() as temporary_directory:
        root = Path(temporary_directory)
        state_path = root / "state.json"
        output_path = root / "evidence.json"
        baseline_payload = b"baseline"
        first_payload = b"first"
        second_payload = b"second"
        first = {"id": "0000000002", "sha256": MODULE.sha256_bytes(first_payload)}
        second = {"id": "0000000003", "sha256": MODULE.sha256_bytes(second_payload)}
        state = complete_state()
        state.update(
            {
                "baseUrl": "http://device.test",
                "baseline": {"0000000001": MODULE.sha256_bytes(baseline_payload)},
                "sentinels": [first, second],
                "ownedBlobs": [first, second],
            }
        )
        state_path.write_text(json.dumps(state), encoding="utf-8")
        fake = FakeRecoveryApi(
            {
                "0000000001": baseline_payload,
                "0000000002": first_payload,
                "0000000003": second_payload,
            },
            fail_delete_once="0000000002",
        )
        args = Namespace(
            state=state_path,
            output=output_path,
            timeout=1.0,
            password_env="UNUSED",
        )
        original_api_from_state = MODULE.api_from_state
        MODULE.api_from_state = lambda current, current_args: fake
        try:
            expect_failure(MODULE.command_finalize, args)
            partial = json.loads(state_path.read_text(encoding="utf-8"))
            assert partial["phase"] == "finalize_in_progress"
            assert partial["ownedBlobs"] == [first]
            MODULE.command_finalize(args)
        finally:
            MODULE.api_from_state = original_api_from_state
        complete = json.loads(state_path.read_text(encoding="utf-8"))
        assert complete["phase"] == "complete"
        assert complete["ownedBlobs"] == []
        assert fake.blobs == {"0000000001": baseline_payload}
        MODULE.command_validate(Namespace(evidence=output_path))


''',
)
replace_once(
    TEST,
    "    test_mount_failure_record()\n",
    "    test_mount_failure_record()\n"
    "    test_recover_cleanup_uses_owned_journal()\n"
    "    test_finalize_resumes_partial_cleanup()\n",
)

replace_once(
    RUNBOOK,
    "The collector rejects dirty or development manifests and refuses to start if\n"
    "the board's 39-character diagnostics `buildId` differs from the ELF SHA prefix\n"
    "recorded by `esptool.py image_info` for the manifest's application image. This\n"
    "comparison happens before any V2-035 blob is created or deleted.\n",
    "The collector rejects dirty or development manifests, requires exactly ESP-IDF\n"
    "v5.5.5, hashes the application image named by the manifest, reruns `esptool.py\n"
    "image_info` to verify the full ELF SHA-256, and refuses to start if the board's\n"
    "39-character diagnostics `buildId` differs from that verified ELF SHA prefix.\n"
    "These checks happen before any V2-035 blob is created or deleted.\n",
)
insert_before(
    RUNBOOK,
    "## Stage 1 — Numeric ordering and deletion preservation\n",
    '''## Recovery after a failed mutating stage

The collector writes its state before the first mutation and journals every
collector-owned blob immediately after creation. If `start`, `fill-storage`, or
`finalize` fails or the host process is interrupted, do not delete IDs by hand.
Run:

```bash
python3 scripts/run-v2-035-hardware.py recover-cleanup \\
  --state "${V2_035_STATE}"
```

Recovery verifies every baseline hash, refuses to touch unowned IDs, verifies
each surviving collector-owned blob before deletion, tolerates an owned blob
that was already deleted immediately before a host crash, restores the exact
pre-test blob set, and only then removes the local state file. After recovery,
restart V2-035 from Stage 1 with a newly generated state path.

''',
)

print("V2-035 recovery and provenance hardening applied")
