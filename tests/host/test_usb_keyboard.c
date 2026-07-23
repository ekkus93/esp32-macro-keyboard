#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_assert.h"
#include "usb_keyboard_state.h"

typedef struct {
    usb_keyboard_state_t state;
    app_error_code_t install_result;
    bool install_observed_enumerating;
    bool mounted;
    bool suspended;
    bool report_result;
    uint32_t now_ms;
    size_t ready_after_calls;
    size_t ready_calls;
    size_t delay_calls;
    uint32_t total_delay_ms;
    size_t disconnect_on_delay_call;
    size_t report_calls;
    uint8_t report_id;
    uint8_t modifiers;
    uint8_t keycodes[6];
} usb_fixture_t;

static void reset_fixture(usb_fixture_t *fixture)
{
    TEST_CHECK(fixture != NULL);
    memset(fixture, 0, sizeof(*fixture));
    fixture->state = USB_KEYBOARD_UNINITIALIZED;
    fixture->report_result = true;
    fixture->ready_after_calls = 1U;
}

static usb_keyboard_state_t fake_state_get(void *context)
{
    const usb_fixture_t *fixture = context;
    TEST_CHECK(fixture != NULL);
    return fixture->state;
}

static void fake_state_set(void *context, usb_keyboard_state_t state)
{
    usb_fixture_t *fixture = context;
    TEST_CHECK(fixture != NULL);
    fixture->state = state;
}

static app_error_code_t fake_driver_install(void *context)
{
    usb_fixture_t *fixture = context;
    TEST_CHECK(fixture != NULL);
    fixture->install_observed_enumerating =
        fixture->state == USB_KEYBOARD_ENUMERATING;
    return fixture->install_result;
}

static uint32_t fake_now_ms(void *context)
{
    const usb_fixture_t *fixture = context;
    TEST_CHECK(fixture != NULL);
    return fixture->now_ms;
}

static void fake_delay_ms(void *context, uint32_t milliseconds)
{
    usb_fixture_t *fixture = context;
    TEST_CHECK(fixture != NULL);
    ++fixture->delay_calls;
    fixture->total_delay_ms += milliseconds;
    fixture->now_ms += milliseconds;
    if (fixture->disconnect_on_delay_call != 0U &&
        fixture->delay_calls == fixture->disconnect_on_delay_call) {
        fixture->state = USB_KEYBOARD_DISCONNECTED;
        fixture->mounted = false;
    }
}

static bool fake_mounted(void *context)
{
    const usb_fixture_t *fixture = context;
    TEST_CHECK(fixture != NULL);
    return fixture->mounted;
}

static bool fake_suspended(void *context)
{
    const usb_fixture_t *fixture = context;
    TEST_CHECK(fixture != NULL);
    return fixture->suspended;
}

static bool fake_hid_ready(void *context)
{
    usb_fixture_t *fixture = context;
    TEST_CHECK(fixture != NULL);
    ++fixture->ready_calls;
    return fixture->ready_after_calls != SIZE_MAX &&
           fixture->ready_calls >= fixture->ready_after_calls;
}

static bool fake_send_report(void *context,
                             uint8_t report_id,
                             uint8_t modifiers,
                             const uint8_t keycodes[6])
{
    usb_fixture_t *fixture = context;
    TEST_CHECK(fixture != NULL);
    TEST_CHECK(keycodes != NULL);
    ++fixture->report_calls;
    fixture->report_id = report_id;
    fixture->modifiers = modifiers;
    memcpy(fixture->keycodes, keycodes, sizeof(fixture->keycodes));
    return fixture->report_result;
}

static usb_keyboard_ops_t make_operations(usb_fixture_t *fixture)
{
    return (usb_keyboard_ops_t){
        .context = fixture,
        .state_get = fake_state_get,
        .state_set = fake_state_set,
        .driver_install = fake_driver_install,
        .now_ms = fake_now_ms,
        .delay_ms = fake_delay_ms,
        .mounted = fake_mounted,
        .suspended = fake_suspended,
        .hid_ready = fake_hid_ready,
        .send_keyboard_report = fake_send_report,
    };
}

static void ready_fixture(usb_fixture_t *fixture)
{
    fixture->state = USB_KEYBOARD_READY;
    fixture->mounted = true;
    fixture->suspended = false;
}

static void test_operation_validation(void)
{
    usb_fixture_t fixture;
    reset_fixture(&fixture);
    usb_keyboard_ops_t operations = make_operations(&fixture);
    TEST_CHECK(usb_keyboard_ops_is_valid(&operations));
    TEST_CHECK(!usb_keyboard_ops_is_valid(NULL));

#define CHECK_MISSING(member)                                                          \
    do {                                                                               \
        operations = make_operations(&fixture);                                        \
        operations.member = NULL;                                                      \
        TEST_CHECK(!usb_keyboard_ops_is_valid(&operations));                           \
        TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,                                  \
                          usb_keyboard_state_init(&operations));                       \
    } while (0)

    CHECK_MISSING(state_get);
    CHECK_MISSING(state_set);
    CHECK_MISSING(driver_install);
    CHECK_MISSING(now_ms);
    CHECK_MISSING(delay_ms);
    CHECK_MISSING(mounted);
    CHECK_MISSING(suspended);
    CHECK_MISSING(hid_ready);
    CHECK_MISSING(send_keyboard_report);

