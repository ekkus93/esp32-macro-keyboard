#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "auth.h"
#include "test_assert.h"
#include "web_server_adapter.h"

#define TOKEN "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"

typedef struct {
    const char *body;
    size_t cursor;
    size_t chunk_size;
    int planned_results[8U];
    size_t planned_count;
    size_t call_count;
} receive_fake_t;

static int receive_body(void *context, char *output, size_t capacity)
{
    receive_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    TEST_CHECK(output != NULL);
    TEST_CHECK(capacity > 0U);
    int planned = 0;
    if (fake->call_count < fake->planned_count) {
        planned = fake->planned_results[fake->call_count];
    }
    ++fake->call_count;
    if (planned != 0) {
        return planned;
    }
    const size_t body_length = strlen(fake->body);
    if (fake->cursor >= body_length) {
        return 0;
    }
    size_t count = body_length - fake->cursor;
    if (fake->chunk_size > 0U && count > fake->chunk_size) {
        count = fake->chunk_size;
    }
    if (count > capacity) {
        count = capacity;
    }
    memcpy(output, fake->body + fake->cursor, count);
    fake->cursor += count;
    return (int)count;
}

static void test_bounded_body_reads(void)
{
    char output[16U];
    receive_fake_t fake = {.body = "abcdef", .chunk_size = 2U};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_adapter_read_bounded_body(6U,
                                                       output,
                                                       sizeof(output),
                                                       6U,
                                                       receive_body,
                                                       &fake));
    TEST_CHECK_EQ_STRING("abcdef", output);
    TEST_CHECK_EQ_U64(3U, fake.call_count);

    fake = (receive_fake_t){.body = ""};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_adapter_read_bounded_body(0U,
                                                       output,
                                                       sizeof(output),
                                                       0U,
                                                       receive_body,
                                                       &fake));
    TEST_CHECK_EQ_STRING("", output);
    TEST_CHECK_EQ_U64(0U, fake.call_count);

    memcpy(output, "dirty", sizeof("dirty"));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_adapter_read_bounded_body(sizeof(output),
                                                       output,
                                                       sizeof(output),
                                                       sizeof(output),
                                                       receive_body,
                                                       &fake));
    TEST_CHECK_EQ_STRING("", output);
}

static void test_body_timeout_and_io_failures(void)
{
    char output[8U];
    receive_fake_t fake = {
        .body = "ab",
        .planned_results = {WEB_ADAPTER_RECEIVE_TIMEOUT,
                            WEB_ADAPTER_RECEIVE_TIMEOUT,
                            0},
        .planned_count = 3U,
    };
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_adapter_read_bounded_body(2U,
                                                       output,
                                                       sizeof(output),
                                                       2U,
                                                       receive_body,
                                                       &fake));
    TEST_CHECK_EQ_STRING("ab", output);

    fake = (receive_fake_t){
        .body = "ab",
        .planned_results = {-1},
        .planned_count = 1U,
    };
    TEST_CHECK_APP_ERROR(APP_ERROR_IO,
                         web_adapter_read_bounded_body(2U,
                                                       output,
                                                       sizeof(output),
                                                       2U,
                                                       receive_body,
                                                       &fake));
    TEST_CHECK_EQ_STRING("", output);

    fake = (receive_fake_t){
        .body = "ab",
        .planned_results = {WEB_ADAPTER_RECEIVE_TIMEOUT,
                            WEB_ADAPTER_RECEIVE_TIMEOUT,
                            WEB_ADAPTER_RECEIVE_TIMEOUT,
                            WEB_ADAPTER_RECEIVE_TIMEOUT,
                            WEB_ADAPTER_RECEIVE_TIMEOUT},
        .planned_count = 5U,
    };
    TEST_CHECK_APP_ERROR(APP_ERROR_TIMEOUT,
                         web_adapter_read_bounded_body(2U,
                                                       output,
                                                       sizeof(output),
                                                       2U,
                                                       receive_body,
                                                       &fake));
    TEST_CHECK_EQ_STRING("", output);
}
