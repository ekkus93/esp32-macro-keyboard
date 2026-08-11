from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    target = Path(path)
    text = target.read_text()
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{path}: expected exactly one anchor, found {count}: {old[:100]!r}")
    target.write_text(text.replace(old, new, 1))


replace_once(
    "tests/host/support/test_assert.h",
    '            test_fail_u64(__FILE__, __LINE__, #actual_value " == " #expected_value,                \\\n                          test_expected_, test_actual_);                                           \\\n',
    '            test_fail_u64(__FILE__, __LINE__, "integer equality", test_expected_, test_actual_);  \\\n',
)
replace_once(
    "tests/host/support/test_assert.c",
    '''void test_fail_u64(const char *file, int line, const char *expression, uint64_t expected,\n                   uint64_t actual) {\n    (void)fprintf(stderr, "test failure at %s:%d: %s; expected=%" PRIu64 ", actual=%" PRIu64 "\\n",\n                  safe_string(file), line, safe_string(expression), expected, actual);\n    exit(EXIT_FAILURE);\n}\n''',
    '''void test_fail_u64(const char *file, int line, const char *expression, uint64_t expected,\n                   uint64_t actual) {\n    (void)expected;\n    (void)actual;\n    (void)fprintf(stderr, "test failure at %s:%d: %s; integer values differ\\n", safe_string(file),\n                  line, safe_string(expression));\n    exit(EXIT_FAILURE);\n}\n''',
)
replace_once(
    "tests/host/support/test_assert.c",
    '''void test_fail_buffer(const char *file, int line, const char *expression, const void *expected,\n                      const void *actual, size_t length) {\n    (void)fprintf(stderr,\n                  "test failure at %s:%d: %s; buffers differ across %zu byte(s), "\n                  "expected=%p, actual=%p\\n",\n                  safe_string(file), line, safe_string(expression), length, expected, actual);\n    exit(EXIT_FAILURE);\n}\n''',
    '''void test_fail_buffer(const char *file, int line, const char *expression, const void *expected,\n                      const void *actual, size_t length) {\n    (void)expected;\n    (void)actual;\n    (void)fprintf(stderr, "test failure at %s:%d: %s; buffers differ across %zu byte(s)\\n",\n                  safe_string(file), line, safe_string(expression), length);\n    exit(EXIT_FAILURE);\n}\n''',
)
replace_once(
    "scripts/check-h9-architecture.py",
    'if "expected=\\\"%s\\\"" in test_assert or "actual=\\\"%s\\\"" in test_assert:\n    fail("host test assertion failures can print compared string values")\n',
    '''for forbidden in (\n    'expected=\\"%s\\"',\n    'actual=\\"%s\\"',\n    'expected=%" PRIu64',\n    'actual=%" PRIu64',\n    'expected=%p',\n    'actual=%p',\n):\n    if forbidden in test_assert:\n        fail(f"host test assertion failures can print compared values: {forbidden}")\n''',
)