#undef CHECK_MISSING
}

static void test_initialization(void)
{
    usb_fixture_t fixture;
    reset_fixture(&fixture);
    usb_keyboard_ops_t operations = make_operations(&fixture);

    TEST_CHECK_EQ_INT(APP_ERROR_NONE, usb_keyboard_state_init(&operations));
    TEST_CHECK(fixture.install_observed_enumerating);
    TEST_CHECK_EQ_INT(USB_KEYBOARD_DISCONNECTED, fixture.state);
    TEST_CHECK_EQ_INT(APP_ERROR_CONFLICT, usb_keyboard_state_init(&operations));

    reset_fixture(&fixture);
    fixture.install_result = APP_ERROR_INTERNAL;
    operations = make_operations(&fixture);
    TEST_CHECK_EQ_INT(APP_ERROR_INTERNAL, usb_keyboard_state_init(&operations));
    TEST_CHECK(fixture.install_observed_enumerating);
    TEST_CHECK_EQ_INT(USB_KEYBOARD_ERROR, fixture.state);
}

static void test_callbacks_and_resume(void)
{
    usb_fixture_t fixture;
    reset_fixture(&fixture);
    usb_keyboard_ops_t operations = make_operations(&fixture);

    usb_keyboard_state_mount(&operations);
    TEST_CHECK_EQ_INT(USB_KEYBOARD_READY, fixture.state);
    usb_keyboard_state_suspend(&operations);
    TEST_CHECK_EQ_INT(USB_KEYBOARD_SUSPENDED, fixture.state);

    fixture.mounted = true;
    usb_keyboard_state_resume(&operations);
    TEST_CHECK_EQ_INT(USB_KEYBOARD_READY, fixture.state);
    fixture.mounted = false;
    usb_keyboard_state_resume(&operations);
    TEST_CHECK_EQ_INT(USB_KEYBOARD_DISCONNECTED, fixture.state);

    usb_keyboard_state_mount(&operations);
    usb_keyboard_state_unmount(&operations);
    TEST_CHECK_EQ_INT(USB_KEYBOARD_DISCONNECTED, fixture.state);

    usb_keyboard_state_mount(NULL);
    usb_keyboard_state_unmount(NULL);
    usb_keyboard_state_suspend(NULL);
    usb_keyboard_state_resume(NULL);
}

static void test_press_rejects_invalid_states(void)
{
    static const usb_keyboard_state_t rejected_states[] = {
        USB_KEYBOARD_UNINITIALIZED,
        USB_KEYBOARD_DISCONNECTED,
        USB_KEYBOARD_ENUMERATING,
        USB_KEYBOARD_SUSPENDED,
        USB_KEYBOARD_ERROR,
    };
    for (size_t index = 0U;
         index < (sizeof(rejected_states) / sizeof(rejected_states[0]));
         ++index) {
        usb_fixture_t fixture;
        reset_fixture(&fixture);
        fixture.state = rejected_states[index];
        fixture.mounted = true;
        usb_keyboard_ops_t operations = make_operations(&fixture);
        TEST_CHECK_EQ_INT(APP_ERROR_USB_NOT_READY,
                          usb_keyboard_state_press(&operations, 0x02U, 0x04U));
        TEST_CHECK_EQ_U64(0U, fixture.report_calls);
    }

    usb_fixture_t fixture;
    reset_fixture(&fixture);
    ready_fixture(&fixture);
    usb_keyboard_ops_t operations = make_operations(&fixture);
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      usb_keyboard_state_press(&operations, 0U, 0U));
    fixture.mounted = false;
    TEST_CHECK_EQ_INT(APP_ERROR_USB_NOT_READY,
                      usb_keyboard_state_press(&operations, 0U, 0x04U));
    fixture.mounted = true;
    fixture.suspended = true;
    TEST_CHECK_EQ_INT(APP_ERROR_USB_NOT_READY,
                      usb_keyboard_state_press(&operations, 0U, 0x04U));
}

