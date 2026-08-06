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
    'FIRMWARE_COMMIT_PATTERN = re.compile(r"^[0-9a-f]{40}$")\nREQUIRED_SCENARIOS = (\n',
    'FIRMWARE_COMMIT_PATTERN = re.compile(r"^[0-9a-f]{40}$")\n'
    'AUTH_LOGIN_PATH = "/api/v1/auth/login"\n'
    'BLOB_COLLECTION_PATH = "/api/v1/blob"\n'
    'DIAGNOSTICS_PATH = "/api/v1/diagnostics"\n'
    'REQUIRED_SCENARIOS = (\n',
    "add current v2 route constants",
)
script = replace_once(
    script,
    '            "POST", "/api/v1/login", body, {"Content-Type": "application/json"}\n',
    '            "POST", AUTH_LOGIN_PATH, body, {"Content-Type": "application/json"}\n',
    "fix login route",
)
script = replace_once(
    script,
    '        status, body, _ = self._request("GET", "/api/v1/blobs")\n',
    '        status, body, _ = self._request("GET", BLOB_COLLECTION_PATH)\n',
    "fix blob list route",
)
script = replace_once(
    script,
    '            "POST", "/api/v1/blobs", payload, {"Content-Type": "application/gzip"}\n',
    '            "POST", BLOB_COLLECTION_PATH, payload, {"Content-Type": "application/gzip"}\n',
    "fix blob create route",
)
script = replace_once(
    script,
    '        status, body, _ = self._request("GET", f"/api/v1/blobs/{blob_id}")\n',
    '        status, body, _ = self._request("GET", f"{BLOB_COLLECTION_PATH}/{blob_id}")\n',
    "fix blob load route",
)
script = replace_once(
    script,
    '        status, body, _ = self._request("DELETE", f"/api/v1/blobs/{blob_id}")\n',
    '        status, body, _ = self._request("DELETE", f"{BLOB_COLLECTION_PATH}/{blob_id}")\n',
    "fix blob delete route",
)
script = replace_once(
    script,
    '    def status(self) -> Any:\n'
    '        status, body, _ = self._request("GET", "/api/v1/status")\n'
    '        require(status == 200, f"status failed with HTTP {status}: {body!r}")\n'
    '        return parse_success(body)\n',
    '    def diagnostics(self) -> Any:\n'
    '        status, body, _ = self._request("GET", DIAGNOSTICS_PATH)\n'
    '        require(status == 200, f"diagnostics failed with HTTP {status}: {body!r}")\n'
    '        return parse_success(body)\n',
    "use diagnostics endpoint",
)
script = replace_once(
    script,
    'def recursive_values(value: Any, key: str) -> list[Any]:\n'
    '    found: list[Any] = []\n'
    '    if isinstance(value, dict):\n'
    '        for child_key, child in value.items():\n'
    '            if child_key == key:\n'
    '                found.append(child)\n'
    '            found.extend(recursive_values(child, key))\n'
    '    elif isinstance(value, list):\n'
    '        for child in value:\n'
    '            found.extend(recursive_values(child, key))\n'
    '    return found\n',
    'def parse_diagnostics(diagnostics: Any) -> dict[str, Any]:\n'
    '    require(isinstance(diagnostics, dict), "diagnostics data must be an object")\n'
    '    build_id = diagnostics.get("buildId")\n'
    '    reset_reason = diagnostics.get("resetReason")\n'
    '    uptime_ms = diagnostics.get("uptimeMs")\n'
    '    blob_scan = diagnostics.get("blobScan")\n'
    '    require(isinstance(build_id, str) and bool(build_id), "diagnostics buildId is invalid")\n'
    '    require(isinstance(reset_reason, str) and bool(reset_reason),\n'
    '            "diagnostics resetReason is invalid")\n'
    '    require(type(uptime_ms) is int and uptime_ms >= 0, "diagnostics uptimeMs is invalid")\n'
    '    require(isinstance(blob_scan, dict), "diagnostics blobScan is invalid")\n'
    '    temporary_count = blob_scan.get("temporaryFileCount")\n'
    '    temporary_files = blob_scan.get("temporaryFiles")\n'
    '    require(type(temporary_count) is int and temporary_count >= 0,\n'
    '            "diagnostics temporaryFileCount is invalid")\n'
    '    require(isinstance(temporary_files, list)\n'
    '            and all(isinstance(value, str) for value in temporary_files),\n'
    '            "diagnostics temporaryFiles is invalid")\n'
    '    require(temporary_count == len(temporary_files),\n'
    '            "diagnostics temporary-file count does not match its list")\n'
    '    return {\n'
    '        "buildId": build_id,\n'
    '        "resetReason": reset_reason,\n'
    '        "uptimeMs": uptime_ms,\n'
    '        "temporaryFileCount": temporary_count,\n'
    '        "temporaryFiles": temporary_files,\n'
    '    }\n',
    "replace generic diagnostics search",
)
script = replace_once(
    script,
    '    baseline = snapshot(api)\n\n    created: list[dict[str, str]] = []\n',
    '    baseline = snapshot(api)\n'
    '    initial_diagnostics = parse_diagnostics(api.diagnostics())\n\n'
    '    created: list[dict[str, str]] = []\n',
    "capture initial diagnostics",
)
script = replace_once(
    script,
    '    require([int(value) for value in listed] == sorted(int(value) for value in listed),\n'
    '            f"blob list is not in numeric order: {listed}")\n',
    '    listed_numeric = [int(value) for value in listed]\n'
    '    require(listed_numeric == sorted(listed_numeric, reverse=True),\n'
    '            f"blob list is not newest-first numeric order: {listed}")\n',
    "enforce newest-first order",
)
script = replace_once(
    script,
    '        "sentinels": [created[0], created[2]],\n'
    '        "scenarios": {},\n',
    '        "sentinels": [created[0], created[2]],\n'
    '        "initialDiagnostics": initial_diagnostics,\n'
    '        "scenarios": {},\n',
    "persist initial diagnostics",
)
script = replace_once(
    script,
    '    verify_snapshot(api, expected, exact_ids=True)\n'
    '    add_scenario(state, "power_cycle_persistence", {"verifiedHashes": expected})\n',
    '    verify_snapshot(api, expected, exact_ids=True)\n'
    '    diagnostics = parse_diagnostics(api.diagnostics())\n'
    '    require(diagnostics["buildId"] == state["initialDiagnostics"]["buildId"],\n'
    '            "firmware build changed across the power-cycle stage")\n'
    '    require(diagnostics["resetReason"] == "power-on",\n'
    '            f"expected a physical power-on reset, found {diagnostics[\"resetReason\"]!r}")\n'
    '    add_scenario(state, "power_cycle_persistence",\n'
    '                 {"verifiedHashes": expected, "postBootDiagnostics": diagnostics})\n',
    "record power cycle diagnostics",
)
script = replace_once(
    script,
    '    path = (parsed.path.rstrip("/") if parsed.path else "") + "/api/v1/blobs"\n',
    '    path = (parsed.path.rstrip("/") if parsed.path else "") + BLOB_COLLECTION_PATH\n',
    "fix raw upload route",
)
script = replace_once(
    script,
    '    connection.putheader("Cookie", api.cookie_header("/api/v1/blobs"))\n',
    '    connection.putheader("Cookie", api.cookie_header(BLOB_COLLECTION_PATH))\n',
    "fix raw upload cookie route",
)
script = replace_once(
    script,
    '    before = snapshot(api)\n'
    '    require(before == expected_live_snapshot(state), "live blobs changed before interruption test")\n',
    '    before = snapshot(api)\n'
    '    require(before == expected_live_snapshot(state), "live blobs changed before interruption test")\n'
    '    before_diagnostics = parse_diagnostics(api.diagnostics())\n',
    "capture pre-interruption diagnostics",
)
script = replace_once(
    script,
    '        "beforeHashes": before,\n'
    '    }\n',
    '        "beforeHashes": before,\n'
    '        "beforeDiagnostics": before_diagnostics,\n'
    '    }\n',
    "persist pre-interruption diagnostics",
)
script = replace_once(
    script,
    '    status = api.status()\n'
    '    temporary_values = recursive_values(status, "temporaryFiles")\n'
    '    scan_failed_values = recursive_values(status, "scanFailed")\n'
    '    require(temporary_values and all(value == 0 for value in temporary_values),\n'
    '            f"diagnostics did not prove temporary-file cleanup: {temporary_values}")\n'
    '    require(not scan_failed_values or all(value is False for value in scan_failed_values),\n'
    '            f"diagnostics reported a failed storage scan: {scan_failed_values}")\n'
    '    details = {\n'
    '        "bytesSentBeforePowerLoss": state["interruptedUpload"]["bytesSentBeforeDisconnect"],\n'
    '        "preservedHashes": before,\n'
    '        "temporaryFiles": temporary_values,\n'
    '        "scanFailed": scan_failed_values,\n'
    '    }\n',
    '    diagnostics = parse_diagnostics(api.diagnostics())\n'
    '    before_diagnostics = state["interruptedUpload"]["beforeDiagnostics"]\n'
    '    require(diagnostics["buildId"] == before_diagnostics["buildId"],\n'
    '            "firmware build changed across interrupted-upload reboot")\n'
    '    require(diagnostics["resetReason"] == "power-on",\n'
    '            f"expected power-on reset after interruption, found {diagnostics[\"resetReason\"]!r}")\n'
    '    require(diagnostics["temporaryFileCount"] == 0\n'
    '            and diagnostics["temporaryFiles"] == [],\n'
    '            f"reboot left temporary files: {diagnostics}")\n'
    '    details = {\n'
    '        "bytesSentBeforePowerLoss": state["interruptedUpload"]["bytesSentBeforeDisconnect"],\n'
    '        "preservedHashes": before,\n'
    '        "prePowerLossDiagnostics": before_diagnostics,\n'
    '        "postBootDiagnostics": diagnostics,\n'
    '    }\n',
    "validate reboot diagnostics schema",
)
script_path.write_text(script, encoding="utf-8")

