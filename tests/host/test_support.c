#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fake_call_log.h"
#include "fake_clock.h"
#include "fake_freertos.h"
#include "fake_fs_backend.h"
#include "fake_gpio_backend.h"
#include "fake_http_backend.h"
#include "fake_random.h"
#include "fake_usb_backend.h"
#include "fake_wifi_backend.h"
#include "test_assert.h"
#include "test_memory.h"
#include "test_temp_dir.h"

static void test_call_log(void)
{
    fake_call_log_t log;
    fake_call_log_reset(&log);
    fake_call_log_set_strict(&log, true);
    fake_call_log_expect(&log, "first");
    fake_call_log_expect(&log, "second");
    TEST_CHECK(!fake_call_log_record(&log, "first", 1U, 2U));
    fake_call_log_fail_on(&log, "second", 1U);
    TEST_CHECK(fake_call_log_record(&log, "second", 3U, 4U));
    fake_call_log_verify(&log);
    TEST_CHECK_EQ_U64(2U, log.call_count);
    TEST_CHECK_EQ_STRING("second", fake_call_log_at(&log, 1U)->name);
}

static void test_memory(void)
{
    test_memory_reset();
    char *buffer = test_calloc(4U, sizeof(*buffer));
    TEST_CHECK(buffer != NULL);
    TEST_CHECK_EQ_U64(1U, test_memory_outstanding_allocations());
    TEST_CHECK_EQ_U64(4U, test_memory_outstanding_bytes());
    memcpy(buffer, "abc", 4U);
    buffer = test_realloc(buffer, 8U);
    TEST_CHECK(buffer != NULL);
    TEST_CHECK_EQ_STRING("abc", buffer);
    TEST_CHECK_EQ_U64(8U, test_memory_outstanding_bytes());
    test_free(buffer);
    TEST_CHECK_EQ_U64(0U, test_memory_outstanding_allocations());
    TEST_CHECK_EQ_U64(0U, test_memory_outstanding_bytes());
    test_memory_reset();
}

static void test_temp_directory_and_filesystem(void)
{
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char path[TEST_TEMP_DIR_PATH_MAX];
    const int written = snprintf(path, sizeof(path), "%s/value.bin", directory.path);
    TEST_CHECK(written > 0 && (size_t)written < sizeof(path));

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    const int descriptor = fake_fs_open(&filesystem, path, O_CREAT | O_WRONLY, 0600);
    TEST_CHECK(descriptor >= 0);
    static const char value[] = "payload";
    fake_fs_backend_set_short_write(&filesystem, 3U);
    TEST_CHECK_EQ_INT(3, fake_fs_write(&filesystem, descriptor, value, sizeof(value) - 1U));
    TEST_CHECK(fake_fs_close(&filesystem, descriptor) == 0);
    TEST_CHECK(unlink(path) == 0);
    test_temp_dir_remove(&directory);
}

static void test_deterministic_fakes(void)
{
    fake_clock_t clock;
    fake_clock_reset(&clock);
    fake_clock_set_us(&clock, 1000U);
    fake_clock_advance_us(&clock, 2500U);
    TEST_CHECK_EQ_U64(3500U, fake_clock_now_us(&clock));
    TEST_CHECK_EQ_U64(3U, fake_clock_now_ms(&clock));

    fake_random_t random;
    fake_random_reset(&random);
    static const uint8_t sequence[] = {1U, 2U, 3U};
    fake_random_set(&random, sequence, sizeof(sequence), true);
    uint8_t output[5U];
    TEST_CHECK(fake_random_fill(&random, output, sizeof(output)));
    static const uint8_t expected[] = {1U, 2U, 3U, 1U, 2U};
    TEST_CHECK_EQ_BUFFER(expected, output, sizeof(output));

    fake_freertos_t freertos;
    fake_freertos_reset(&freertos);
    TEST_CHECK(fake_freertos_lock(&freertos));
    TEST_CHECK(fake_freertos_unlock(&freertos));
    fake_freertos_notify(&freertos);
    TEST_CHECK(fake_freertos_wait(&freertos, 10U));
    TEST_CHECK_EQ_U64(10U, freertos.elapsed_ms);

    fake_usb_backend_t usb;
    fake_usb_backend_reset(&usb);
    TEST_CHECK_EQ_INT(0, fake_usb_backend_press(&usb, 2U, 4U));
    TEST_CHECK_EQ_INT(0, fake_usb_backend_release_all(&usb));
    TEST_CHECK_EQ_U64(1U, usb.press_count);
    TEST_CHECK_EQ_U64(1U, usb.release_count);

    fake_gpio_backend_t gpio;
    fake_gpio_backend_reset(&gpio);
    TEST_CHECK_EQ_INT(0, fake_gpio_backend_configure(&gpio, 2U, 1));
    TEST_CHECK_EQ_INT(0, fake_gpio_backend_set(&gpio, 2U, 1));
    TEST_CHECK_EQ_INT(1, fake_gpio_backend_get(&gpio, 2U));

    fake_wifi_backend_t wifi;
    fake_wifi_backend_reset(&wifi);
    fake_wifi_backend_capture_config(&wifi, "ssid", "abcdefghijkl", 4U);
    TEST_CHECK_EQ_STRING("ssid", (const char *)wifi.configured_ssid);
    TEST_CHECK_EQ_U64(4U, wifi.configured_max_clients);

    fake_http_backend_t http;
    fake_http_backend_reset(&http);
    fake_http_backend_add_header(&http, "Host", "192.168.4.1");
    TEST_CHECK_EQ_STRING("192.168.4.1", fake_http_backend_get_header(&http, "Host"));
    fake_http_backend_set_body(&http, "abcdef", 2U);
    char chunk[3U] = {0};
    TEST_CHECK_EQ_INT(2, fake_http_backend_receive(&http, chunk, 2U));
    TEST_CHECK_EQ_BUFFER("ab", chunk, 2U);
}

int main(void)
{
    test_call_log();
    test_memory();
    test_temp_directory_and_filesystem();
    test_deterministic_fakes();
    puts("test support tests passed");
    return EXIT_SUCCESS;
}
