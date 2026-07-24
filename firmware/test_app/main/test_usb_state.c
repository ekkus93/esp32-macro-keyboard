#include "unity.h"
#include "usb_keyboard.h"

TEST_CASE("USB keyboard initializes without emitting a key", "[device][usb]") {
    const app_error_code_t init_result = usb_keyboard_init();
    TEST_ASSERT_TRUE(init_result == APP_ERROR_NONE || init_result == APP_ERROR_CONFLICT);

    const usb_keyboard_state_t state = usb_keyboard_get_state();
    TEST_ASSERT_TRUE(state == USB_KEYBOARD_DISCONNECTED || state == USB_KEYBOARD_ENUMERATING ||
                     state == USB_KEYBOARD_READY || state == USB_KEYBOARD_SUSPENDED);
    TEST_ASSERT_EQUAL(APP_ERROR_INVALID_ARGUMENT, usb_keyboard_press(0U, 0U));

    if (state != USB_KEYBOARD_READY) {
        TEST_ASSERT_EQUAL(APP_ERROR_USB_NOT_READY, usb_keyboard_press(0U, 0x04U));
    }
}
