#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "auth.h"
#include "test_assert.h"
#include "web_server.h"
#include "web_server_password_record.h"

#define STRESS_ITERATIONS 50000U
#define STRESS_READER_COUNT 4U

/* The production module owns this global through web_server_common.c. This
 * focused host target defines it directly so the exact synchronized accessor
 * implementation can be exercised without pulling in the HTTP adapter. */
web_server_config_t server_configuration;

typedef struct {
    auth_password_record_t first;
    auth_password_record_t second;
    atomic_bool start;
    atomic_bool failed;
} stress_context_t;

static auth_password_record_t make_record(uint8_t salt_byte, uint8_t hash_byte,
                                          uint32_t iterations) {
    auth_password_record_t record = {0};
    memset(record.salt, salt_byte, sizeof(record.salt));
    memset(record.hash, hash_byte, sizeof(record.hash));
    record.iterations = iterations;
    return record;
}

static bool record_equal(const auth_password_record_t *left,
                         const auth_password_record_t *right) {
    return left->iterations == right->iterations &&
           memcmp(left->salt, right->salt, sizeof(left->salt)) == 0 &&
           memcmp(left->hash, right->hash, sizeof(left->hash)) == 0;
}

static void await_start(const stress_context_t *context) {
    while (!atomic_load_explicit(&context->start, memory_order_acquire)) {
    }
}

static void *writer_thread(void *argument) {
    stress_context_t *context = argument;
    await_start(context);
    for (size_t index = 0U; index < (size_t)STRESS_ITERATIONS; ++index) {
        const auth_password_record_t *next =
            (index & 1U) == 0U ? &context->second : &context->first;
        web_server_password_record_replace(next);
    }
    return NULL;
}

static void *reader_thread(void *argument) {
    stress_context_t *context = argument;
    await_start(context);
    for (size_t index = 0U; index < (size_t)STRESS_ITERATIONS; ++index) {
        auth_password_record_t snapshot = {0};
        web_server_password_record_snapshot(&snapshot);
        if (!record_equal(&snapshot, &context->first) &&
            !record_equal(&snapshot, &context->second)) {
            atomic_store_explicit(&context->failed, true, memory_order_release);
            break;
        }
    }
    return NULL;
}

static void test_configuration_store_and_clear_use_same_lock_domain(void) {
    const auth_password_record_t expected = make_record(0x31U, 0xA7U, 5500U);
    web_server_config_t configuration = {
        .mode = WEB_SERVER_MODE_NORMAL,
        .login_enabled = true,
        .password_record = expected,
    };

    web_server_configuration_store(&configuration);
    auth_password_record_t snapshot = {0};
    web_server_password_record_snapshot(&snapshot);
    TEST_CHECK(record_equal(&snapshot, &expected));

    web_server_configuration_clear();
    web_server_password_record_snapshot(&snapshot);
    const auth_password_record_t zero = {0};
    TEST_CHECK(record_equal(&snapshot, &zero));
}

static void test_concurrent_snapshot_and_replace_never_observe_torn_record(void) {
    stress_context_t context = {
        .first = make_record(0x11U, 0x22U, 5500U),
        .second = make_record(0xE1U, 0xF2U, 9001U),
    };
    atomic_init(&context.start, false);
    atomic_init(&context.failed, false);

    web_server_config_t configuration = {
        .mode = WEB_SERVER_MODE_NORMAL,
        .login_enabled = true,
        .password_record = context.first,
    };
    web_server_configuration_store(&configuration);

    pthread_t writer;
    pthread_t readers[STRESS_READER_COUNT];
    TEST_CHECK_EQ_INT(0, pthread_create(&writer, NULL, writer_thread, &context));
    for (size_t index = 0U; index < (size_t)STRESS_READER_COUNT; ++index) {
        TEST_CHECK_EQ_INT(0, pthread_create(&readers[index], NULL, reader_thread, &context));
    }

    atomic_store_explicit(&context.start, true, memory_order_release);
    TEST_CHECK_EQ_INT(0, pthread_join(writer, NULL));
    for (size_t index = 0U; index < (size_t)STRESS_READER_COUNT; ++index) {
        TEST_CHECK_EQ_INT(0, pthread_join(readers[index], NULL));
    }

    TEST_CHECK(!atomic_load_explicit(&context.failed, memory_order_acquire));
}

int main(void) {
    test_configuration_store_and_clear_use_same_lock_domain();
    test_concurrent_snapshot_and_replace_never_observe_torn_record();
    puts("web password-record synchronization tests passed");
    return EXIT_SUCCESS;
}
