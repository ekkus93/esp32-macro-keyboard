from pathlib import Path


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected one match, found {count}")
    return text.replace(old, new, 1)


script_path = Path("scripts/run-v2-035-hardware.py")
script = script_path.read_text(encoding="utf-8")
script = replace_once(
    script,
    "import os\nimport socket\n",
    "import os\nimport re\nimport socket\n",
    "add re import",
)
script = replace_once(
    script,
    "STATE_SCHEMA = 1\nREQUIRED_SCENARIOS = (\n",
    'STATE_SCHEMA = 1\nFIRMWARE_COMMIT_PATTERN = re.compile(r"^[0-9a-f]{40}$")\nREQUIRED_SCENARIOS = (\n',
    "add firmware pattern",
)
script = replace_once(
    script,
    "    api.login(password_from_environment(args.password_env))\n"
    "    baseline = snapshot(api)\n\n"
    "    created: list[dict[str, str]] = []\n",
    '    api.login(password_from_environment(args.password_env))\n'
    '    firmware_commit = args.firmware_sha.strip().lower()\n'
    '    require(\n'
    '        FIRMWARE_COMMIT_PATTERN.fullmatch(firmware_commit) is not None,\n'
    '        "firmware SHA must be exactly 40 hexadecimal characters",\n'
    '    )\n'
    '    baseline = snapshot(api)\n\n'
    '    created: list[dict[str, str]] = []\n',
    "bind start to firmware",
)
script = replace_once(
    script,
    '        "baseUrl": args.base_url.rstrip("/"),\n'
    '        "phase": "awaiting_power_cycle",\n',
    '        "baseUrl": args.base_url.rstrip("/"),\n'
    '        "firmwareCommit": firmware_commit,\n'
    '        "targetHardware": "ESP32-S3R8",\n'
    '        "phase": "awaiting_power_cycle",\n',
    "persist provenance",
)
script = replace_once(
    script,
    '    connection.putrequest("POST", path)\n',
    '    connection.putrequest("POST", path, skip_host=True)\n',
    "avoid duplicate host",
)
script = replace_once(
    script,
    'def validate_complete_state(state: dict[str, Any]) -> None:\n'
    '    scenarios = state.get("scenarios")\n',
    'def validate_complete_state(state: dict[str, Any]) -> None:\n'
    '    firmware_commit = state.get("firmwareCommit")\n'
    '    require(\n'
    '        isinstance(firmware_commit, str)\n'
    '        and FIRMWARE_COMMIT_PATTERN.fullmatch(firmware_commit) is not None,\n'
    '        "evidence is not bound to an exact firmware commit",\n'
    '    )\n'
    '    require(\n'
    '        state.get("targetHardware") == "ESP32-S3R8",\n'
    '        "evidence target hardware is not ESP32-S3R8",\n'
    '    )\n'
    '    scenarios = state.get("scenarios")\n',
    "validate provenance",
)
script = replace_once(
    script,
    '    start.add_argument("--base-url", required=True)\n'
    '    add_online_arguments(start)\n',
    '    start.add_argument("--base-url", required=True)\n'
    '    start.add_argument("--firmware-sha", required=True)\n'
    '    add_online_arguments(start)\n',
    "add firmware cli",
)
script_path.write_text(script, encoding="utf-8")


test_path = Path("tests/scripts/test-v2-035-hardware.py")
test = test_path.read_text(encoding="utf-8")
test = replace_once(
    test,
    '        "task": "V2-035",\n'
    '        "phase": "ready_to_finalize",\n',
    '        "task": "V2-035",\n'
    '        "firmwareCommit": "0123456789abcdef0123456789abcdef01234567",\n'
    '        "targetHardware": "ESP32-S3R8",\n'
    '        "phase": "ready_to_finalize",\n',
    "test provenance fixture",
)
request_test = '''


def test_interrupted_upload_request_headers() -> None:
    class FakeApi:
        base_url = "http://192.0.2.1:8080/base"
        timeout = 3.0

        @staticmethod
        def cookie_header(path: str) -> str:
            assert path == "/api/v1/blobs"
            return "session=test"

    class FakeConnection:
        def __init__(self, host: str, port: int, timeout: float) -> None:
            self.host = host
            self.port = port
            self.timeout = timeout
            self.requests = []
            self.headers = []
            self.ended = False

        def putrequest(self, method: str, path: str, **kwargs) -> None:
            self.requests.append((method, path, kwargs))

        def putheader(self, name: str, value: str) -> None:
            self.headers.append((name, value))

        def endheaders(self) -> None:
            self.ended = True

    original = MODULE.http.client.HTTPConnection
    MODULE.http.client.HTTPConnection = FakeConnection
    try:
        connection = MODULE.open_upload_connection(FakeApi())
    finally:
        MODULE.http.client.HTTPConnection = original

    assert connection.requests == [
        ("POST", "/base/api/v1/blobs", {"skip_host": True})
    ]
    host_headers = [
        value for name, value in connection.headers if name.lower() == "host"
    ]
    assert host_headers == ["192.0.2.1:8080"]
    assert ("Content-Length", str(MODULE.BLOB_MAX_BYTES)) in connection.headers
    assert connection.ended is True
'''
test = replace_once(
    test,
    "\ndef test_mount_failure_record() -> None:\n",
    request_test + "\ndef test_mount_failure_record() -> None:\n",
    "insert request regression",
)
test = replace_once(
    test,
    "    test_complete_validation()\n    test_mount_failure_record()\n",
    "    test_complete_validation()\n"
    "    test_interrupted_upload_request_headers()\n"
    "    test_mount_failure_record()\n",
    "run request regression",
)
test_path.write_text(test, encoding="utf-8")


report_path = Path(
    "docs/implementation-v2/V2_035_HARDWARE_EVIDENCE_HARNESS_2026-08-06.md"
)
report = report_path.read_text(encoding="utf-8")
report = replace_once(
    report,
    "status or diagnostic state, and refuses to finalize unless all seven physical\n"
    "scenarios have a passing observation.",
    "status or diagnostic state, binds the record to the exact 40-character firmware\n"
    "commit supplied at the first stage, and refuses to finalize unless all seven\n"
    "physical scenarios have a passing observation.",
    "document provenance",
)
report = replace_once(
    report,
    "export DEVICE_URL='http://192.168.4.1'\n",
    "export DEVICE_URL='http://192.168.4.1'\n"
    'export FIRMWARE_SHA="$(git rev-parse HEAD)"\n',
    "document firmware sha env",
)
report = replace_once(
    report,
    '  --base-url "${DEVICE_URL}" \\\n'
    '  --state "${V2_035_STATE}"\n',
    '  --base-url "${DEVICE_URL}" \\\n'
    '  --firmware-sha "${FIRMWARE_SHA}" \\\n'
    '  --state "${V2_035_STATE}"\n',
    "document firmware sha cli",
)
report_path.write_text(report, encoding="utf-8")
