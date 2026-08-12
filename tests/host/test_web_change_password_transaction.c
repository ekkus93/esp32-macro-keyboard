#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "auth_core.h"
#include "auth_core_internal.h"
#include "device_settings_v2.h"
#include "fake_call_log.h"
#include "setup_contract_v2.h"
#include "test_assert.h"
#include "web_settings.h"

/* Reuse the deterministic, secure-zero-observable auth-core fixture so this
 * test exercises the real password/session implementation while keeping PBKDF2
 * host-fast and deterministic. */
#include "auth_test_fixture.inc"

#define BODY_CAPACITY 256U

typedef struct {
    auth_fake_t auth_fake;
    auth_core_t auth_core;
    app_v2_device_settings_t durable;
    auth_password_record_t active_record;
    bool transition_active;
    app_error_code_t transition_begin_error;
    app_error_code_t replace_error;
    app_error_code_t create_error;
    app_error_code_t invalidate_error;
    unsigned int replace_calls;
    unsigned int activate_calls;
    unsigned int invalidate_calls;
} h2_fixture_t;

static void copy_record_to_settings(const auth_password_record_t *record,
                                    app_v2_device_settings_t *settings) {
    settings->credential_version = APP_V2_CREDENTIAL_VERSION;
    settings->password_algorithm_version = APP_V2_PASSWORD_ALGORITHM_VERSION;
    settings->password_iterations = record->iterations;
    memcpy(settings->password_salt, record->salt, sizeof(settings->password_salt));
    memcpy(settings->password_verifier, record->hash, sizeof(settings->password_verifier));
}

static auth_password_record_t record_from_settings(const app_v2_device_settings_t *settings) {
    auth_password_record_t record = {.iterations = settings->password_iterations};
    memcpy(record.salt, settings->password_salt, sizeof(record.salt));
    memcpy(record.hash, settings->password_verifier, sizeof(record.hash));
    return record;
}

static void init_h2_fixture(h2_fixture_t *fixture, const char *password) {
    memset(fixture, 0, sizeof(*fixture));
    fake_reset(&fixture->auth_fake);
    fixture->auth_fake.now_us = UINT64_C(1000000);
    init_core(&fixture->auth_core, &fixture->auth_fake);

    auth_password_record_t record = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, auth_core_password_create(&fixture->auth_core, password,
                                                                   strlen(password), &record));
    fixture->active_record = record;
    app_v2_device_settings_init_unprovisioned(&fixture->durable);
    fixture->durable.provisioned = true;
    copy_record_to_settings(&record, &fixture->durable);
    memcpy(fixture->durable.device_name, "Desk Macro Keyboard", sizeof("Desk Macro Keyboard"));
    memcpy(fixture->durable.ap_ssid, "MacroKeyboard", sizeof("MacroKeyboard"));
    memcpy(fixture->durable.ap_passphrase, "example-passphrase", sizeof("example-passphrase"));
    memset(&record, 0, sizeof(record));
}

static app_error_code_t h2_settings_read(void *context, app_v2_device_settings_t *out_settings) {
    h2_fixture_t *fixture = context;
    *out_settings = fixture->durable;
    return APP_ERROR_NONE;
}

static app_error_code_t h2_settings_replace(void *context, const app_v2_device_settings_t *settings,
                                            bool *out_changed) {
    h2_fixture_t *fixture = context;
    ++fixture->replace_calls;
    if (fixture->replace_error != APP_ERROR_NONE) {
        *out_changed = false;
        return fixture->replace_error;
    }
    fixture->durable = *settings;
    *out_changed = true;
    return APP_ERROR_NONE;
}

static app_error_code_t h2_password_verify(void *context, const char *password,
                                           size_t password_length,
                                           const app_v2_device_settings_t *settings,
                                           bool *out_matches) {
    h2_fixture_t *fixture = context;
    auth_password_record_t record = record_from_settings(settings);
    const app_error_code_t result = auth_core_password_verify(
        &fixture->auth_core, password, password_length, &record, out_matches);
    memset(&record, 0, sizeof(record));
    return result;
}

