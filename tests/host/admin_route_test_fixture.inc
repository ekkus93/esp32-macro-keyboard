#define ADMIN_TEST_SESSION_TOKEN                                                                   \
    "0123456789abcdef"                                                                             \
    "0123456789abcdef"                                                                             \
    "0123456789abcdef"                                                                             \
    "0123456789abcdef"

/* ---------------------------------------------------------------------- *
 * Test doubles for auth.c's, device_settings.c's, and device_controls.c's
 * public entry points -- none host-linkable (mbedtls/NVS/GPIO/FreeRTOS),
 * the same narrow-substitution technique test_web_api_administration.c and
 * test_web_server_send_route.c already use -- plus esp_restart()
 * (esp_system.h has no real host implementation either).
 * ---------------------------------------------------------------------- */

static app_error_code_t g_auth_session_validate_result;

app_error_code_t auth_session_validate(const char *session_token) {
    (void)session_token;
    return g_auth_session_validate_result;
}

static app_error_code_t g_session_remaining_result;
static uint32_t g_idle_seconds;
static uint32_t g_absolute_seconds;

app_error_code_t auth_session_remaining(const char *session_token,
                                        uint32_t *out_idle_seconds_remaining,
                                        uint32_t *out_absolute_seconds_remaining) {
    (void)session_token;
    if (g_session_remaining_result != APP_ERROR_NONE) {
        return g_session_remaining_result;
    }
    *out_idle_seconds_remaining = g_idle_seconds;
    *out_absolute_seconds_remaining = g_absolute_seconds;
    return APP_ERROR_NONE;
}

static app_error_code_t g_password_verify_result;
static bool g_password_matches;

app_error_code_t auth_password_verify(const char *password, size_t password_length,
                                      const auth_password_record_t *record, bool *out_matches) {
    (void)password;
    (void)password_length;
    (void)record;
    if (g_password_verify_result != APP_ERROR_NONE) {
        return g_password_verify_result;
    }
    *out_matches = g_password_matches;
    return APP_ERROR_NONE;
}

static app_error_code_t g_password_create_result;

app_error_code_t auth_password_create(const char *password, size_t password_length,
                                      auth_password_record_t *out_record) {
    (void)password;
    (void)password_length;
    if (g_password_create_result != APP_ERROR_NONE) {
        return g_password_create_result;
    }
    memset(out_record, 0, sizeof(*out_record));
    out_record->iterations = AUTH_PBKDF2_ITERATIONS;
    /* Non-zero: app_v2_password_change_prepare_candidate()'s
     * app_v2_device_settings_validate() call (settings_contract_v2.c)
     * rejects an all-zero salt/verifier the same way the real
     * auth_password_create() never would. */
    memset(out_record->salt, 0x77, sizeof(out_record->salt));
    memset(out_record->hash, 0x88, sizeof(out_record->hash));
    return APP_ERROR_NONE;
}

static app_error_code_t g_logout_all_result;
static size_t g_logout_all_calls;

app_error_code_t auth_session_logout_all(void) {
    ++g_logout_all_calls;
    return g_logout_all_result;
}

static app_v2_device_settings_t g_device_settings_record;
static app_error_code_t g_device_settings_read_result;
static app_error_code_t g_device_settings_replace_result;
static bool g_device_settings_replace_changed;

app_error_code_t device_settings_read(app_v2_device_settings_t *out_settings) {
    if (g_device_settings_read_result != APP_ERROR_NONE) {
        return g_device_settings_read_result;
    }
    *out_settings = g_device_settings_record;
    return APP_ERROR_NONE;
}

app_error_code_t device_settings_replace(const app_v2_device_settings_t *settings,
                                         bool *out_changed) {
    if (g_device_settings_replace_result != APP_ERROR_NONE) {
        return g_device_settings_replace_result;
    }
    g_device_settings_record = *settings;
    *out_changed = g_device_settings_replace_changed;
    return APP_ERROR_NONE;
}