if "/api/v1/blobs" in script or '"/api/v1/login"' in script:
    raise SystemExit("legacy V2-035 API path remains in collector")


test_path = Path("tests/scripts/test-v2-035-hardware.py")
test = test_path.read_text(encoding="utf-8")
test = replace_once(
    test,
    '            assert path == "/api/v1/blobs"\n',
    '            assert path == MODULE.BLOB_COLLECTION_PATH\n',
    "fix cookie path expectation",
)
test = replace_once(
    test,
    '        ("POST", "/base/api/v1/blobs", {"skip_host": True})\n',
    '        ("POST", "/base/api/v1/blob", {"skip_host": True})\n',
    "fix raw path expectation",
)
new_tests = '''


def test_current_v2_routes() -> None:
    assert MODULE.AUTH_LOGIN_PATH == "/api/v1/auth/login"
    assert MODULE.BLOB_COLLECTION_PATH == "/api/v1/blob"
    assert MODULE.DIAGNOSTICS_PATH == "/api/v1/diagnostics"
    source = SCRIPT.read_text(encoding="utf-8")
    assert "/api/v1/blobs" not in source
    assert '"/api/v1/login"' not in source


def test_diagnostics_schema() -> None:
    parsed = MODULE.parse_diagnostics(
        {
            "buildId": "abc123",
            "resetReason": "power-on",
            "uptimeMs": 12,
            "blobScan": {"temporaryFileCount": 0, "temporaryFiles": []},
        }
    )
    assert parsed["temporaryFileCount"] == 0
    assert parsed["temporaryFiles"] == []

    MODULE.expect_failure = None
    expect_failure(
        MODULE.parse_diagnostics,
        {
            "buildId": "abc123",
            "resetReason": "power-on",
            "uptimeMs": 12,
            "blobScan": {"temporaryFileCount": 1, "temporaryFiles": []},
        },
    )
'''
test = replace_once(
    test,
    "\ndef test_interrupted_upload_request_headers() -> None:\n",
    new_tests + "\ndef test_interrupted_upload_request_headers() -> None:\n",
    "insert route and diagnostics tests",
)
test = replace_once(
    test,
    "    test_complete_validation()\n"
    "    test_interrupted_upload_request_headers()\n",
    "    test_complete_validation()\n"
    "    test_current_v2_routes()\n"
    "    test_diagnostics_schema()\n"
    "    test_interrupted_upload_request_headers()\n",
    "run route and diagnostics tests",
)
test_path.write_text(test, encoding="utf-8")


