#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "provisioning.h"
#include "provisioning_core.h"
#include "test_assert.h"

typedef struct {
    uint8_t committed[PROVISIONING_RECORD_BYTES];
    uint8_t pending[PROVISIONING_RECORD_BYTES];
    size_t size;
    bool present;
    bool pending_valid;
    size_t commit_count;
} settings_store_t;

static app_error_code_t read_blob(void *context, uint8_t *output, size_t capacity,
                                  size_t *out_size) {
    settings_store_t *store = context;
    if (!store->present) {
        return APP_ERROR_NOT_FOUND;
    }
    if (capacity < store->size) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    memcpy(output, store->committed, store->size);
    *out_size = store->size;
    return APP_ERROR_NONE;
}

static app_error_code_t write_blob(void *context, const uint8_t *data, size_t size) {
    settings_store_t *store = context;
    if (size != sizeof(store->pending)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memcpy(store->pending, data, size);
    store->size = size;
    store->pending_valid = true;
    return APP_ERROR_NONE;
}

static app_error_code_t erase_blob(void *context) {
    settings_store_t *store = context;
    memset(store->pending, 0, sizeof(store->pending));
    store->size = 0U;
    store->pending_valid = true;
    return APP_ERROR_NONE;
}

static app_error_code_t commit_blob(void *context) {
    settings_store_t *store = context;
    ++store->commit_count;
    if (!store->pending_valid) {
        return APP_ERROR_INTERNAL;
    }
    if (store->size == 0U) {
        memset(store->committed, 0, sizeof(store->committed));
        store->present = false;
    } else {
        memcpy(store->committed, store->pending, store->size);
        store->present = true;
    }
    store->pending_valid = false;
    return APP_ERROR_NONE;
}

static void secure_zero(void *context, void *memory, size_t size) {
    (void)context;
    volatile uint8_t *bytes = memory;
    for (size_t index = 0U; index < size; ++index) {
        bytes[index] = 0U;
    }
}

static app_uuid_t make_uuid(void) {
    app_uuid_t uuid = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         app_uuid_parse("12345678-0000-4000-8000-000000000001", &uuid));
    return uuid;
}

static provisioning_core_t initialized_core(settings_store_t *store) {
    const provisioning_ops_t operations = {
        .context = store,
        .read_blob = read_blob,
        .write_blob = write_blob,
        .erase_blob = erase_blob,
        .commit = commit_blob,
        .secure_zero = secure_zero,
    };
    provisioning_core_t core = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_init(&core, &operations));
    provisioning_config_t configuration = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_load(&core, &configuration));
    return core;
}

static void test_redacted_settings_round_trip_and_clear(void) {
    settings_store_t store = {0};
    provisioning_core_t core = initialized_core(&store);
    provisioning_settings_t settings = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_settings_read(&core, &settings));
    TEST_CHECK_EQ_U64(APP_SCHEMA_VERSION, settings.schema_version);
    TEST_CHECK_EQ_U64(0U, settings.revision);
    TEST_CHECK(settings.always_select_set);
    /* Off by default: the device must be usable with no button on it. */
    TEST_CHECK(!settings.require_physical_confirmation);
    TEST_CHECK(!settings.has_active_set);

    settings.always_select_set = false;
    settings.has_active_set = true;
    settings.active_set_id = make_uuid();
    provisioning_settings_t committed = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         provisioning_core_settings_update(&core, &settings, 0U, &committed));
    TEST_CHECK_EQ_U64(1U, committed.revision);
    TEST_CHECK(!committed.always_select_set);
    TEST_CHECK(committed.has_active_set);
    TEST_CHECK_EQ_UUID(&settings.active_set_id, &committed.active_set_id);
    TEST_CHECK_EQ_U64(1U, store.commit_count);

    bool cleared = true;
    const app_uuid_t other = {
        .value = "87654321-0000-4000-8000-000000000002",
    };
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         provisioning_core_clear_active_set_if_matches(&core, &other, &cleared));
    TEST_CHECK(!cleared);
    TEST_CHECK_EQ_U64(1U, store.commit_count);

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_clear_active_set_if_matches(
                                             &core, &settings.active_set_id, &cleared));
    TEST_CHECK(cleared);
    TEST_CHECK_EQ_U64(2U, store.commit_count);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_settings_read(&core, &committed));
    TEST_CHECK_EQ_U64(2U, committed.revision);
    TEST_CHECK(!committed.has_active_set);

    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT,
                         provisioning_core_settings_update(&core, &settings, 0U, &committed));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_deinit(&core));
}

static void test_settings_validation(void) {
    settings_store_t store = {0};
    provisioning_core_t core = initialized_core(&store);
    provisioning_settings_t settings = {
        .schema_version = APP_SCHEMA_VERSION,
        .revision = 0U,
        .require_physical_confirmation = true,
        .always_select_set = true,
        .has_active_set = true,
    };
    provisioning_settings_t output = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         provisioning_core_settings_update(&core, &settings, 0U, &output));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         provisioning_core_clear_active_set_if_matches(&core, NULL, NULL));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_deinit(&core));
}

int main(void) {
    test_redacted_settings_round_trip_and_clear();
    test_settings_validation();
    puts("provisioning settings tests passed");
    return EXIT_SUCCESS;
}