static app_error_code_t g_restart_result;
static size_t g_restart_calls;

app_error_code_t device_controls_restart(void) {
    ++g_restart_calls;
    return g_restart_result;
}

static device_controls_reset_settings_outcome_t g_reset_settings_outcome;
static size_t g_reset_settings_calls;

device_controls_reset_settings_outcome_t device_controls_reset_settings(void) {
    ++g_reset_settings_calls;
    return g_reset_settings_outcome;
}

static device_controls_factory_reset_outcome_t g_factory_reset_outcome;
static size_t g_factory_reset_calls;

device_controls_factory_reset_outcome_t device_controls_factory_reset(void) {
    ++g_factory_reset_calls;
    return g_factory_reset_outcome;
}

static app_error_code_t g_factory_reset_state_read_result;
static factory_reset_state_t g_factory_reset_state;

app_error_code_t factory_reset_state_read(factory_reset_state_t *out_state) {
    if (g_factory_reset_state_read_result != APP_ERROR_NONE) {
        return g_factory_reset_state_read_result;
    }
    *out_state = g_factory_reset_state;
    return APP_ERROR_NONE;
}

app_error_code_t device_controls_wait_for_confirmation(unsigned int timeout_ms) {
    (void)timeout_ms;
    /* Never reached: server_configuration.require_physical_confirmation is
     * false for every test below -- see the file header comment. */
    TEST_CHECK(false);
    return APP_ERROR_INTERNAL;
}

static size_t g_esp_restart_calls;

void esp_restart(void) {
    ++g_esp_restart_calls;
}

/* ---------------------------------------------------------------------- *
 * Test doubles for GET /api/v1/diagnostics's own dependencies --
 * web_diagnostics_handle() itself (web_server_diagnostics.c) links in real
 * (see the file header comment for why); these are the handful of
 * NVS/FreeRTOS/hardware-backed entry points its collect_diagnostics() calls
 * that are not host-linkable, the same narrow-substitution technique used
 * throughout this file. Matches test_web_server_status_limits_route.c's
 * device_settings_read/wifi_ap_get_status/macro_executor_get_status/
 * usb_keyboard_get_state/storage_mount_state/storage_partition_capacity
 * fakes exactly (status_handler() and collect_diagnostics() read the same
 * subsystems), plus storage_blob_collect_diagnostics()/
 * device_controls_get_health(), which status/limits do not need.
 * ---------------------------------------------------------------------- */

static wifi_ap_status_t g_wifi_status;

wifi_ap_status_t wifi_ap_get_status(void) {
    return g_wifi_status;
}

static macro_execution_status_t g_execution_status;

macro_execution_status_t macro_executor_get_status(void) {
    return g_execution_status;
}

static usb_keyboard_state_t g_usb_state;

usb_keyboard_state_t usb_keyboard_get_state(void) {
    return g_usb_state;
}

static storage_mount_state_t g_mount_state;

storage_mount_state_t storage_mount_state(void) {
    return g_mount_state;
}

static app_error_code_t g_webfs_capacity_result;
static size_t g_webfs_total_bytes;
static size_t g_webfs_used_bytes;
static app_error_code_t g_userdata_capacity_result;
static size_t g_userdata_total_bytes;
static size_t g_userdata_used_bytes;

app_error_code_t storage_partition_capacity(const char *partition_label, size_t *out_total_bytes,
                                            size_t *out_used_bytes) {
    if (strcmp(partition_label, STORAGE_WEB_PARTITION) == 0) {
        if (g_webfs_capacity_result != APP_ERROR_NONE) {
            return g_webfs_capacity_result;
        }
        *out_total_bytes = g_webfs_total_bytes;
        *out_used_bytes = g_webfs_used_bytes;
        return APP_ERROR_NONE;
    }
    TEST_CHECK_EQ_STRING(STORAGE_DATA_PARTITION, partition_label);
    if (g_userdata_capacity_result != APP_ERROR_NONE) {
        return g_userdata_capacity_result;
    }
    *out_total_bytes = g_userdata_total_bytes;
    *out_used_bytes = g_userdata_used_bytes;
    return APP_ERROR_NONE;
}