static app_error_code_t h2_password_create(void *context, const char *password,
                                           size_t password_length,
                                           app_v2_setup_password_material_t *out_material) {
    h2_fixture_t *fixture = context;
    if (fixture->create_error != APP_ERROR_NONE) {
        memset(out_material, 0, sizeof(*out_material));
        return fixture->create_error;
    }
    auth_password_record_t record = {0};
    const app_error_code_t result =
        auth_core_password_create(&fixture->auth_core, password, password_length, &record);
    if (result == APP_ERROR_NONE) {
        out_material->credential_version = APP_V2_CREDENTIAL_VERSION;
        out_material->password_algorithm_version = APP_V2_PASSWORD_ALGORITHM_VERSION;
        out_material->password_iterations = record.iterations;
        memcpy(out_material->password_salt, record.salt, sizeof(out_material->password_salt));
        memcpy(out_material->password_verifier, record.hash,
               sizeof(out_material->password_verifier));
    }
    memset(&record, 0, sizeof(record));
    return result;
}

static app_error_code_t h2_transition_begin(void *context) {
    h2_fixture_t *fixture = context;
    if (fixture->transition_begin_error != APP_ERROR_NONE) {
        return fixture->transition_begin_error;
    }
    if (fixture->transition_active) {
        return APP_ERROR_CONFLICT;
    }
    fixture->transition_active = true;
    return APP_ERROR_NONE;
}

static void h2_password_activate(void *context, const app_v2_device_settings_t *settings) {
    h2_fixture_t *fixture = context;
    TEST_CHECK(fixture->transition_active);
    ++fixture->activate_calls;
    fixture->active_record = record_from_settings(settings);
}

static void h2_transition_end(void *context) {
    h2_fixture_t *fixture = context;
    TEST_CHECK(fixture->transition_active);
    fixture->transition_active = false;
}

static app_error_code_t h2_invalidate_all_sessions(void *context) {
    h2_fixture_t *fixture = context;
    TEST_CHECK(fixture->transition_active);
    ++fixture->invalidate_calls;
    if (fixture->invalidate_error != APP_ERROR_NONE) {
        return fixture->invalidate_error;
    }
    return auth_core_session_logout_all(&fixture->auth_core);
}

static web_settings_ops_t h2_operations(h2_fixture_t *fixture) {
    return (web_settings_ops_t){
        .context = fixture,
        .settings_read = h2_settings_read,
        .settings_replace = h2_settings_replace,
        .password_verify = h2_password_verify,
        .password_create = h2_password_create,
        .password_transition_begin = h2_transition_begin,
        .password_activate = h2_password_activate,
        .password_transition_end = h2_transition_end,
        .invalidate_all_sessions = h2_invalidate_all_sessions,
    };
}

static void password_body(char *body, size_t capacity, const char *current, const char *next) {
    const int written = snprintf(
        body, capacity, "{\"currentPassword\":\"%s\",\"newPassword\":\"%s\"}", current, next);
    TEST_CHECK(written > 0 && (size_t)written < capacity);
}

static web_change_password_outcome_t change_password(h2_fixture_t *fixture, const char *current,
                                                     const char *next) {
    char body[BODY_CAPACITY] = {0};
    password_body(body, sizeof(body), current, next);
    const web_settings_ops_t ops = h2_operations(fixture);
    const web_change_password_outcome_t outcome =
        web_change_password_handle(body, sizeof(body), &ops);
    for (size_t index = 0U; index < sizeof(body); ++index) {
        TEST_CHECK_EQ_U64(0U, (uint8_t)body[index]);
    }
    return outcome;
}

static bool active_password_matches(h2_fixture_t *fixture, const char *password) {
    bool matches = false;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         auth_core_password_verify(&fixture->auth_core, password, strlen(password),
                                                   &fixture->active_record, &matches));
    return matches;
}

static auth_session_view_t create_session(h2_fixture_t *fixture) {
    auth_session_view_t session = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, auth_core_session_create(&fixture->auth_core, &session));
    return session;
}

