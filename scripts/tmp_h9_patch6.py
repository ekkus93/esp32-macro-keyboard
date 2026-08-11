from pathlib import Path

path = Path("tests/host/test_app_core.c")
text = path.read_text()
old = '''    } else if (event->type == APP_CORE_LOG_SETUP_CODE) {\n        ++fixture->setup_code_logs;\n        TEST_CHECK_EQ_STRING(fixture->setup_code, event->setup_code);\n    }\n'''
new = '''    } else if (event->type == APP_CORE_LOG_SETUP_CODE) {\n        ++fixture->setup_code_logs;\n        /* H9: startup may emit a generic setup-readiness event, but the\n         * manufacturing-label setup secret must never be carried into the\n         * logging boundary. */\n        TEST_CHECK(event->setup_code == NULL);\n    }\n'''
if text.count(old) != 1:
    raise SystemExit(f"test_app_core setup log anchor count: {text.count(old)}")
path.write_text(text.replace(old, new, 1))
