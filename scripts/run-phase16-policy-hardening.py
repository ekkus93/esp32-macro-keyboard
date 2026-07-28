#!/usr/bin/env python3
"""Repair policy transform matchers and execute only outstanding hardening."""

from pathlib import Path
import re
import runpy

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts/apply-phase16-policy-hardening.py"
text = SCRIPT.read_text(encoding="utf-8")
old = '''replace_once(
    "firmware/components/auth/auth_core.h",
    "app_error_code_t auth_core_session_validate(auth_core_t *core, const char *session_token,\\n"
    "                                             const char *csrf_token);\\n",
    "app_error_code_t auth_core_session_validate(auth_core_t *core, const char *session_token,\\n"
    "                                             const char *csrf_token);\\n"
    "app_error_code_t auth_core_session_validate_read_only(auth_core_t *core,\\n"
    "                                                       const char *session_token);\\n",
)
'''
new = '''replace_regex_once(
    "firmware/components/auth/auth_core.h",
    r"app_error_code_t auth_core_session_validate\\(auth_core_t \\*core,.*?const char \\*csrf_token\\);\\n",
    "app_error_code_t auth_core_session_validate(auth_core_t *core, const char *session_token,\\n"
    "                                             const char *csrf_token);\\n"
    "app_error_code_t auth_core_session_validate_read_only(auth_core_t *core,\\n"
    "                                                       const char *session_token);\\n",
)
'''
if text.count(old) != 1:
    raise SystemExit("Phase 16 auth-core declaration matcher changed unexpectedly")
text = text.replace(old, new, 1)

route_start = text.find("# Restrict methods accurately and ensure execution submission enters confirmation policy.")
route_end = text.find("# Core route regression coverage.", route_start)
if route_start < 0 or route_end < 0:
    raise SystemExit("Phase 16 route-policy transform block changed unexpectedly")
text = text[:route_start] + text[route_end:]

auth_test_start = text.find("# Authentication-core coverage for read-only validation and token rejection.")
auth_test_end = text.find('print("Phase 16 request-policy hardening applied")', auth_test_start)
if auth_test_start < 0 or auth_test_end < 0:
    raise SystemExit("Phase 16 auth-test transform block changed unexpectedly")
text = text[:auth_test_start] + text[auth_test_end:]
SCRIPT.write_text(text, encoding="utf-8")
runpy.run_path(str(SCRIPT), run_name="__main__")

auth_test = ROOT / "tests/host/auth_existing_tests.inc"
auth_text = auth_test.read_text(encoding="utf-8")
pattern = (
    r"(TEST_CHECK\(auth_core_session_validate\(&core,\s*session\.session_token,\s*"
    r"session\.csrf_token\) == APP_ERROR_NONE\);)(\s*fake\.now_us \+= 1000000U;)"
)
replacement = (
    r"\1\n    TEST_CHECK(auth_core_session_validate_read_only(&core, session.session_token) ==\n"
    r"               APP_ERROR_NONE);\n\2"
)
auth_text, count = re.subn(pattern, replacement, auth_text, count=1)
if count != 1:
    raise SystemExit(f"auth_existing_tests.inc: read-only success insertion matched {count}")
pattern = (
    r'(TEST_CHECK\(auth_core_session_validate\(&core, "short", wrong\) == '
    r'APP_ERROR_AUTH_REQUIRED\);)'
)
replacement = (
    r"\1\n    TEST_CHECK(auth_core_session_validate_read_only(&core, wrong) == APP_ERROR_AUTH_REQUIRED);\n"
    r"    TEST_CHECK(auth_core_session_validate_read_only(NULL, session.session_token) ==\n"
    r"               APP_ERROR_AUTH_REQUIRED);"
)
auth_text, count = re.subn(pattern, replacement, auth_text, count=1)
if count != 1:
    raise SystemExit(f"auth_existing_tests.inc: read-only rejection insertion matched {count}")
auth_test.write_text(auth_text, encoding="utf-8")
