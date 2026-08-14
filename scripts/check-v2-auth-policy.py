#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(relative: str) -> str:
    return (ROOT / relative).read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"V2 auth policy check failed: {message}")


def uint32_macro(source: str, name: str) -> int:
    match = re.search(
        rf"^#define\s+{re.escape(name)}\s+UINT32_C\((\d+)\)\s*$",
        source,
        re.MULTILINE,
    )
    require(match is not None, f"missing {name}")
    return int(match.group(1))


def unsigned_macro(source: str, name: str) -> int:
    match = re.search(
        rf"^#define\s+{re.escape(name)}\s+(\d+)U\s*$",
        source,
        re.MULTILINE,
    )
    require(match is not None, f"missing {name}")
    return int(match.group(1))


def uint64_seconds_macro(source: str, name: str) -> int:
    match = re.search(
        rf"^#define\s+{re.escape(name)}\s+\(UINT64_C\((\d+)\)\s+\*\s+UINT64_C\(1000000\)\)\s*$",
        source,
        re.MULTILINE,
    )
    require(match is not None, f"missing {name}")
    return int(match.group(1))


def main() -> None:
    limits = read("firmware/components/app_contracts_v2/include/app_limits_v2.h")
    macro_limits = read("firmware/components/macro_model/include/macro_limits.h")
    auth_header = read("firmware/components/auth/include/auth.h")
    auth_core_header = read("firmware/components/auth/auth_core.h")
    auth_internal = read("firmware/components/auth/auth_core_internal.h")
    auth_session = read("firmware/components/auth/auth_core_session.c")
    auth_rate = read("firmware/components/auth/auth_core_rate_limit.c")
    login = read("firmware/components/web_server/web_server_login.c")
    logout = read("firmware/components/web_server/web_server_logout.c")
    cookie = read("firmware/components/web_server/web_cookie.c")
    session_tests = read("tests/host/auth_additional_session_tests.inc")
    rate_tests = read("tests/host/auth_additional_rate_tests.inc")
    spec = read("docs/SPEC_V2.md")
    api_current = read("docs/API.md").split("## Archived: retired v1 API", maxsplit=1)[0]
    web_server_readme = read("firmware/components/web_server/README.md")

    active_sessions = uint32_macro(limits, "APP_V2_ACTIVE_SESSIONS_MAX")
    idle_seconds = uint32_macro(limits, "APP_V2_SESSION_IDLE_LIFETIME_SECONDS")
    absolute_seconds = uint32_macro(limits, "APP_V2_SESSION_ABSOLUTE_LIFETIME_SECONDS")
    require(active_sessions == 8, "active session limit must remain 8")
    require(idle_seconds == 86400, "idle session lifetime must remain 86400 seconds")
    require(absolute_seconds == 604800, "absolute session lifetime must remain 604800 seconds")
    require(
        uint32_macro(macro_limits, "APP_SESSION_TABLE_MAX") == active_sessions,
        "production session table must match the centralized V2 active-session limit",
    )
    require(
        unsigned_macro(macro_limits, "APP_SESSION_TOKEN_BYTES") == 32,
        "session token entropy must remain exactly 32 random bytes",
    )

    require(
        uint64_seconds_macro(auth_internal, "AUTH_CORE_SESSION_IDLE_US") == idle_seconds,
        "auth core idle lifetime drifted from the centralized V2 limit",
    )
    require(
        uint64_seconds_macro(auth_internal, "AUTH_CORE_SESSION_ABSOLUTE_US") == absolute_seconds,
        "auth core absolute lifetime drifted from the centralized V2 limit",
    )
    require(
        uint64_seconds_macro(auth_internal, "AUTH_CORE_FAILURE_WINDOW_US") == 60,
        "failed-login rolling window must remain 60 seconds",
    )
    require(
        uint64_seconds_macro(auth_internal, "AUTH_CORE_LOCKOUT_US") == 300,
        "login lockout must remain 300 seconds",
    )

    require(
        "#define AUTH_RATE_LIMIT_SOURCE_MAX 8U" in auth_core_header,
        "per-source rate-limit table must stay explicitly bounded",
    )
    require(
        "#define AUTH_RATE_LIMIT_FAILURE_MAX 5U" in auth_core_header,
        "failed-login threshold must remain five",
    )
    require(
        "uint8_t failures_us[" not in auth_core_header,
        "failure timestamps must retain uint64_t precision",
    )
    require(
        "uint64_t failures_us[AUTH_RATE_LIMIT_FAILURE_MAX]" in auth_core_header,
        "per-source rolling failure timestamps are missing",
    )

    require(
        "APP_SESSION_TOKEN_BYTES * 2U" in auth_header,
        "session token must remain the hex encoding of APP_SESSION_TOKEN_BYTES",
    )
    require(
        "uint32_t source_ipv4" in auth_header,
        "public login-throttle API must remain source-aware",
    )
    require(
        "absolute_expires_at_us" in auth_header,
        "session view must expose the absolute expiry internally",
    )

    require(
        "least_recently_used" in auth_session,
        "ninth-session behavior must remain deterministic LRU replacement",
    )
    require(
        "APP_ERROR_CONFLICT" not in auth_session,
        "full session table must not reject the ninth successful login",
    )
    require(
        "auth_core_restore_state(core, &snapshot);" in auth_session,
        "failed session replacement must preserve prior session state",
    )
    require(
        "AUTH_CORE_SESSION_ABSOLUTE_US" in auth_session,
        "session absolute expiry enforcement is missing",
    )

    require(
        "find_entry(core, source_ipv4)" in auth_rate,
        "rate limiting must remain keyed by source address",
    )
    require(
        "AUTH_CORE_FAILURE_WINDOW_US" in auth_rate,
        "rolling failure-window enforcement is missing",
    )
    require(
        "AUTH_CORE_LOCKOUT_US" in auth_rate,
        "five-minute lockout enforcement is missing",
    )
    require(
        "least_recent_unlocked" in auth_rate,
        "bounded throttle table must not evict active lockouts",
    )

    require(
        "getpeername(" in login and "peer.ss_family != AF_INET" in login,
        "login must fail closed when an IPv4 source address cannot be identified",
    )
    require(
        "auth_login_attempt_allowed(source_ipv4" in login,
        "login handler is not using source-aware throttling",
    )
    require(
        "auth_login_record_failure(source_ipv4)" in login,
        "failed login is not recorded against its source",
    )
    require(
        "auth_login_record_success(source_ipv4)" in login,
        "successful login is not clearing only its source throttle",
    )
    require(
        "HttpOnly; SameSite=Strict; Path=/" in cookie,
        "login cookie attributes drifted from V2 policy",
    )
    require(
        "There is no separate CSRF token. There is no Host/Origin check" in spec,
        "normative development-appliance CSRF/Host/Origin policy is missing",
    )
    require(
        re.search(r"no\s+separate CSRF token", api_current) is not None
        and "no `Host`/`Origin` check" in api_current,
        "current API documentation contradicts the V2 development-appliance auth policy",
    )
    require(
        "matching CSRF token" not in api_current and "same-origin security checks" not in api_current,
        "current API documentation still describes the retired CSRF/origin policy",
    )
    require(
        re.search(r"no\s+separate CSRF", web_server_readme) is not None
        and "no `Host`/`Origin` check" in web_server_readme,
        "web-server documentation contradicts the V2 development-appliance auth policy",
    )
    require(
        "same-origin security checks" not in web_server_readme,
        "web-server documentation still claims an origin check that production code does not perform",
    )
    require(
        "Secure" not in login and "Secure" not in cookie,
        "login cookie must not claim Secure while the device serves plain HTTP",
    )

    require(
        "auth_session_logout(session_token)" in logout,
        "logout must invalidate the server-side session",
    )
    require(
        "HttpOnly; SameSite=Strict; Path=/; Max-Age=0" in logout,
        "logout cookie clearing attributes drifted from V2 policy",
    )
    require(
        "Secure" not in logout,
        "logout cookie must not claim Secure while the device serves plain HTTP",
    )

    require(
        "rebooted_core" in session_tests,
        "host coverage must prove RAM-only sessions disappear on reboot/reinit",
    )
    require(
        "sessions_before_failure" in session_tests,
        "host coverage must prove failed LRU replacement preserves existing sessions",
    )
    require(
        "source_b" in rate_tests,
        "host coverage must prove per-source rate-limit isolation",
    )
    require(
        "AUTH_RATE_LIMIT_SOURCE_MAX" in rate_tests,
        "host coverage must exercise bounded throttle-table capacity",
    )

    print("V2 authentication policy checks passed")


if __name__ == "__main__":
    main()
