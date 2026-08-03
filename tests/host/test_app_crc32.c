#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_crc32.h"
#include "test_assert.h"

/* Known-answer vectors. A checksum implementation that agrees only with itself
 * proves nothing, so these were taken from Python's zlib.crc32 -- an
 * independent implementation of CRC-32/ISO-HDLC -- and not from running this
 * code and recording what it said. */
static void test_known_answers(void) {
    TEST_CHECK_EQ_U64(UINT32_C(0x00000000), app_crc32("", 0U));
    TEST_CHECK_EQ_U64(UINT32_C(0xCBF43926), app_crc32("123456789", 9U));
    TEST_CHECK_EQ_U64(UINT32_C(0x414FA339),
                      app_crc32("The quick brown fox jumps over the lazy dog", 43U));
    TEST_CHECK_EQ_U64(UINT32_C(0x6522DF69), app_crc32("\x00\x00\x00\x00\x00\x00\x00\x00", 8U));
}

/* The point of carrying a checksum is that a changed package stops matching. */
static void test_single_bit_change_is_detected(void) {
    const char before[] = "{\"id\":\"a\",\"revision\":1}";
    const char after[] = "{\"id\":\"a\",\"revision\":2}";
    TEST_CHECK(app_crc32(before, sizeof(before) - 1U) != app_crc32(after, sizeof(after) - 1U));
}

static void test_null_is_not_a_crash(void) {
    TEST_CHECK_EQ_U64(0U, app_crc32(NULL, 16U));
}

/* The incremental form exists so a checksum spanning several fields needs no
 * buffer. It has to produce exactly what one call over the joined bytes would,
 * or the device and anything that recomputes it would disagree. */
static void test_incremental_matches_one_shot(void) {
    const char whole[] = "123456789";
    uint32_t crc = APP_CRC32_INITIAL;
    crc = app_crc32_update(crc, "1234", 4U);
    crc = app_crc32_update(crc, "5678", 4U);
    crc = app_crc32_update(crc, "9", 1U);
    TEST_CHECK_EQ_U64(app_crc32(whole, sizeof(whole) - 1U), app_crc32_finish(crc));

    /* A zero-length field, and a NULL one, must not disturb the running value. */
    uint32_t stepped = APP_CRC32_INITIAL;
    stepped = app_crc32_update(stepped, "12345", 5U);
    stepped = app_crc32_update(stepped, "", 0U);
    stepped = app_crc32_update(stepped, NULL, 8U);
    stepped = app_crc32_update(stepped, "6789", 4U);
    TEST_CHECK_EQ_U64(app_crc32(whole, sizeof(whole) - 1U), app_crc32_finish(stepped));
}

int main(void) {
    test_known_answers();
    test_incremental_matches_one_shot();
    test_single_bit_change_is_detected();
    test_null_is_not_a_crash();
    puts("app_crc32 tests passed");
    return EXIT_SUCCESS;
}