static void test_press_reports_and_waits(void)
{
    usb_fixture_t fixture;
    reset_fixture(&fixture);
    ready_fixture(&fixture);
    usb_keyboard_ops_t operations = make_operations(&fixture);

    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      usb_keyboard_state_press(&operations, 0x03U, 0x2aU));
    TEST_CHECK_EQ_U64(1U, fixture.report_calls);
    TEST_CHECK_EQ_INT(0U, fixture.report_id);
    TEST_CHECK_EQ_INT(0x03U, fixture.modifiers);
    static const uint8_t expected[6] = {0x2aU, 0U, 0U, 0U, 0U, 0U};
    TEST_CHECK_EQ_BUFFER(expected, fixture.keycodes, sizeof(expected));
    TEST_CHECK_EQ_U64(0U, fixture.delay_calls);

    reset_fixture(&fixture);
    ready_fixture(&fixture);
    fixture.ready_after_calls = 4U;
    operations = make_operations(&fixture);
    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      usb_keyboard_state_press(&operations, 0U, 0x04U));
    TEST_CHECK_EQ_U64(4U, fixture.ready_calls);
    TEST_CHECK_EQ_U64(3U, fixture.delay_calls);
    TEST_CHECK_EQ_U64(3U, fixture.total_delay_ms);
}

static void test_press_stops_on_disconnect_and_timeout(void)
{
    usb_fixture_t fixture;
    reset_fixture(&fixture);
    ready_fixture(&fixture);
    fixture.ready_after_calls = SIZE_MAX;
    fixture.disconnect_on_delay_call = 2U;
    usb_keyboard_ops_t operations = make_operations(&fixture);
    TEST_CHECK_EQ_INT(APP_ERROR_USB_NOT_READY,
                      usb_keyboard_state_press(&operations, 0U, 0x04U));
    TEST_CHECK_EQ_U64(0U, fixture.report_calls);
    TEST_CHECK_EQ_U64(2U, fixture.delay_calls);

    reset_fixture(&fixture);
    ready_fixture(&fixture);
    fixture.ready_after_calls = SIZE_MAX;
    operations = make_operations(&fixture);
    TEST_CHECK_EQ_INT(APP_ERROR_TIMEOUT,
                      usb_keyboard_state_press(&operations, 0U, 0x04U));
    TEST_CHECK_EQ_U64(USB_KEYBOARD_READY_TIMEOUT_MS, fixture.delay_calls);
    TEST_CHECK_EQ_U64(0U, fixture.report_calls);

    reset_fixture(&fixture);
    ready_fixture(&fixture);
    fixture.ready_after_calls = SIZE_MAX;
    fixture.now_ms = UINT32_MAX - 50U;
    operations = make_operations(&fixture);
    TEST_CHECK_EQ_INT(APP_ERROR_TIMEOUT,
                      usb_keyboard_state_press(&operations, 0U, 0x04U));
    TEST_CHECK_EQ_U64(USB_KEYBOARD_READY_TIMEOUT_MS, fixture.delay_calls);
}

static void test_report_failure(void)
{
    usb_fixture_t fixture;
    reset_fixture(&fixture);
    ready_fixture(&fixture);
    fixture.report_result = false;
    usb_keyboard_ops_t operations = make_operations(&fixture);
    TEST_CHECK_EQ_INT(APP_ERROR_IO,
                      usb_keyboard_state_press(&operations, 0U, 0x04U));
    TEST_CHECK_EQ_U64(1U, fixture.report_calls);
}

static void test_release_all(void)
{
    usb_fixture_t fixture;
    reset_fixture(&fixture);
    usb_keyboard_ops_t operations = make_operations(&fixture);
    TEST_CHECK_EQ_INT(APP_ERROR_USB_NOT_READY,
                      usb_keyboard_state_release_all(&operations));

    ready_fixture(&fixture);
    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      usb_keyboard_state_release_all(&operations));
    TEST_CHECK_EQ_U64(1U, fixture.report_calls);
    TEST_CHECK_EQ_INT(0U, fixture.report_id);
    TEST_CHECK_EQ_INT(0U, fixture.modifiers);
    static const uint8_t expected[6] = {0U, 0U, 0U, 0U, 0U, 0U};
    TEST_CHECK_EQ_BUFFER(expected, fixture.keycodes, sizeof(expected));

    reset_fixture(&fixture);
    ready_fixture(&fixture);
    fixture.suspended = true;
    operations = make_operations(&fixture);
    TEST_CHECK_EQ_INT(APP_ERROR_USB_NOT_READY,
                      usb_keyboard_state_release_all(&operations));

    reset_fixture(&fixture);
    ready_fixture(&fixture);
    fixture.report_result = false;
    operations = make_operations(&fixture);
    TEST_CHECK_EQ_INT(APP_ERROR_IO,
                      usb_keyboard_state_release_all(&operations));
}

int main(void)
{
    test_operation_validation();
    test_initialization();
    test_callbacks_and_resume();
    test_press_rejects_invalid_states();
    test_press_reports_and_waits();
    test_press_stops_on_disconnect_and_timeout();
    test_report_failure();
    test_release_all();
    puts("USB keyboard tests passed");
    return EXIT_SUCCESS;
}
