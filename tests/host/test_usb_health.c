#include <stdio.h>
#include <stdlib.h>

#include "app_error.h"
#include "test_assert.h"
#include "usb_health.h"

static void test_default_is_healthy(void) {
    usb_health_reset();
    const usb_health_t health = usb_health_snapshot();
    TEST_CHECK_EQ_INT(SUBSYSTEM_HEALTH_HEALTHY, health.state);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, health.primary_error);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, health.cleanup_error);
    TEST_CHECK(!health.cleanup_incomplete);
}

static void test_primary_failure_reports_failed(void) {
    usb_health_reset();
    usb_health_record_primary(APP_ERROR_USB_NOT_READY);
    const usb_health_t health = usb_health_snapshot();
    TEST_CHECK_EQ_INT(SUBSYSTEM_HEALTH_FAILED, health.state);
    TEST_CHECK_APP_ERROR(APP_ERROR_USB_NOT_READY, health.primary_error);
}

static void test_primary_failure_first_wins(void) {
    usb_health_reset();
    usb_health_record_primary(APP_ERROR_USB_NOT_READY);
    usb_health_record_primary(APP_ERROR_IO);
    const usb_health_t health = usb_health_snapshot();
    TEST_CHECK_APP_ERROR(APP_ERROR_USB_NOT_READY, health.primary_error);
}

static void test_cleanup_incomplete_alone_reports_failed(void) {
    usb_health_reset();
    usb_health_record_cleanup(APP_ERROR_NONE, true);
    const usb_health_t health = usb_health_snapshot();
    TEST_CHECK_EQ_INT(SUBSYSTEM_HEALTH_FAILED, health.state);
    TEST_CHECK(health.cleanup_incomplete);
}

static void test_cleanup_none_and_complete_is_noop(void) {
    usb_health_reset();
    usb_health_record_cleanup(APP_ERROR_NONE, false);
    const usb_health_t health = usb_health_snapshot();
    TEST_CHECK_EQ_INT(SUBSYSTEM_HEALTH_HEALTHY, health.state);
}

int main(void) {
    test_default_is_healthy();
    test_primary_failure_reports_failed();
    test_primary_failure_first_wins();
    test_cleanup_incomplete_alone_reports_failed();
    test_cleanup_none_and_complete_is_noop();
    puts("usb health tests passed");
    return EXIT_SUCCESS;
}
