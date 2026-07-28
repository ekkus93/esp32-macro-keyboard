#!/usr/bin/env python3
"""Repair policy transform matchers and execute only outstanding hardening."""

from pathlib import Path
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

SCRIPT.write_text(text, encoding="utf-8")
runpy.run_path(str(SCRIPT), run_name="__main__")
