"""The device HTTP client and the response/diagnostics parsers."""

from __future__ import annotations

import gzip
import http.client
import http.cookiejar
import json
import urllib.error
import urllib.parse
import urllib.request
from typing import Any

from .core import (
    AUTH_LOGIN_PATH,
    BLOB_COLLECTION_PATH,
    DIAGNOSTICS_PATH,
    EvidenceError,
    LIVE_BUILD_ID_PATTERN,
    REQUIRED_SCENARIOS,
    require,
)
from .evidence import sha256_bytes, utc_now


def parse_success(body: bytes) -> Any:
    # SPEC_V2 13 success responses are the flat object itself - no v1-style
    # {"ok":true,"data":...} envelope (see web_api_response.c). The caller is
    # responsible for only invoking this on a genuinely successful HTTP status.
    try:
        payload = json.loads(body.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise EvidenceError(f"device returned invalid JSON: {error}") from error
    require(isinstance(payload, dict), "device response must be a JSON object")
    return payload


class DeviceApi:
    def __init__(self, base_url: str, timeout: float = 15.0) -> None:
        parsed = urllib.parse.urlparse(base_url.rstrip("/"))
        require(parsed.scheme in ("http", "https"), "base URL must use http or https")
        require(bool(parsed.hostname), "base URL must include a host")
        self.base_url = base_url.rstrip("/")
        self.timeout = timeout
        self.cookies = http.cookiejar.CookieJar()
        self.opener = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(self.cookies))

    def _request(self, method: str, path: str, body: bytes | None = None,
                 headers: dict[str, str] | None = None) -> tuple[int, bytes, Any]:
        request = urllib.request.Request(
            self.base_url + path,
            data=body,
            headers=headers or {},
            method=method,
        )
        try:
            with self.opener.open(request, timeout=self.timeout) as response:
                return response.status, response.read(), response.headers
        except urllib.error.HTTPError as error:
            return error.code, error.read(), error.headers
        except OSError as error:
            raise EvidenceError(f"device request failed: {method} {path}: {error}") from error

    def login(self, password: str) -> None:
        body = json.dumps({"adminPassword": password}, separators=(",", ":")).encode()
        status, response, _ = self._request(
            "POST", AUTH_LOGIN_PATH, body, {"Content-Type": "application/json"}
        )
        require(status == 200, f"login failed with HTTP {status}: {response!r}")
        parse_success(response)
        require(any(True for _ in self.cookies), "login succeeded without setting a session cookie")

    def list_blobs(self) -> list[dict[str, Any]]:
        status, body, _ = self._request("GET", BLOB_COLLECTION_PATH)
        require(status == 200, f"blob list failed with HTTP {status}: {body!r}")
        data = parse_success(body)
        require(isinstance(data, dict) and isinstance(data.get("blobs"), list),
                "blob list response has the wrong schema")
        entries = data["blobs"]
        for entry in entries:
            require(isinstance(entry, dict) and isinstance(entry.get("id"), str),
                    "blob list contains an invalid entry")
        return entries

    def create_blob(self, payload: bytes) -> tuple[int, Any]:
        status, body, _ = self._request(
            "POST", BLOB_COLLECTION_PATH, payload, {"Content-Type": "application/gzip"}
        )
        if 200 <= status < 300:
            return status, parse_success(body)
        try:
            parsed: Any = json.loads(body.decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError):
            parsed = {"raw": body.hex()}
        return status, parsed

    def load_blob(self, blob_id: str) -> bytes:
        status, body, _ = self._request("GET", f"{BLOB_COLLECTION_PATH}/{blob_id}")
        require(status == 200, f"blob {blob_id} load failed with HTTP {status}")
        return body

    def delete_blob(self, blob_id: str) -> None:
        status, body, _ = self._request("DELETE", f"{BLOB_COLLECTION_PATH}/{blob_id}")
        require(status in (200, 204),
                f"blob {blob_id} delete failed with HTTP {status}: {body!r}")

    def diagnostics(self) -> Any:
        status, body, _ = self._request("GET", DIAGNOSTICS_PATH)
        require(status == 200, f"diagnostics failed with HTTP {status}: {body!r}")
        return parse_success(body)

    def cookie_header(self, path: str) -> str:
        request = urllib.request.Request(self.base_url + path)
        self.cookies.add_cookie_header(request)
        cookie = request.get_header("Cookie")
        require(bool(cookie), "authenticated request has no session cookie")
        return str(cookie)


def blob_ids(api: DeviceApi) -> list[str]:
    return [entry["id"] for entry in api.list_blobs()]


def snapshot(api: DeviceApi) -> dict[str, str]:
    result: dict[str, str] = {}
    for blob_id in blob_ids(api):
        require(blob_id not in result, f"duplicate blob ID {blob_id}")
        result[blob_id] = sha256_bytes(api.load_blob(blob_id))
    return result


def verify_snapshot(api: DeviceApi, expected: dict[str, str], exact_ids: bool = False) -> None:
    actual_ids = blob_ids(api)
    if exact_ids:
        require(set(actual_ids) == set(expected),
                f"blob ID set changed: expected {sorted(expected)}, found {actual_ids}")
    for blob_id, expected_hash in expected.items():
        actual_hash = sha256_bytes(api.load_blob(blob_id))
        require(actual_hash == expected_hash, f"blob {blob_id} changed byte-for-byte")


def add_scenario(state: dict[str, Any], name: str, details: dict[str, Any]) -> None:
    scenarios = state.setdefault("scenarios", {})
    require(name in REQUIRED_SCENARIOS, f"unknown V2-035 scenario {name}")
    scenarios[name] = {"status": "pass", "observedAt": utc_now(), **details}


def parse_diagnostics(diagnostics: Any) -> dict[str, Any]:
    require(isinstance(diagnostics, dict), "diagnostics data must be an object")
    build_id = diagnostics.get("buildId")
    reset_reason = diagnostics.get("resetReason")
    uptime_ms = diagnostics.get("uptimeMs")
    # The real contract has no "blobScan" object and no separate count field
    # (see contracts/v2/api/examples.json's "diagnostics" example): temporary
    # files live at storage.temporaryFiles, and the count is just its length.
    storage = diagnostics.get("storage")
    require(isinstance(build_id, str)
            and LIVE_BUILD_ID_PATTERN.fullmatch(build_id) is not None,
            "diagnostics buildId must be a lowercase ELF SHA prefix")
    require(isinstance(reset_reason, str) and bool(reset_reason),
            "diagnostics resetReason is invalid")
    require(type(uptime_ms) is int and uptime_ms >= 0, "diagnostics uptimeMs is invalid")
    require(isinstance(storage, dict), "diagnostics storage is invalid")
    temporary_files = storage.get("temporaryFiles")
    require(isinstance(temporary_files, list)
            and all(isinstance(value, str) for value in temporary_files),
            "diagnostics storage.temporaryFiles is invalid")
    return {
        "buildId": build_id,
        "resetReason": reset_reason,
        "uptimeMs": uptime_ms,
        "temporaryFileCount": len(temporary_files),
        "temporaryFiles": temporary_files,
    }


