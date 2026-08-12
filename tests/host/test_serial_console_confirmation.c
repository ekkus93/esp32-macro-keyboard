#include <stdbool.h>

#include "app_error.h"
#include "serial_console_confirmation.h"
#include "test_assert.h"

typedef struct {
    app_error_code_t send_result;
    app_error_code_t device_result;
    unsigned send_calls;
    unsigned device_calls;
} fake_t;

static app_error_code_t confirm_send(void *context) {
    fake_t *fake = context;
    fake->send_calls += 1U;
    return fake->send_result;
}

static app_error_code_t confirm_device(void *context) {
    fake_t *fake = context;
    fake->device_calls += 1U;
    return fake->device_result;
}

static serial_console_confirmation_ops_t ops(fake_t *fake) {
    return (serial_console_confirmation_ops_t){
        .context = fake,
        .confirm_send = confirm_send,
        .confirm_device_action = confirm_device,
    };
}

static void test_pending_send_consumes_confirmation_without_admin_fallthrough(void) {
    fake_t fake = {.send_result = APP_ERROR_NONE, .device_result = APP_ERROR_NONE};
    const serial_console_confirmation_ops_t operations = ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, serial_console_route_confirmation(&operations));
    TEST_CHECK_EQ_U64(1U, fake.send_calls);
    TEST_CHECK_EQ_U64(0U, fake.device_calls);
}

static void test_duplicate_send_confirmation_does_not_confirm_admin_action(void) {
    fake_t fake = {.send_result = APP_ERROR_CONFLICT, .device_result = APP_ERROR_NONE};
    const serial_console_confirmation_ops_t operations = ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, serial_console_route_confirmation(&operations));
    TEST_CHECK_EQ_U64(1U, fake.send_calls);
    TEST_CHECK_EQ_U64(0U, fake.device_calls);
}

static void test_no_pending_send_falls_back_to_admin_confirmation(void) {
    fake_t fake = {.send_result = APP_ERROR_NOT_FOUND, .device_result = APP_ERROR_NONE};
    const serial_console_confirmation_ops_t operations = ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, serial_console_route_confirmation(&operations));
    TEST_CHECK_EQ_U64(1U, fake.send_calls);
    TEST_CHECK_EQ_U64(1U, fake.device_calls);
}

static void test_send_confirmation_failure_fails_closed(void) {
    fake_t fake = {.send_result = APP_ERROR_INTERNAL, .device_result = APP_ERROR_NONE};
    const serial_console_confirmation_ops_t operations = ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, serial_console_route_confirmation(&operations));
    TEST_CHECK_EQ_U64(1U, fake.send_calls);
    TEST_CHECK_EQ_U64(0U, fake.device_calls);
}

int main(void) {
    test_pending_send_consumes_confirmation_without_admin_fallthrough();
    test_duplicate_send_confirmation_does_not_confirm_admin_action();
    test_no_pending_send_falls_back_to_admin_confirmation();
    test_send_confirmation_failure_fails_closed();
    return 0;
}
