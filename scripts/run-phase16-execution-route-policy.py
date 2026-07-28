#!/usr/bin/env python3
"""Repair the known two-case confirm matcher, then run the execution transform."""

from pathlib import Path
import runpy

root = Path(__file__).resolve().parents[1]
script = root / "scripts/apply-phase16-execution-route-policy.py"
text = script.read_text(encoding="utf-8")
old = '''replace_once(
    "firmware/components/web_server/web_api_core.c",
    "    case WEB_API_ROUTE_EXECUTION_CONFIRM:\\n",
    "",
)
'''
new = '''core_path = Path("firmware/components/web_server/web_api_core.c")
core_text = core_path.read_text(encoding="utf-8")
confirm_case = "    case WEB_API_ROUTE_EXECUTION_CONFIRM:\\n"
if core_text.count(confirm_case) != 2:
    raise SystemExit(
        f"web_api_core.c: expected two confirm cases, found {core_text.count(confirm_case)}"
    )
core_path.write_text(core_text.replace(confirm_case, ""), encoding="utf-8")
'''
if text.count(old) != 1:
    raise SystemExit("execution route policy confirm-case matcher changed unexpectedly")
script.write_text(text.replace(old, new, 1), encoding="utf-8")
runpy.run_path(str(script), run_name="__main__")
