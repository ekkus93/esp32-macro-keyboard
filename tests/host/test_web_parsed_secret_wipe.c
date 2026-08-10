#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "cJSON.h"
#include "test_assert.h"
#include "web_device_actions.h"
#include "web_settings.h"

#define TEST_BODY_CAPACITY 512U
#define TEST_SECRET "R1-011-parsed-secret-sentinel-7f39d14b"

typedef struct {
    size_t size;
} guarded_allocation_t;

static const char *guarded_secret;
static size_t guarded_outstanding_allocations;
static bool guarded_secret_seen_at_free;

static bool memory_contains_secret(const void *memory, size_t size, const char *secret) {
    const size_t secret_length = strlen(secret);
    if (secret_length == 0U || size < secret_length) {
        return false;
    }
    const unsigned char *bytes = memory;
    for (size_t offset = 0U; offset <= size - secret_length; ++offset) {
        if (memcmp(bytes + offset, secret, secret_length) == 0) {
            return true;
        }
    }
    return false;
}

static void *guarded_malloc(size_t size) {
    if (size > SIZE_MAX - sizeof(guarded_allocation_t)) {
        return NULL;
    }
    guarded_allocation_t *allocation = malloc(sizeof(*allocation) + size);
    if (allocation == NULL) {
        return NULL;
    }
    allocation->size = size;
    ++guarded_outstanding_allocations;
    return allocation + 1;
}

static void guarded_free(void *memory) {
    if (memory == NULL) {
        return;
    }
    guarded_allocation_t *allocation = ((guarded_allocation_t *)memory) - 1;
    if (guarded_secret != NULL &&
        memory_contains_secret(memory, allocation->size, guarded_secret)) {
        guarded_secret_seen_at_free = true;
    }
    TEST_CHECK(guarded_outstanding_allocations > 0U);
    --guarded_outstanding_allocations;
    free(allocation);
}

static void begin_free_guard(const char *secret) {
    TEST_CHECK(secret != NULL && secret[0] != '\0');
    TEST_CHECK_EQ_U64(0U, guarded_outstanding_allocations);
    guarded_secret = secret;
    guarded_secret_seen_at_free = false;
    const cJSON_Hooks hooks = {
        .malloc_fn = guarded_malloc,
        .free_fn = guarded_free,
    };
    cJSON_InitHooks((cJSON_Hooks *)&hooks);
}

static void end_free_guard(void) {
    TEST_CHECK_EQ_U64(0U, guarded_outstanding_allocations);
    cJSON_InitHooks(NULL);
    TEST_CHECK(!guarded_secret_seen_at_free);
    guarded_secret = NULL;
}

static void build_body(char *buffer, size_t capacity, const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);
    const int written = vsnprintf(buffer, capacity, format, arguments);
    va_end(arguments);
    TEST_CHECK(written > 0 && (size_t)written < capacity);
}

static app_error_code_t unavailable_settings_read(void *context,
                                                  app_v2_device_settings_t *out_settings) {
    (void)context;
    (void)out_settings;
    return APP_ERROR_STORAGE_UNAVAILABLE;
}

static app_error_code_t unused_settings_replace(void *context,
                                                const app_v2_device_settings_t *settings,
                                                bool *out_changed) {
    (void)context;
    (void)settings;
    (void)out_changed;
    return APP_ERROR_INTERNAL;
}

static app_error_code_t unused_password_verify(void *context, const char *password,
                                               size_t password_length,
                                               const app_v2_device_settings_t *settings,
                                               bool *out_matches) {
    (void)context;
    (void)password;
    (void)password_length;
    (void)settings;
    (void)out_matches;
    return APP_ERROR_INTERNAL;
}

static app_error_code_t unused_password_create(void *context, const char *password,
                                               size_t password_length,
                                               app_v2_setup_password_material_t *out_material) {
    (void)context;
    (void)password;
    (void)password_length;
    (void)out_material;
    return APP_ERROR_INTERNAL;
}

static app_error_code_t unused_invalidate_sessions(void *context) {
    (void)context;
    return APP_ERROR_INTERNAL;
}

static app_error_code_t unused_factory_reset(void *context) {
    (void)context;
    return APP_ERROR_INTERNAL;
}