static void test_success_is_immediately_coherent(void) {
    static const char old_password[] = "old-example-password";
    static const char new_password[] = "new-example-password";
    h2_fixture_t fixture;
    init_h2_fixture(&fixture, old_password);
    const auth_session_view_t first = create_session(&fixture);
    const auth_session_view_t second = create_session(&fixture);

    const size_t zero_before = fixture.auth_fake.zero_count;
    const web_change_password_outcome_t outcome =
        change_password(&fixture, old_password, new_password);

    TEST_CHECK_EQ_INT(WEB_CHANGE_PASSWORD_OK, outcome.result);
    TEST_CHECK(!fixture.transition_active);
    TEST_CHECK_EQ_U64(1U, fixture.replace_calls);
    TEST_CHECK_EQ_U64(1U, fixture.activate_calls);
    TEST_CHECK_EQ_U64(1U, fixture.invalidate_calls);
    TEST_CHECK(!active_password_matches(&fixture, old_password));
    TEST_CHECK(active_password_matches(&fixture, new_password));
    TEST_CHECK_APP_ERROR(APP_ERROR_AUTH_REQUIRED,
                         auth_core_session_validate(&fixture.auth_core, first.session_token));
    TEST_CHECK_APP_ERROR(APP_ERROR_AUTH_REQUIRED,
                         auth_core_session_validate(&fixture.auth_core, second.session_token));

    /* A session created immediately after the 204 path retains the normal
     * auth-core lifetime semantics; the password transaction does not mutate
     * the TTL/lockout machinery. */
    const auth_session_view_t fresh = create_session(&fixture);
    TEST_CHECK_EQ_U64(fixture.auth_fake.now_us + AUTH_CORE_SESSION_IDLE_US, fresh.expires_at_us);
    TEST_CHECK_EQ_U64(fixture.auth_fake.now_us + AUTH_CORE_SESSION_ABSOLUTE_US,
                      fresh.absolute_expires_at_us);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         auth_core_session_validate(&fixture.auth_core, fresh.session_token));
    const uint32_t source = UINT32_C(0x0A000001);
    uint32_t retry_after_seconds = UINT32_MAX;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, auth_core_login_attempt_allowed(&fixture.auth_core, source,
                                                                         &retry_after_seconds));
    TEST_CHECK_EQ_U64(0U, retry_after_seconds);
    for (size_t index = 0U; index < AUTH_RATE_LIMIT_FAILURE_MAX; ++index) {
        TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                             auth_core_login_record_failure(&fixture.auth_core, source));
    }
    TEST_CHECK_APP_ERROR(
        APP_ERROR_RATE_LIMITED,
        auth_core_login_attempt_allowed(&fixture.auth_core, source, &retry_after_seconds));
    TEST_CHECK_EQ_U64(300U, retry_after_seconds);

    TEST_CHECK(fixture.auth_fake.zero_count > zero_before);
}

