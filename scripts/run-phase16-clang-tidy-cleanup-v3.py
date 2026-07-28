#!/usr/bin/env python3
"""Apply the final dispatcher cleanup, then run the structural Phase 16 cleanup."""

from pathlib import Path
import runpy

ROOT = Path(__file__).resolve().parents[1]
CLEANUP = ROOT / "scripts/fix-phase16-clang-tidy.py"
WRAPPER = ROOT / "scripts/run-phase16-clang-tidy-cleanup.py"
text = CLEANUP.read_text(encoding="utf-8")

old_dispatch = '''static app_error_code_t dispatch_api_call(const web_api_call_t *call,
                                          web_api_response_t *response,
                                          bool *out_response_ready) {
    app_error_code_t result = web_api_dispatch(call, response);
    if (result != APP_ERROR_NONE && response->body == NULL) {
        result = set_error_response(response, web_api_http_status_for_error(result), result,
                                    "API operation failed");
    }
    *out_response_ready = result == APP_ERROR_NONE && response->body != NULL;
    return result;
}
'''
new_dispatch = '''static bool dispatch_api_call(const web_api_call_t *call,
                              web_api_response_t *response) {
    app_error_code_t result = web_api_dispatch(call, response);
    if (result != APP_ERROR_NONE && response->body == NULL) {
        result = set_error_response(response, web_api_http_status_for_error(result), result,
                                    "API operation failed");
    }
    return result == APP_ERROR_NONE && response->body != NULL;
}
'''
old_call = '''    if (!response_ready && result == APP_ERROR_NONE) {
        result = dispatch_api_call(&call, &response, &response_ready);
    }
'''
new_call = '''    if (!response_ready && result == APP_ERROR_NONE) {
        response_ready = dispatch_api_call(&call, &response);
    }
'''
if text.count(old_dispatch) != 1:
    raise SystemExit("Phase 16 dispatcher helper source changed unexpectedly")
if text.count(old_call) != 1:
    raise SystemExit("Phase 16 dispatcher call source changed unexpectedly")
CLEANUP.write_text(text.replace(old_dispatch, new_dispatch, 1).replace(old_call, new_call, 1),
                   encoding="utf-8")
runpy.run_path(str(WRAPPER), run_name="__main__")
