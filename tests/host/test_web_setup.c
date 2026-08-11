/*
 * LEGACY COVERAGE ONLY: this test exercises retained V1-shaped setup reference code that
 * is not linked into shipped firmware. Production POST /api/v1/setup coverage lives in
 * the web_server_setup_submit tests; passing this target is not production setup coverage.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_assert.h"
#include "web_setup_core.h"

typedef struct {
    provisioning_config_t current;
    app_error_code_t load_error;
    app_error_code_t password_error;
    app_error_code_t commit_error;
    app_error_code_t confirmation_error;
    app_error_code_t restart_error;
    unsigned int load_calls;
    unsigned int password_calls;
    unsigned int commit_calls;
    unsigned int confirmation_calls;
    unsigned int restart_calls;
    unsigned int zero_calls;
    size_t zero_bytes;
    uint32_t expected_revision;
    provisioning_config_t committed_input;
} fake_setup_t;

static app_error_code_t fake_load(void *context, provisioning_config_t *out_configuration) {
    fake_setup_t *fake = context;
    ++fake->load_calls;
    if (fake->load_error != APP_ERROR_NONE) {
        return fake->load_error;
    }
    *out_configuration = fake->current;
    return APP_ERROR_NONE;
}

static app_error_code_t fake_password_create(void *context, const char *password,
                                             size_t password_length,
                                             auth_password_record_t *out_record) {
    fake_setup_t *fake = context;
    ++fake->password_calls;
    if (fake->password_error != APP_ERROR_NONE) {
        return fake->password_error;
    }
    TEST_CHECK(password != NULL);
    TEST_CHECK_EQ_U64(strlen(password), password_length);
    memset(out_record->salt, 0x11, sizeof(out_record->salt));
    memset(out_record->hash, 0x22, sizeof(out_record->hash));
    out_record->iterations = AUTH_PBKDF2_ITERATIONS;
    return APP_ERROR_NONE;
}

static app_error_code_t fake_commit(void *context, const provisioning_config_t *replacement,
                                    uint32_t expected_revision,
                                    provisioning_config_t *out_committed) {
    fake_setup_t *fake = context;
    ++fake->commit_calls;
    fake->expected_revision = expected_revision;
    fake->committed_input = *replacement;
    if (fake->commit_error != APP_ERROR_NONE) {
        return fake->commit_error;
    }
    *out_committed = *replacement;
    out_committed->revision = expected_revision + 1U;
    fake->current = *out_committed;
    return APP_ERROR_NONE;
}

static app_error_code_t fake_wait_confirmation(void *context, uint32_t timeout_ms) {
    fake_setup_t *fake = context;
    ++fake->confirmation_calls;
    TEST_CHECK_EQ_U64(WEB_SETUP_CONFIRMATION_TIMEOUT_MS, timeout_ms);
    return fake->confirmation_error;
}

static app_error_code_t fake_schedule_restart(void *context) {
    fake_setup_t *fake = context;
    ++fake->restart_calls;
    return fake->restart_error;
}

static void fake_secure_zero(void *context, void *memory, size_t size) {
    fake_setup_t *fake = context;
    memset(memory, 0, size);
    ++fake->zero_calls;
    fake->zero_bytes += size;
}

static web_setup_ops_t operations(fake_setup_t *fake) {
    return (web_setup_ops_t){
        .context = fake,
        .provisioning_load = fake_load,
        .password_create = fake_password_create,
        .provisioning_commit = fake_commit,
        .wait_for_confirmation = fake_wait_confirmation,
        .schedule_restart = fake_schedule_restart,
        .secure_zero = fake_secure_zero,
    };
}

static web_setup_configuration_t configuration(void) {
    web_setup_configuration_t value = {
        .physical_confirmation_required = true,
        .manufacturing_bypass = false,
    };
    TEST_CHECK_EQ_INT(12, snprintf(value.device_id, sizeof(value.device_id), "%s", "102030A0B0C0"));
    TEST_CHECK_EQ_INT(18,
                      snprintf(value.ap_ssid, sizeof(value.ap_ssid), "%s", "ESP32-Macro-A0B0C0"));
    TEST_CHECK_EQ_INT(
        24, snprintf(value.setup_code, sizeof(value.setup_code), "%s", "45175C9BB39D8BE5FC7EF773"));
    return value;
}

static web_setup_submission_t submission(void) {
    web_setup_submission_t value = {
        .require_physical_confirmation = true,
        .always_select_package = true,
    };
    TEST_CHECK_EQ_INT(
        24, snprintf(value.setup_code, sizeof(value.setup_code), "%s", "45175C9BB39D8BE5FC7EF773"));
    TEST_CHECK_EQ_INT(14, snprintf(value.ap_ssid, sizeof(value.ap_ssid), "%s", "Macro Keyboard"));
    TEST_CHECK_EQ_INT(21, snprintf(value.ap_passphrase, sizeof(value.ap_passphrase), "%s",
                                   "correct-horse-battery"));
    TEST_CHECK_EQ_INT(21,
                      snprintf(value.administrator_password, sizeof(value.administrator_password),
                               "%s", "admin-password-strong"));
    return value;
}

static void initialize(web_setup_core_t *core, fake_setup_t *fake) {
    fake->current = (provisioning_config_t){
        .schema_version = APP_SCHEMA_VERSION,
        .revision = 0U,
        .require_physical_confirmation = true,
        .always_select_package = true,
    };
    const web_setup_ops_t ops = operations(fake);
    const web_setup_configuration_t config = configuration();
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_setup_core_init(core, &ops, &config));
}

static void test_init_and_state(void) {
    fake_setup_t fake = {0};
    web_setup_core_t core = {0};
    web_setup_ops_t ops = operations(&fake);
    web_setup_configuration_t config = configuration();
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, web_setup_core_init(NULL, &ops, &config));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, web_setup_core_init(&core, NULL, &config));
    config.setup_code[0] = 'x';
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, web_setup_core_init(&core, &ops, &config));

    config = configuration();
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_setup_core_init(&core, &ops, &config));
    const web_setup_state_t state = web_setup_core_get_state(&core);
    TEST_CHECK_EQ_STRING("102030A0B0C0", state.device_id);
    TEST_CHECK_EQ_STRING("ESP32-Macro-A0B0C0", state.ap_ssid);
    TEST_CHECK(!state.completed);
    TEST_CHECK(state.physical_confirmation_required);
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, web_setup_core_restart(&core));
    TEST_CHECK_EQ_U64(0U, fake.restart_calls);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_setup_core_deinit(&core));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_setup_core_deinit(&core));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, web_setup_core_deinit(NULL));
    const web_setup_state_t empty = web_setup_core_get_state(NULL);
    TEST_CHECK_EQ_STRING("", empty.device_id);
}

static void test_success_requires_code_and_confirmation(void) {
    fake_setup_t fake = {0};
    web_setup_core_t core;
    initialize(&core, &fake);
    web_setup_submission_t request = submission();
    provisioning_config_t committed;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_setup_core_submit(&core, &request, &committed));
    TEST_CHECK_EQ_U64(1U, fake.confirmation_calls);
    TEST_CHECK_EQ_U64(1U, fake.load_calls);
    TEST_CHECK_EQ_U64(1U, fake.password_calls);
    TEST_CHECK_EQ_U64(1U, fake.commit_calls);
    TEST_CHECK_EQ_U64(0U, fake.expected_revision);
    TEST_CHECK(committed.provisioned);
    TEST_CHECK_EQ_U64(1U, committed.revision);
    TEST_CHECK_EQ_STRING("Macro Keyboard", committed.ap_ssid);
    TEST_CHECK_EQ_STRING("correct-horse-battery", committed.ap_passphrase);
    TEST_CHECK_EQ_U64(AUTH_PBKDF2_ITERATIONS, committed.password_record.iterations);
    TEST_CHECK(core.completed);
    TEST_CHECK_EQ_STRING("", core.configuration.setup_code);
    const uint8_t zero_request[sizeof(request)] = {0};
    TEST_CHECK(memcmp(&request, zero_request, sizeof(request)) == 0);
    TEST_CHECK(fake.zero_calls >= 6U);

    const web_setup_state_t state = web_setup_core_get_state(&core);
    TEST_CHECK(state.completed);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_setup_core_restart(&core));
    TEST_CHECK_EQ_U64(1U, fake.restart_calls);

    request = submission();
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, web_setup_core_submit(&core, &request, &committed));
    TEST_CHECK(memcmp(zero_request, &request, sizeof(request)) == 0);
}

/* SPEC 15.2: a stored station network is cleared by an explicit empty SSID, and
 * by nothing else. First-run setup claims the device -- it sets the AP
 * credentials, the administrator password, and two policy flags -- and says
 * nothing about the network the device was told to join. Discarding it here is
 * a side effect the specification does not authorise.
 *
 * This is the third time the same defect has appeared: a writer that rebuilds a
 * shared record with a designated initialiser, so every field it does not name
 * is silently zeroed. storage_package_reorder dropped the active set the same way.
 * Found on hardware -- the device came back from setup on its access point
 * alone, having forgotten the network it had been joining since boot. */
