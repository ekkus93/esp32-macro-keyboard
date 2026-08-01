#ifndef TEST_SECRET_SENTINEL_H
#define TEST_SECRET_SENTINEL_H

#include <stddef.h>

/* Writes `secret` and each of `outputs` to real files, then runs the actual
 * scripts/check-secret-sentinel.py against them (all 7 representations it
 * checks: raw, JSON-escaped, URL-encoded, base64, base64url, hex-lower,
 * hex-upper) and TEST_CHECKs that it finds nothing. This is the same script
 * that gates FIX1 TODO 18.5 in CI, so a caller that constructs `outputs` from
 * a real production function's return value is exercising the real,
 * production-path proof that the secret does not occur in it - not a
 * substring check reimplemented in C. */
void test_assert_no_secret_sentinel(const char *secret, const char *const *outputs,
                                    size_t output_count);

#endif
