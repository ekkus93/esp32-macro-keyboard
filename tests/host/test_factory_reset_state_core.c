#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "factory_reset_state_core.h"
#include "test_assert.h"

typedef struct {
    app_error_code_t read_result;
    app_error_code_t write_result;
    app_error_code_t erase_result;
    uint8_t stored_value;
    unsigned int read_calls;
    unsigned int write_calls;
    unsigned int erase_calls;
} fake_state_t;

static app_error_code_t fake_read(void *context, uint8_t *out_value) {
    fake_state_t *fake = context;
    ++fake->read_calls;
    if (fake->read_result == APP_ERROR_NONE) {
        *out_value = fake->stored_value;
    }
    return fake->read_result;
}

static app_error_code_t fake_write(void *context, uint8_t value) {
    fake_state_t *fake = context;
    ++fake->write_calls;
    if (fake->write_result == APP_ERROR_NONE) {
        fake->stored_value = value;
    }
    return fake->write_result;
}

static app_error_code_t fake_erase(void *context) {
    fake_state_t *fake = context;
    ++fake->erase_calls;
    return fake->erase_result;
}

static factory_reset_state_core_ops_t operations(fake_state_t *fake) {
    return (factory_reset_state_core_ops_t){
        .context = fake,
        .read_value = fake_read,
        .write_value = fake_write,
        .erase_value = fake_erase,
    };
}

static void test_missing_marker_is_none(void) {
    fake_state_t fake = {.read_result = APP_ERROR_NOT_FOUND};
    const factory_reset_state_core_ops_t ops = operations(&fake);
    factory_reset_state_t state = FACTORY_RESET_STATE_PENDING;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, factory_reset_state_core_read(&ops, &state));
    TEST_CHECK_EQ_INT(FACTORY_RESET_STATE_NONE, state);
    TEST_CHECK_EQ_U64(1U, fake.read_calls);
}

static void test_pending_marker_round_trips(void) {
    fake_state_t fake = {
        .read_result = APP_ERROR_NONE,
        .stored_value = (uint8_t)FACTORY_RESET_STATE_PENDING,
    };
    const factory_reset_state_core_ops_t ops = operations(&fake);
    factory_reset_state_t state = FACTORY_RESET_STATE_NONE;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, factory_reset_state_core_read(&ops, &state));
    TEST_CHECK_EQ_INT(FACTORY_RESET_STATE_PENDING, state);
}

static void test_unknown_marker_fails_closed(void) {
    fake_state_t fake = {.read_result = APP_ERROR_NONE, .stored_value = UINT8_C(7)};
    const factory_reset_state_core_ops_t ops = operations(&fake);
    factory_reset_state_t state = FACTORY_RESET_STATE_PENDING;
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, factory_reset_state_core_read(&ops, &state));
    TEST_CHECK_EQ_INT(FACTORY_RESET_STATE_NONE, state);
}

static void test_mark_pending_and_clear_propagate_storage_results(void) {
    fake_state_t fake = {0};
    factory_reset_state_core_ops_t ops = operations(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, factory_reset_state_core_mark_pending(&ops));
    TEST_CHECK_EQ_U64(1U, fake.write_calls);
    TEST_CHECK_EQ_U64((uint8_t)FACTORY_RESET_STATE_PENDING, fake.stored_value);

    fake.write_result = APP_ERROR_STORAGE_UNAVAILABLE;
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE,
                         factory_reset_state_core_mark_pending(&ops));

    fake.erase_result = APP_ERROR_NOT_FOUND;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, factory_reset_state_core_clear(&ops));
    fake.erase_result = APP_ERROR_STORAGE_UNAVAILABLE;
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, factory_reset_state_core_clear(&ops));
}

int main(void) {
    test_missing_marker_is_none();
    test_pending_marker_round_trips();
    test_unknown_marker_fails_closed();
    test_mark_pending_and_clear_propagate_storage_results();
    puts("factory reset state core tests passed");
    return EXIT_SUCCESS;
}
