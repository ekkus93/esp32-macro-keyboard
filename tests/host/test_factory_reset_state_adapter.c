#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "factory_reset_state.h"
#include "nvs.h"
#include "test_assert.h"

typedef struct {
    esp_err_t open_result;
    esp_err_t get_result;
    esp_err_t set_result;
    esp_err_t erase_result;
    esp_err_t commit_result;
    uint8_t stored_value;
    unsigned int open_calls;
    unsigned int get_calls;
    unsigned int set_calls;
    unsigned int erase_calls;
    unsigned int commit_calls;
    unsigned int close_calls;
    nvs_open_mode_t last_mode;
} fake_nvs_t;

static fake_nvs_t fake;
static const nvs_handle_t TEST_HANDLE = UINT32_C(0x1234);

static void reset_fake(void) {
    memset(&fake, 0, sizeof(fake));
    fake.open_result = ESP_OK;
    fake.get_result = ESP_OK;
    fake.set_result = ESP_OK;
    fake.erase_result = ESP_OK;
    fake.commit_result = ESP_OK;
    fake.stored_value = (uint8_t)FACTORY_RESET_STATE_PENDING;
}

esp_err_t nvs_open(const char *name, nvs_open_mode_t open_mode, nvs_handle_t *out_handle) {
    TEST_CHECK(strcmp("reset_journal", name) == 0);
    TEST_CHECK(out_handle != NULL);
    ++fake.open_calls;
    fake.last_mode = open_mode;
    if (fake.open_result == ESP_OK) {
        *out_handle = TEST_HANDLE;
    }
    return fake.open_result;
}

static void check_handle_and_key(nvs_handle_t handle, const char *key) {
    TEST_CHECK_EQ_U64(TEST_HANDLE, handle);
    TEST_CHECK(strcmp("factory_reset", key) == 0);
}

esp_err_t nvs_get_u8(nvs_handle_t handle, const char *key, uint8_t *out_value) {
    check_handle_and_key(handle, key);
    TEST_CHECK(out_value != NULL);
    ++fake.get_calls;
    if (fake.get_result == ESP_OK) {
        *out_value = fake.stored_value;
    }
    return fake.get_result;
}

esp_err_t nvs_set_u8(nvs_handle_t handle, const char *key, uint8_t value) {
    check_handle_and_key(handle, key);
    ++fake.set_calls;
    TEST_CHECK_EQ_U64(FACTORY_RESET_STATE_PENDING, value);
    return fake.set_result;
}

esp_err_t nvs_erase_key(nvs_handle_t handle, const char *key) {
    check_handle_and_key(handle, key);
    ++fake.erase_calls;
    return fake.erase_result;
}

esp_err_t nvs_commit(nvs_handle_t handle) {
    TEST_CHECK_EQ_U64(TEST_HANDLE, handle);
    ++fake.commit_calls;
    return fake.commit_result;
}

void nvs_close(nvs_handle_t handle) {
    TEST_CHECK_EQ_U64(TEST_HANDLE, handle);
    ++fake.close_calls;
}

static void test_missing_namespace_reads_none(void) {
    reset_fake();
    fake.open_result = ESP_ERR_NVS_NOT_FOUND;
    factory_reset_state_t state = FACTORY_RESET_STATE_PENDING;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, factory_reset_state_read(&state));
    TEST_CHECK_EQ_INT(FACTORY_RESET_STATE_NONE, state);
    TEST_CHECK_EQ_U64(1U, fake.open_calls);
    TEST_CHECK_EQ_U64(0U, fake.get_calls);
    TEST_CHECK_EQ_U64(0U, fake.close_calls);
    TEST_CHECK_EQ_INT(NVS_READONLY, fake.last_mode);
}

static void test_pending_read_and_corrupt_value_fail_closed(void) {
    reset_fake();
    factory_reset_state_t state = FACTORY_RESET_STATE_NONE;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, factory_reset_state_read(&state));
    TEST_CHECK_EQ_INT(FACTORY_RESET_STATE_PENDING, state);
    TEST_CHECK_EQ_U64(1U, fake.get_calls);
    TEST_CHECK_EQ_U64(1U, fake.close_calls);
    TEST_CHECK_EQ_INT(NVS_READONLY, fake.last_mode);

    reset_fake();
    fake.stored_value = UINT8_C(7);
    state = FACTORY_RESET_STATE_NONE;
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, factory_reset_state_read(&state));
    TEST_CHECK_EQ_INT(FACTORY_RESET_STATE_NONE, state);
    TEST_CHECK_EQ_U64(1U, fake.close_calls);
}

static void test_mark_pending_requires_commit(void) {
    reset_fake();
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, factory_reset_state_mark_pending());
    TEST_CHECK_EQ_INT(NVS_READWRITE, fake.last_mode);
    TEST_CHECK_EQ_U64(1U, fake.set_calls);
    TEST_CHECK_EQ_U64(1U, fake.commit_calls);
    TEST_CHECK_EQ_U64(1U, fake.close_calls);

    reset_fake();
    fake.set_result = ESP_FAIL;
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, factory_reset_state_mark_pending());
    TEST_CHECK_EQ_U64(1U, fake.set_calls);
    TEST_CHECK_EQ_U64(0U, fake.commit_calls);
    TEST_CHECK_EQ_U64(1U, fake.close_calls);

    reset_fake();
    fake.commit_result = ESP_FAIL;
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, factory_reset_state_mark_pending());
    TEST_CHECK_EQ_U64(1U, fake.set_calls);
    TEST_CHECK_EQ_U64(1U, fake.commit_calls);
    TEST_CHECK_EQ_U64(1U, fake.close_calls);
}

static void test_clear_requires_commit_unless_already_absent(void) {
    reset_fake();
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, factory_reset_state_clear());
    TEST_CHECK_EQ_INT(NVS_READWRITE, fake.last_mode);
    TEST_CHECK_EQ_U64(1U, fake.erase_calls);
    TEST_CHECK_EQ_U64(1U, fake.commit_calls);
    TEST_CHECK_EQ_U64(1U, fake.close_calls);

    reset_fake();
    fake.erase_result = ESP_ERR_NVS_NOT_FOUND;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, factory_reset_state_clear());
    TEST_CHECK_EQ_U64(1U, fake.erase_calls);
    TEST_CHECK_EQ_U64(0U, fake.commit_calls);
    TEST_CHECK_EQ_U64(1U, fake.close_calls);

    reset_fake();
    fake.commit_result = ESP_FAIL;
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, factory_reset_state_clear());
    TEST_CHECK_EQ_U64(1U, fake.erase_calls);
    TEST_CHECK_EQ_U64(1U, fake.commit_calls);
    TEST_CHECK_EQ_U64(1U, fake.close_calls);
}

int main(void) {
    test_missing_namespace_reads_none();
    test_pending_read_and_corrupt_value_fail_closed();
    test_mark_pending_requires_commit();
    test_clear_requires_commit_unless_already_absent();
    puts("factory reset state NVS adapter tests passed");
    return EXIT_SUCCESS;
}
