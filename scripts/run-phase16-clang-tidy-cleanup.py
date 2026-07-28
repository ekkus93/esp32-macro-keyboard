#!/usr/bin/env python3
"""Run the Phase 16 cleanup after replacing two indentation-sensitive matchers."""

from pathlib import Path
import runpy

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts/fix-phase16-clang-tidy.py"
text = SCRIPT.read_text(encoding="utf-8")
old = '''replace_once(
    "firmware/components/web_server/web_api_json.h",
    "app_error_code_t web_api_json_parse_resource_mutation(const char *body, size_t body_length,\\n"
    "                                                       size_t maximum_resource_length,\\n"
    "                                                       web_api_resource_mutation_t *out_mutation);\\n",
    "app_error_code_t web_api_json_parse_resource_mutation(\\n"
    "    const char *body, const web_api_resource_parse_limits_t *limits,\\n"
    "    web_api_resource_mutation_t *out_mutation);\\n",
)
replace_once(
    "firmware/components/web_server/web_api_json.h",
    "app_error_code_t web_api_json_parse_uuid_order(const char *body, size_t body_length,\\n"
    "                                                size_t maximum_count,\\n"
    "                                                storage_uuid_order_t *out_order);\\n",
    "app_error_code_t web_api_json_parse_uuid_order(\\n"
    "    const char *body, const web_api_order_parse_limits_t *limits,\\n"
    "    storage_uuid_order_t *out_order);\\n",
)
'''
new = '''replace_regex_once(
    "firmware/components/web_server/web_api_json.h",
    r"app_error_code_t web_api_json_parse_resource_mutation\\(.*?out_mutation\\);\\n",
    "app_error_code_t web_api_json_parse_resource_mutation(\\n"
    "    const char *body, const web_api_resource_parse_limits_t *limits,\\n"
    "    web_api_resource_mutation_t *out_mutation);\\n",
)
replace_regex_once(
    "firmware/components/web_server/web_api_json.h",
    r"app_error_code_t web_api_json_parse_uuid_order\\(.*?out_order\\);\\n",
    "app_error_code_t web_api_json_parse_uuid_order(\\n"
    "    const char *body, const web_api_order_parse_limits_t *limits,\\n"
    "    storage_uuid_order_t *out_order);\\n",
)
'''
if text.count(old) != 1:
    raise SystemExit("Phase 16 cleanup script declaration matcher block changed unexpectedly")
SCRIPT.write_text(text.replace(old, new, 1), encoding="utf-8")
runpy.run_path(str(SCRIPT), run_name="__main__")
