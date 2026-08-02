"""Minimal HTTP client for the ESP32 macro keyboard's API.

The session cookie is the whole credential (SPEC 16.2). The CSRF token and the
Host/Origin pair this client used to send were removed with the checks that
required them.
"""

import json
import urllib.error
import urllib.request

import hil_state


class Device:
    def __init__(self, ip=None):
        self.ip = ip or hil_state.device_ip()
        self.base = f"http://{self.ip}"
        self.cookie = None

    def _request(self, method, path, body=None, raw=False):
        headers = {}
        data = None
        if body is not None:
            data = json.dumps(body).encode()
            headers["Content-Type"] = "application/json"
        if self.cookie:
            headers["Cookie"] = self.cookie

        request = urllib.request.Request(
            f"{self.base}{path}", data=data, headers=headers, method=method
        )
        try:
            with urllib.request.urlopen(request, timeout=45) as response:
                payload = response.read().decode()
                set_cookie = response.headers.get("Set-Cookie")
                status = response.status
        except urllib.error.HTTPError as error:
            payload = error.read().decode()
            set_cookie = error.headers.get("Set-Cookie")
            status = error.code

        if set_cookie and "=" in set_cookie:
            token = set_cookie.split(";")[0]
            if not token.endswith("="):
                self.cookie = token

        if raw:
            return status, payload
        try:
            return status, json.loads(payload)
        except json.JSONDecodeError:
            return status, payload

    def get(self, path, **kw):
        return self._request("GET", path, **kw)

    def post(self, path, body=None, **kw):
        return self._request("POST", path, body, **kw)

    def put(self, path, body=None, **kw):
        return self._request("PUT", path, body, **kw)

    def delete(self, path, body=None, **kw):
        return self._request("DELETE", path, body, **kw)

    def logout(self):
        """Release the session.

        Sessions are RAM-only and the table is bounded (APP_SESSION_TABLE_MAX),
        so a harness that logs in on every run without logging out will exhaust
        it and every later login fails 503. The firmware is right to refuse;
        the caller has to clean up after itself.
        """
        if self.cookie is None:
            return
        try:
            self.post("/api/v1/auth/logout")
        except Exception:
            pass
        self.cookie = None

    def __enter__(self):
        self.login()
        return self

    def __exit__(self, *exc):
        self.logout()
        return False

    def login(self):
        password = hil_state.admin_password()
        status, payload = self.post("/api/v1/auth/login", {"password": password})
        if status != 200:
            raise SystemExit(f"login failed: HTTP {status} {payload}")
        status, payload = self.get("/api/v1/auth/session")
        if status != 200:
            raise SystemExit(f"session fetch failed: HTTP {status} {payload}")
        return payload.get("data", payload)