static web_settings_ops_t settings_operations(void) {
    return (web_settings_ops_t){
        .context = NULL,
        .settings_read = unavailable_settings_read,
        .settings_replace = unused_settings_replace,
        .password_verify = unused_password_verify,
        .password_create = unused_password_create,
        .invalidate_all_sessions = unused_invalidate_sessions,
    };
}

static web_device_actions_ops_t device_operations(void) {
    return (web_device_actions_ops_t){
        .context = NULL,
        .settings_read = unavailable_settings_read,
        .password_verify = unused_password_verify,
        .factory_reset = unused_factory_reset,
    };
}

static void test_change_password_schema_rejection_wipes_parsed_secret(void) {
    const web_settings_ops_t ops = settings_operations();
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"currentPassword\":\"%s\",\"newPassword\":\"new-example-password\","
               "\"extra\":true}",
               TEST_SECRET);

    begin_free_guard(TEST_SECRET);
    const web_change_password_outcome_t outcome =
        web_change_password_handle(body, sizeof(body), &ops);
    end_free_guard();

    TEST_CHECK_EQ_INT(WEB_CHANGE_PASSWORD_INVALID_BODY, outcome.result);
}

static void test_change_password_backend_failure_wipes_parsed_secret(void) {
    const web_settings_ops_t ops = settings_operations();
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"currentPassword\":\"%s\",\"newPassword\":\"new-example-password\"}",
               TEST_SECRET);

    begin_free_guard(TEST_SECRET);
    const web_change_password_outcome_t outcome =
        web_change_password_handle(body, sizeof(body), &ops);
    end_free_guard();

    TEST_CHECK_EQ_INT(WEB_CHANGE_PASSWORD_BACKEND_UNAVAILABLE, outcome.result);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, outcome.detail);
}

static void test_change_password_non_object_wipes_parsed_secret(void) {
    const web_settings_ops_t ops = settings_operations();
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "[\"%s\"]", TEST_SECRET);

    begin_free_guard(TEST_SECRET);
    const web_change_password_outcome_t outcome =
        web_change_password_handle(body, sizeof(body), &ops);
    end_free_guard();

    TEST_CHECK_EQ_INT(WEB_CHANGE_PASSWORD_INVALID_BODY, outcome.result);
}

static void test_factory_reset_schema_rejection_wipes_parsed_secret(void) {
    const web_device_actions_ops_t ops = device_operations();
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"adminPassword\":\"%s\",\"confirmation\":\"FACTORY RESET\",\"extra\":true}",
               TEST_SECRET);

    begin_free_guard(TEST_SECRET);
    const web_device_factory_reset_outcome_t outcome =
        web_device_factory_reset_handle(body, sizeof(body), &ops);
    end_free_guard();

    TEST_CHECK_EQ_INT(WEB_DEVICE_FACTORY_RESET_INVALID_BODY, outcome.result);
}

static void test_factory_reset_confirmation_rejection_wipes_parsed_secret(void) {
    const web_device_actions_ops_t ops = device_operations();
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"adminPassword\":\"%s\",\"confirmation\":\"factory reset\"}", TEST_SECRET);

    begin_free_guard(TEST_SECRET);
    const web_device_factory_reset_outcome_t outcome =
        web_device_factory_reset_handle(body, sizeof(body), &ops);
    end_free_guard();

    TEST_CHECK_EQ_INT(WEB_DEVICE_FACTORY_RESET_INVALID_CONFIRMATION, outcome.result);
}

static void test_factory_reset_non_object_wipes_parsed_secret(void) {
    const web_device_actions_ops_t ops = device_operations();
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "[\"%s\"]", TEST_SECRET);

    begin_free_guard(TEST_SECRET);
    const web_device_factory_reset_outcome_t outcome =
        web_device_factory_reset_handle(body, sizeof(body), &ops);
    end_free_guard();

    TEST_CHECK_EQ_INT(WEB_DEVICE_FACTORY_RESET_INVALID_BODY, outcome.result);
}

int main(void) {
    test_change_password_schema_rejection_wipes_parsed_secret();
    test_change_password_backend_failure_wipes_parsed_secret();
    test_change_password_non_object_wipes_parsed_secret();
    test_factory_reset_schema_rejection_wipes_parsed_secret();
    test_factory_reset_confirmation_rejection_wipes_parsed_secret();
    test_factory_reset_non_object_wipes_parsed_secret();
    puts("web parsed secret wipe tests passed");
    return EXIT_SUCCESS;
}
