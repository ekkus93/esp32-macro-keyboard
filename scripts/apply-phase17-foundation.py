#!/usr/bin/env python3
"""Apply the Phase 17 authenticated frontend foundation deterministically."""

from __future__ import annotations

import base64
import io
import tarfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ARCHIVE = """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[... ELLIPSIZATION ...]n    "    TEST_CHECK(strstr(json, csrf_token) != NULL);\n"
    '    TEST_CHECK(strstr(json, "sessionToken") == NULL);\n'
    "    web_api_handler_json_free(json);\n"
    "}\n\n"
    "int main(void) {\n"
    "    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_lock_init());",
    "session response redaction test",
)
replace_once(
    "tests/host/test_web_api_repository_handlers.c",
    "    reset_store();\n    test_set_routes();",
    "    reset_store();\n    test_session_json_redaction();\n    test_set_routes();",
    "session encoder test invocation",
)

replace_once(
    "docs/API.md",
    "| POST | `/api/v1/device/factory-reset` | Factory reset and restart |\n\n"
    "Password change, settings reset, restart, and factory reset require physical",
    "| POST | `/api/v1/device/factory-reset` | Factory reset and restart |\n\n"
    "`GET /api/v1/auth/session` returns `authenticated: true` plus the current CSRF token so a\n"
    "same-origin page reload can restore the RAM-only frontend token. It never returns the\n"
    "HttpOnly session token, and all API responses use `Cache-Control: no-store`.\n\n"
    "Password change, settings reset, restart, and factory reset require physical",
    "session CSRF restore API documentation",
)

# Synchronize Phase 17 evidence without claiming later editor/workflow slices.
todo_path = ROOT / "docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md"
todo = todo_path.read_text(encoding="utf-8")
def todo_replace(old: str, new: str, description: str) -> None:
    global todo
    count = todo.count(old)
    if count != 1:
        raise SystemExit(f"expected one {description}, found {count}")
    todo = todo.replace(old, new, 1)

todo_replace(
    "Keep `App.tsx` responsible for routing, session boundary, and global shell only.\n",
    "Keep `App.tsx` responsible for routing, session boundary, and global shell only.\n\n"
    "**Implemented foundation:** feature-owned setup/authentication, set selection, settings, and "
    "execution views now sit behind a small routing and shell coordinator.\n",
    "Phase 17.1 evidence",
)
todo_replace(
    "Do not use `value.data as T` without route-specific validation.\n",
    "Do not use `value.data as T` without route-specific validation.\n\n"
    "**Implemented foundation:** `apiRequest` requires a route validator; exact guards cover setup, "
    "status, session, settings, sets, cancellation, and execution payloads. Invalid 2xx payloads "
    "fail closed as `invalid_response`.\n",
    "Phase 17.2 evidence",
)
for item in (
    "load status/session on startup;",
    "redirect unprovisioned devices to setup;",
    "redirect expired sessions to login;",
    "clear CSRF token on logout or 401;",
    "display rate-limit retry time;",
    "stop authenticated polling after session expiry.",
    "load sets;",
    "recents;",
    "search;",
    "metadata;",
    "explicit active set;",
    "no hardcoded HP model;",
    "active set shown in the header from server state.",
):
    todo_replace(f"- [ ] {item}", f"- [x] {item}", f"{item} checkbox")
todo_replace(
    "Update `webapp/tests/app-execution.test.tsx` so cancellation expects\n"
    "`Macro cancelled`, not `Macro finished`.\n",
    "Update `webapp/tests/app-execution.test.tsx` so cancellation expects\n"
    "`Macro cancelled`, not `Macro finished`.\n\n"
    "**Implemented:** terminal labels are exhaustive for completed, cancelled, failed, timed out, "
    "and key-release failure. Polling stops on terminal state, route exit, unmount, or session "
    "expiry.\n",
    "Phase 17.8 evidence",
)
todo_replace("- [ ] settings;", "- [x] settings;", "Phase 17.9 settings checkbox")
todo_path.write_text(todo, encoding="utf-8")

progress_path = ROOT / "docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_PROGRESS.md"
progress = progress_path.read_text(encoding="utf-8")
count = progress.count("| 17 | Replace frontend mock behavior | not started |")
if count != 1:
    raise SystemExit(f"expected one Phase 17 progress row, found {count}")
progress = progress.replace(
    "| 17 | Replace frontend mock behavior | not started |",
    "| 17 | Replace frontend mock behavior | in progress (17.1-17.4, 17.8, and settings complete) |",
    1,
)
marker = "## Completed tasks (commit evidence)\n\n"
if progress.count(marker) != 1:
    raise SystemExit("unexpected completed-task marker count")
progress = progress.replace(
    marker,
    marker
    + "- Phase 17 foundation (split shell, runtime response validation, setup/login/logout and "
      "reload-safe session lifecycle, live set selection, settings, and execution-result semantics) "
      "— complete in the commit containing this progress update. The authenticated session route "
      "restores only the CSRF token after reload; the HttpOnly session token remains undisclosed. "
      "Invalid response data fails closed, 401 clears CSRF and stops polling, login throttling is "
      "visible, the hardcoded model/set is removed, and deferred Phase 18/19 functions are explicit "
      "rather than inert.\n\n",
    1,
)
progress_path.write_text(progress, encoding="utf-8")

print("Phase 17 authenticated frontend foundation applied")
