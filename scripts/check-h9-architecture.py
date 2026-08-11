#!/usr/bin/env python3
"""Fail closed on H9 safety-architecture regressions."""

from pathlib import Path
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


lifecycle = read("firmware/components/web_server/web_server_lifecycle.c")
if "(void)httpd_stop(handle)" in lifecycle:
    fail("httpd cleanup result is discarded after async-worker start failure")
if "*out_handle = handle;" not in lifecycle:
    fail("failed web-server start cannot preserve a still-live httpd handle")

adapter = read("firmware/components/web_server/web_server_adapter_lifecycle.c")
for required in (
    "const int start_result = ops->start(ops->context, &handle);",
    "if (start_result != 0)",
    "lifecycle->handle = handle;",
    "lifecycle->cleanup_error = APP_ERROR_IO;",
):
    if required not in adapter:
        fail(f"web lifecycle lost failed-start ownership guard: {required}")

async_source = read("firmware/components/web_server/web_server_async.c")
worker_unavailable = "if (async_queue == NULL || async_task_handle == NULL)"
if worker_unavailable not in async_source:
    fail("async confirmation dispatch no longer has an explicit worker-unavailable gate")
start = async_source.index(worker_unavailable)
end = async_source.find("if (!claim_in_flight())", start)
if end < 0:
    fail("could not bound worker-unavailable confirmation branch")
branch = async_source[start:end]
for required in ("WEB_HTTP_STATUS_SERVICE_UNAVAILABLE", "confirmation service unavailable"):
    if required not in branch:
        fail(f"worker-unavailable confirmation path no longer fails closed: {required}")
if "web_api_handle_call" in branch:
    fail("worker-unavailable confirmation path reintroduced synchronous handler fallback")

web_send_h = read("firmware/components/web_server/web_send.h")
web_send = read("firmware/components/web_server/web_send.c")
server_send = read("firmware/components/web_server/web_server_send.c")
for required in (
    "get_require_confirmation",
    "bool *out_required",
):
    if required not in web_send_h:
        fail(f"send boundary no longer requires confirmation policy callback: {required}")
for required in (
    "ops->get_require_confirmation(ops->context, &require_confirmation)",
    ".require_confirmation = require_confirmation",
    "WEB_SEND_CREATE_BACKEND_UNAVAILABLE",
):
    if required not in web_send:
        fail(f"send construction no longer binds/fail-closes confirmation policy: {required}")
for required in (
    '#include "device_settings.h"',
    "device_settings_read(&settings)",
    "settings.require_serial_confirmation",
    ".get_require_confirmation = confirmation_policy_adapter",
    "memset(&settings, 0, sizeof(settings))",
):
    if required not in server_send:
        fail(f"real send adapter no longer reads/clears authoritative confirmation settings: {required}")

print("H9 architecture guard passed")
