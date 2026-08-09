#ifndef TEST_EXAMPLES_FIXTURE_H
#define TEST_EXAMPLES_FIXTURE_H

#include "cJSON.h"

/* Loads and parses the single checked-in contracts/v2/api/examples.json (the
 * same file webapp/tests/v2-api-contracts.test.ts validates every response
 * type guard against) exactly once per process, cached for the life of the
 * test binary, and returns a borrowed pointer to the top-level member named
 * `key` (e.g. "setupState", "diagnostics") -- TEST_CHECKs (aborting the test
 * run) on a missing/unparseable file or an absent key rather than returning
 * NULL, since any of those means the fixture itself is broken, not the code
 * under test. The returned cJSON* is owned by this module; callers must not
 * cJSON_Delete() it. Requires EXAMPLES_JSON_PATH to be defined by the build
 * (see tests/host/CMakeLists.txt), the same convention
 * TEST_SECRET_SENTINEL_SCANNER_PATH already uses for
 * scripts/check-secret-sentinel.py. */
const cJSON *test_examples_fixture_get(const char *key);

#endif