static void test_precommit_failures_leave_old_authority_and_retry_cleanly(void) {
    static const char old_password[] = "old-example-password";
    static const char new_password[] = "new-example-password";

    h2_fixture_t create_failure;
    init_h2_fixture(&create_failure, old_password);
    const auth_session_view_t create_session_before = create_session(&create_failure);
    create_failure.create_error = APP_ERROR_INTERNAL;
    web_change_password_outcome_t outcome =
        change_password(&create_failure, old_password, new_password);
    TEST_CHECK_EQ_INT(WEB_CHANGE_PASSWORD_BACKEND_UNAVAILABLE, outcome.result);
    TEST_CHECK(active_password_matches(&create_failure, old_password));
    TEST_CHECK(!active_password_matches(&create_failure, new_password));
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE,
        auth_core_session_validate(&create_failure.auth_core, create_session_before.session_token));
    create_failure.create_error = APP_ERROR_NONE;
    TEST_CHECK_EQ_INT(WEB_CHANGE_PASSWORD_OK,
                      change_password(&create_failure, old_password, new_password).result);

    h2_fixture_t replace_failure;
    init_h2_fixture(&replace_failure, old_password);
    const auth_session_view_t replace_session_before = create_session(&replace_failure);
    replace_failure.replace_error = APP_ERROR_STORAGE_FULL;
    outcome = change_password(&replace_failure, old_password, new_password);
    TEST_CHECK_EQ_INT(WEB_CHANGE_PASSWORD_BACKEND_UNAVAILABLE, outcome.result);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_FULL, outcome.detail);
    TEST_CHECK(active_password_matches(&replace_failure, old_password));
    TEST_CHECK(!active_password_matches(&replace_failure, new_password));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         auth_core_session_validate(&replace_failure.auth_core,
                                                    replace_session_before.session_token));
    TEST_CHECK(!replace_failure.transition_active);
    replace_failure.replace_error = APP_ERROR_NONE;
    TEST_CHECK_EQ_INT(WEB_CHANGE_PASSWORD_OK,
                      change_password(&replace_failure, old_password, new_password).result);

    h2_fixture_t begin_failure;
    init_h2_fixture(&begin_failure, old_password);
    begin_failure.transition_begin_error = APP_ERROR_CONFLICT;
    outcome = change_password(&begin_failure, old_password, new_password);
    TEST_CHECK_EQ_INT(WEB_CHANGE_PASSWORD_BACKEND_UNAVAILABLE, outcome.result);
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, outcome.detail);
    TEST_CHECK_EQ_U64(0U, begin_failure.replace_calls);
    TEST_CHECK(active_password_matches(&begin_failure, old_password));
    begin_failure.transition_begin_error = APP_ERROR_NONE;
    TEST_CHECK_EQ_INT(WEB_CHANGE_PASSWORD_OK,
                      change_password(&begin_failure, old_password, new_password).result);
}

static void test_postcommit_invalidation_failure_names_new_authority_and_retry_semantics(void) {
    static const char old_password[] = "old-example-password";
    static const char new_password[] = "new-example-password";
    static const char third_password[] = "third-example-password";
    h2_fixture_t fixture;
    init_h2_fixture(&fixture, old_password);
    const auth_session_view_t first = create_session(&fixture);
    const auth_session_view_t second = create_session(&fixture);
    fixture.invalidate_error = APP_ERROR_INTERNAL;

    const web_change_password_outcome_t outcome =
        change_password(&fixture, old_password, new_password);
    TEST_CHECK_EQ_INT(WEB_CHANGE_PASSWORD_COMMITTED_SESSION_INVALIDATION_INCOMPLETE,
                      outcome.result);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, outcome.detail);
    TEST_CHECK(!fixture.transition_active);
    TEST_CHECK(!active_password_matches(&fixture, old_password));
    TEST_CHECK(active_password_matches(&fixture, new_password));
    /* The injected invalidation failed before touching the real session table,
     * proving why the caller must be warned rather than told the password
     * change simply failed. */
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         auth_core_session_validate(&fixture.auth_core, first.session_token));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         auth_core_session_validate(&fixture.auth_core, second.session_token));

    /* Blindly retrying the original old->new request is deterministically
     * rejected because the old password is no longer authoritative. */
    TEST_CHECK_EQ_INT(WEB_CHANGE_PASSWORD_INCORRECT_CURRENT_PASSWORD,
                      change_password(&fixture, old_password, new_password).result);

    /* Recovery uses the authoritative new password. Once invalidation works,
     * the next committed change closes every pre-existing session. */
    fixture.invalidate_error = APP_ERROR_NONE;
    TEST_CHECK_EQ_INT(WEB_CHANGE_PASSWORD_OK,
                      change_password(&fixture, new_password, third_password).result);
    TEST_CHECK(!active_password_matches(&fixture, new_password));
    TEST_CHECK(active_password_matches(&fixture, third_password));
    TEST_CHECK_APP_ERROR(APP_ERROR_AUTH_REQUIRED,
                         auth_core_session_validate(&fixture.auth_core, first.session_token));
    TEST_CHECK_APP_ERROR(APP_ERROR_AUTH_REQUIRED,
                         auth_core_session_validate(&fixture.auth_core, second.session_token));
}

int main(void) {
    test_success_is_immediately_coherent();
    test_precommit_failures_leave_old_authority_and_retry_cleanly();
    test_postcommit_invalidation_failure_names_new_authority_and_retry_semantics();
    puts("H2 password transaction integration tests passed");
    return EXIT_SUCCESS;
}