static app_error_code_t g_blob_diagnostics_result;
static size_t g_blob_valid_count;

app_error_code_t storage_blob_collect_diagnostics(storage_blob_diagnostics_t *out_diagnostics) {
    if (g_blob_diagnostics_result != APP_ERROR_NONE) {
        return g_blob_diagnostics_result;
    }
    *out_diagnostics = (storage_blob_diagnostics_t){0};
    out_diagnostics->summary.valid_count = g_blob_valid_count;
    return APP_ERROR_NONE;
}

static device_controls_health_t g_device_controls_health;

device_controls_health_t device_controls_get_health(void) {
    return g_device_controls_health;
}

const esp_app_desc_t *esp_app_get_description(void) {
    static const esp_app_desc_t description = {.version = "0.2.0"};
    return &description;
}

int esp_app_get_elf_sha256(char *dst, size_t size) {
    (void)snprintf(dst, size, "%s", "git-abcdef0");
    return 0;
}

int64_t esp_timer_get_time(void) {
    return 123456000; /* 123456 ms. */
}

static esp_reset_reason_t g_reset_reason;

esp_reset_reason_t esp_reset_reason(void) {
    return g_reset_reason;
}

static uint32_t g_free_heap_bytes;

uint32_t esp_get_free_heap_size(void) {
    return g_free_heap_bytes;
}

static uint32_t g_minimum_free_heap_bytes;

uint32_t esp_get_minimum_free_heap_size(void) {
    return g_minimum_free_heap_bytes;
}

static size_t g_largest_free_block_bytes;

size_t heap_caps_get_largest_free_block(uint32_t caps) {
    TEST_CHECK_EQ_U64((uint64_t)MALLOC_CAP_8BIT, (uint64_t)caps);
    return g_largest_free_block_bytes;
}

esp_err_t web_server_async_dispatch(httpd_req_t *request) {
    (void)request;
    /* Never reached: web_api_request_requires_worker() (web_server_api.c)
     * only returns true when physical confirmation is required, which
     * server_configuration.require_physical_confirmation=false disables for
     * every test below -- see the file header comment. Present only so the
     * linker resolves api_handler()'s reference to it. */
    TEST_CHECK(false);
    return ESP_FAIL;
}

/* ---------------------------------------------------------------------- */

static app_v2_device_settings_t provisioned_settings(void) {
    app_v2_device_settings_t settings;
    app_v2_device_settings_init_unprovisioned(&settings);
    settings.provisioned = true;
    settings.credential_version = APP_V2_CREDENTIAL_VERSION;
    settings.password_algorithm_version = APP_V2_PASSWORD_ALGORITHM_VERSION;
    settings.password_iterations = 120000U;
    memset(settings.password_salt, 0x11, sizeof(settings.password_salt));
    memset(settings.password_verifier, 0x22, sizeof(settings.password_verifier));
    memcpy(settings.device_name, "Desk Macro Keyboard", sizeof("Desk Macro Keyboard"));
    memcpy(settings.ap_ssid, "MacroKeyboard", sizeof("MacroKeyboard"));
    memcpy(settings.ap_passphrase, "example-passphrase", sizeof("example-passphrase"));
    return settings;
}