static void test_setup_preserves_a_stored_station_network(void) {
    fake_setup_t fake = {0};
    web_setup_core_t core;
    initialize(&core, &fake);
    fake.current.has_station = true;
    TEST_CHECK_EQ_INT(11, snprintf(fake.current.station_ssid, sizeof(fake.current.station_ssid),
                                   "%s", "example-net"));
    TEST_CHECK_EQ_INT(8, snprintf(fake.current.station_password,
                                  sizeof(fake.current.station_password), "%s", "not-real"));

    web_setup_submission_t request = submission();
    provisioning_config_t committed;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_setup_core_submit(&core, &request, &committed));

    TEST_CHECK(committed.has_station);
    TEST_CHECK_EQ_STRING("example-net", committed.station_ssid);
    TEST_CHECK_EQ_STRING("not-real", committed.station_password);
}

static void test_wrong_code_fails_before_storage(void) {
    fake_setup_t fake = {0};
    web_setup_core_t core;
    initialize(&core, &fake);
    web_setup_submission_t request = submission();
    request.setup_code[0] = '0';
    provisioning_config_t committed;
    TEST_CHECK_APP_ERROR(APP_ERROR_AUTH_FAILED, web_setup_core_submit(&core, &request, &committed));
    TEST_CHECK_EQ_U64(0U, fake.confirmation_calls);
    TEST_CHECK_EQ_U64(0U, fake.load_calls);
    TEST_CHECK_EQ_U64(0U, fake.password_calls);
    TEST_CHECK_EQ_U64(0U, fake.commit_calls);
}