report_path = Path(
    "docs/implementation-v2/V2_035_HARDWARE_EVIDENCE_HARNESS_2026-08-06.md"
)
report = report_path.read_text(encoding="utf-8")
report = replace_once(
    report,
    "- requires the list endpoint to return numeric order;\n",
    "- requires the list endpoint to return newest-first numeric order;\n",
    "document newest-first order",
)
report = replace_once(
    report,
    "- diagnostics report `temporaryFiles == 0`; and\n"
    "- diagnostics do not report `scanFailed == true`.\n",
    "- `GET /api/v1/diagnostics` reports `blobScan.temporaryFileCount == 0`;\n"
    "- `blobScan.temporaryFiles` is empty; and\n"
    "- diagnostics report a physical `power-on` reset with the same build ID.\n",
    "document diagnostics contract",
)
insert_after = (
    "The collector does not check a V2-035 TODO item merely because a command ran.\n"
    "Each stage verifies byte identity with SHA-256, verifies the exact expected HTTP\n"
    "status or diagnostic state, binds the record to the exact 40-character firmware\n"
    "commit supplied at the first stage, and refuses to finalize unless all seven\n"
    "physical scenarios have a passing observation.\n"
)
report = replace_once(
    report,
    insert_after,
    insert_after
    + "\nThe HTTP client uses the current V2 production routes: `/api/v1/auth/login`,\n"
      "`/api/v1/blob`, `/api/v1/blob/{id}`, and `/api/v1/diagnostics`. Legacy\n"
      "plural blob paths are rejected by the collector regression suite.\n",
    "document current routes",
)
report_path.write_text(report, encoding="utf-8")