static void reset_fakes(void) {
    g_auth_session_validate_result = APP_ERROR_NONE;
    g_session_remaining_result = APP_ERROR_NONE;
    g_idle_seconds = 86400U;
    g_absolute_seconds = 604800U;
    g_password_verify_result = APP_ERROR_NONE;
    g_password_matches = true;
    g_password_create_result = APP_ERROR_NONE;
    g_logout_all_result = APP_ERROR_NONE;
    g_logout_all_calls = 0U;
    g_device_settings_record = provisioned_settings();
    g_device_settings_read_result = APP_ERROR_NONE;
    g_device_settings_replace_result = APP_ERROR_NONE;
    g_device_settings_replace_changed = true;
    g_restart_result = APP_ERROR_NONE;
    g_restart_calls = 0U;
    g_reset_settings_outcome = (device_controls_reset_settings_outcome_t){
        .settings_applied = true,
        .sessions_invalidated = true,
        .restart_owned = true,
        .primary_error = APP_ERROR_NONE,
        .restart_error = APP_ERROR_NONE,
    };
    g_reset_settings_calls = 0U;
    g_factory_reset_outcome = (device_controls_factory_reset_outcome_t){
        .durably_accepted = true,
        .recovery_required = false,
        .primary_error = APP_ERROR_NONE,
    };
    g_factory_reset_calls = 0U;
    g_factory_reset_state_read_result = APP_ERROR_NONE;
    g_factory_reset_state = FACTORY_RESET_STATE_NONE;
    g_esp_restart_calls = 0U;

    g_wifi_status = (wifi_ap_status_t){.state = WIFI_AP_READY, .client_count = 1U};
    g_execution_status = (macro_execution_status_t){.state = EXECUTION_IDLE, .available = true};
    g_usb_state = USB_KEYBOARD_READY;
    g_mount_state = (storage_mount_state_t){.web_mounted = true, .data_mounted = true};
    /* webfsTotalBytes/webfsUsedBytes/userdataTotalBytes/userdataUsedBytes/
     * blobCount below are chosen to equal contracts/v2/api/examples.json's
     * "diagnostics" fixture exactly (not just plausible-looking numbers), so
     * test_diagnostics_get_valid() can compare the real response against
     * that checked-in example directly -- see its own comment. */
    g_webfs_capacity_result = APP_ERROR_NONE;
    g_webfs_total_bytes = 1048576U;
    g_webfs_used_bytes = 500000U;
    g_userdata_capacity_result = APP_ERROR_NONE;
    g_userdata_total_bytes = 524288U;
    g_userdata_used_bytes = 4096U;
    g_blob_diagnostics_result = APP_ERROR_NONE;
    g_blob_valid_count = 3U;
    g_device_controls_health = (device_controls_health_t){.task_running = true};
    g_reset_reason = ESP_RST_POWERON;
    g_free_heap_bytes = 200000U;
    g_minimum_free_heap_bytes = 180000U;
    g_largest_free_block_bytes = 120000U;
    executor_health_reset();
    http_health_reset();

    server_configuration = (web_server_config_t){0};
    server_configuration.require_physical_confirmation = false;
}

static void authenticate(fake_httpd_request_t *fake) {
    fake_httpd_add_request_header(fake, "Cookie", "MKSESSION=" ADMIN_TEST_SESSION_TOKEN);
}

static cJSON *parse_response(const fake_httpd_request_t *fake) {
    cJSON *root = cJSON_ParseWithLength(fake->response_body, fake->response_body_length);
    TEST_CHECK(root != NULL);
    return root;
}

static void bind_bodyless(httpd_req_t *request, fake_httpd_request_t *fake, const char *uri,
                          httpd_method_t method) {
    fake_httpd_bind(request, fake, uri, 0U);
    fake_httpd_set_method(request, method);
}

static void bind_json_body(httpd_req_t *request, fake_httpd_request_t *fake, const char *uri,
                           httpd_method_t method, const char *body) {
    fake_httpd_add_request_header(fake, "Content-Type", "application/json");
    const size_t length = strlen(body);
    fake_httpd_set_body(fake, body, length, 0U);
    fake_httpd_bind(request, fake, uri, length);
    fake_httpd_set_method(request, method);
}