static void test_confirmation_failure(void) {
    fake_setup_t fake = {.confirmation_error = APP_ERROR_TIMEOUT};
    web_setup_core_t core;
    initialize(&core, &fake);
    web_setup_submission_t request = submission();
    provisioning_config_t committed;
    TEST_CHECK_APP_ERROR(APP_ERROR_TIMEOUT, web_setup_core_submit(&core, &request, &committed));
    TEST_CHECK_EQ_U64(1U, fake.confirmation_calls);
    TEST_CHECK_EQ_U64(0U, fake.load_calls);
}

static void test_manufacturing_bypass(void) {
    fake_setup_t fake = {0};
    web_setup_core_t core;
    fake.current = (provisioning_config_t){.schema_version = APP_SCHEMA_VERSION};
    web_setup_ops_t ops = operations(&fake);
    web_setup_configuration_t config = configuration();
    config.manufacturing_bypass = true;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_setup_core_init(&core, &ops, &config));
    const web_setup_state_t state = web_setup_core_get_state(&core);
    TEST_CHECK(!state.physical_confirmation_required);
    web_setup_submission_t request = submission();
    provisioning_config_t committed;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_setup_core_submit(&core, &request, &committed));
    TEST_CHECK_EQ_U64(0U, fake.confirmation_calls);
}

static void test_backend_failures(void) {
    fake_setup_t fake = {.load_error = APP_ERROR_STORAGE_UNAVAILABLE};
    web_setup_core_t core;
    initialize(&core, &fake);
    web_setup_submission_t request = submission();
    provisioning_config_t committed;
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE,
                         web_setup_core_submit(&core, &request, &committed));

    fake = (fake_setup_t){.password_error = APP_ERROR_INTERNAL};
    initialize(&core, &fake);
    request = submission();
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, web_setup_core_submit(&core, &request, &committed));
    TEST_CHECK_EQ_U64(0U, fake.commit_calls);

    fake = (fake_setup_t){.commit_error = APP_ERROR_IO};
    initialize(&core, &fake);
    request = submission();
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, web_setup_core_submit(&core, &request, &committed));
    TEST_CHECK(!core.completed);
    TEST_CHECK(core.configuration.setup_code[0] != '\0');

    fake = (fake_setup_t){0};
    initialize(&core, &fake);
    fake.current.provisioned = true;
    request = submission();
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, web_setup_core_submit(&core, &request, &committed));
    TEST_CHECK_EQ_U64(0U, fake.password_calls);
}

static void test_submission_validation(void) {
    fake_setup_t fake = {0};
    web_setup_core_t core;
    initialize(&core, &fake);
    provisioning_config_t committed;
    web_setup_submission_t request = submission();
    request.ap_ssid[0] = '\0';
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_setup_core_submit(&core, &request, &committed));
    const uint8_t zero_request[sizeof(request)] = {0};
    TEST_CHECK(memcmp(zero_request, &request, sizeof(request)) == 0);
    request = submission();
    request.ap_passphrase[11] = '\0';
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_setup_core_submit(&core, &request, &committed));
    TEST_CHECK(memcmp(zero_request, &request, sizeof(request)) == 0);
    request = submission();
    request.administrator_password[11] = '\0';
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_setup_core_submit(&core, &request, &committed));
    TEST_CHECK(memcmp(zero_request, &request, sizeof(request)) == 0);
    request = submission();
    request.setup_code[24] = 'X';
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_setup_core_submit(&core, &request, &committed));
    TEST_CHECK(memcmp(zero_request, &request, sizeof(request)) == 0);
}

static void test_restart_failure_visible(void) {
    fake_setup_t fake = {.restart_error = APP_ERROR_INTERNAL};
    web_setup_core_t core;
    initialize(&core, &fake);
    web_setup_submission_t request = submission();
    provisioning_config_t committed;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_setup_core_submit(&core, &request, &committed));
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, web_setup_core_restart(&core));
    TEST_CHECK_EQ_U64(1U, fake.restart_calls);
}

int main(void) {
    test_init_and_state();
    test_success_requires_code_and_confirmation();
    test_setup_preserves_a_stored_station_network();
    test_wrong_code_fails_before_storage();
    test_confirmation_failure();
    test_manufacturing_bypass();
    test_backend_failures();
    test_submission_validation();
    test_restart_failure_visible();
    puts("web setup core tests passed");
    return EXIT_SUCCESS;
}
