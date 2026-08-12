#!/usr/bin/env python3
"""Fail closed on H2 password-transaction architecture regressions."""

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    print(f"error: {message}", file=sys.stderr)
    raise SystemExit(1)


def read(relative: str) -> str:
    path = ROOT / relative
    if not path.is_file():
        fail(f"required source is missing: {relative}")
    return path.read_text(encoding="utf-8", errors="replace")


def join_adjacent_c_string_literals(source: str) -> str:
    """Normalize clang-format line-splitting of adjacent C string literals.

    clang-format may turn one long literal into adjacent literals such as
    ``"... new " "password"``. Architecture checks should validate the
    semantic message, not a particular source layout.
    """
    return re.sub(r'"\s*"', "", source)


app_error_h = read("firmware/components/macro_model/include/app_error.h")
if "APP_ERROR_AUTH_STATE_INCOMPLETE" not in app_error_h:
    fail("partial credential/session commit lost its stable app error")
if app_error_h.index("APP_ERROR_AUTH_STATE_INCOMPLETE") < app_error_h.index("APP_ERROR_INTERNAL"):
    fail("H2 app error must remain appended so existing numeric error values do not change")

administration = read("firmware/components/web_server/web_api_administration.c")
if "refresh_password_record_cache" in administration:
    fail("best-effort password-record NVS refresh was reintroduced")
for required in (
    "web_server_password_transition_begin()",
    "web_server_password_record_replace(&record)",
    "web_server_password_transition_end()",
    ".password_transition_begin = settings_ops_password_transition_begin",
    ".password_activate = settings_ops_password_activate",
    ".password_transition_end = settings_ops_password_transition_end",
    "WEB_CHANGE_PASSWORD_COMMITTED_SESSION_INVALIDATION_INCOMPLETE",
    "APP_ERROR_AUTH_STATE_INCOMPLETE",
    "password change already in progress",
):
    if required not in administration:
        fail(f"H2 production change-password binding is missing: {required}")

partial_commit_message = (
    "password changed; session invalidation incomplete; sign in with the new password"
)
if partial_commit_message not in join_adjacent_c_string_literals(administration):
    fail(f"H2 production change-password binding is missing: {partial_commit_message}")

settings = read("firmware/components/web_server/web_settings.c")
ordered = (
    "ops->password_transition_begin(ops->context)",
    "ops->settings_read(ops->context, &current)",
    "ops->password_verify(ops->context, current_password_view.data",
    "ops->password_create(",
    "ops->settings_replace(ops->context, &candidate, &changed)",
    "ops->password_activate(ops->context, &candidate)",
    "ops->invalidate_all_sessions(ops->context)",
)
handler_start = settings.find("web_change_password_outcome_t web_change_password_handle")
if handler_start < 0:
    fail("H2 change-password handler is missing")
positions = []
for required in ordered:
    position = settings.find(required, handler_start)
    if position < 0:
        fail(f"H2 transaction step is missing: {required}")
    positions.append(position)
if positions != sorted(positions):
    fail("H2 transaction order changed: gate -> read/verify/create -> durable commit -> RAM activate -> sessions")
for required in (
    "secure_zero_local(&material, sizeof(material))",
    "secure_zero_local(&candidate, sizeof(candidate))",
    "secure_zero_local(&current, sizeof(current))",
    "WEB_CHANGE_PASSWORD_COMMITTED_SESSION_INVALIDATION_INCOMPLETE",
):
    if required not in settings:
        fail(f"H2 secret/partial-commit cleanup guard is missing: {required}")

password_record = read("firmware/components/web_server/web_server_password_record.c")
for required in (
    "password_transition_in_progress",
    "web_server_password_record_snapshot_for_login",
    "APP_ERROR_AUTH_STATE_INCOMPLETE",
    "web_server_password_transition_begin",
    "web_server_password_transition_end",
):
    if required not in password_record:
        fail(f"H2 login transition gate is missing: {required}")

login = read("firmware/components/web_server/web_server_login.c")
for required in (
    "web_server_password_record_snapshot_for_login(&password_record)",
    '"503 Service Unavailable"',
    '"credential transition in progress"',
):
    if required not in login:
        fail(f"login no longer fails closed during password transition: {required}")

server_api = read("firmware/components/web_server/web_server_api.c")
for required in (
    "WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD",
    "WEB_HTTP_STATUS_NO_CONTENT",
    "WEB_HTTP_STATUS_CONFLICT",
    "Max-Age=0",
):
    if required not in server_api:
        fail(f"H2 current-cookie invalidation guard is missing: {required}")

transaction_test = read("tests/host/test_web_change_password_transaction.c")
for required in (
    "test_success_is_immediately_coherent",
    "test_precommit_failures_leave_old_authority_and_retry_cleanly",
    "test_postcommit_invalidation_failure_names_new_authority_and_retry_semantics",
):
    if required not in transaction_test:
        fail(f"H2 transaction regression coverage is missing: {required}")

print("H2 password transaction architecture guard passed")
