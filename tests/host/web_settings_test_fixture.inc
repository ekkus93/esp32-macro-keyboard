#define TEST_BODY_CAPACITY 1024U

typedef struct {
    app_v2_device_settings_t current;
    app_error_code_t settings_read_error;
    app_error_code_t settings_replace_error;
    app_error_code_t password_verify_error;
    app_error_code_t password_create_error;
    app_error_code_t password_transition_begin_error;
    app_error_code_t invalidate_error;
    bool password_matches;
    bool bad_credential_material;
    unsigned int settings_read_calls;
    unsigned int settings_replace_calls;
    unsigned int password_verify_calls;
    unsigned int password_create_calls;
    unsigned int password_transition_begin_calls;
    unsigned int password_activate_calls;
    unsigned int password_transition_end_calls;
    unsigned int invalidate_calls;
    bool password_transition_active;
    app_v2_device_settings_t committed_candidate;
    app_v2_device_settings_t active_candidate;
    char observed_current_password[256];
    char observed_new_password[256];
} fake_t;

static app_error_code_t fake_settings_read(void *context, app_v2_device_settings_t *out_settings) {
    fake_t *fake = context;
    ++fake->settings_read_calls;
    if (fake->settings_read_error != APP_ERROR_NONE) {
        return fake->settings_read_error;
    }
    *out_settings = fake->current;
    return APP_ERROR_NONE;
}

static app_error_code_t
fake_settings_replace(void *context, const app_v2_device_settings_t *settings, bool *out_changed) {
    fake_t *fake = context;
    ++fake->settings_replace_calls;
    fake->committed_candidate = *settings;
    if (fake->settings_replace_error != APP_ERROR_NONE) {
        *out_changed = false;
        return fake->settings_replace_error;
    }
    *out_changed = true;
    fake->current = *settings;
    return APP_ERROR_NONE;
}

static app_error_code_t fake_password_verify(void *context, const char *password,
                                             size_t password_length,
                                             const app_v2_device_settings_t *settings,
                                             bool *out_matches) {
    fake_t *fake = context;
    (void)settings;
    ++fake->password_verify_calls;
    TEST_CHECK(password_length < sizeof(fake->observed_current_password));
    memcpy(fake->observed_current_password, password, password_length);
    fake->observed_current_password[password_length] = '\0';
    if (fake->password_verify_error != APP_ERROR_NONE) {
        return fake->password_verify_error;
    }
    *out_matches = fake->password_matches;
    return APP_ERROR_NONE;
}

static app_error_code_t fake_password_create(void *context, const char *password,
                                             size_t password_length,
                                             app_v2_setup_password_material_t *out_material) {
    fake_t *fake = context;
    ++fake->password_create_calls;
    TEST_CHECK(password_length < sizeof(fake->observed_new_password));
    memcpy(fake->observed_new_password, password, password_length);
    fake->observed_new_password[password_length] = '\0';
    if (fake->password_create_error != APP_ERROR_NONE) {
        return fake->password_create_error;
    }
    memset(out_material, 0, sizeof(*out_material));
    out_material->credential_version =
        (uint16_t)(fake->bad_credential_material ? APP_V2_CREDENTIAL_VERSION + 1U
                                                 : APP_V2_CREDENTIAL_VERSION);
    out_material->password_algorithm_version = APP_V2_PASSWORD_ALGORITHM_VERSION;
    out_material->password_iterations = 5500U;
    memset(out_material->password_salt, 0x77, sizeof(out_material->password_salt));
    memset(out_material->password_verifier, 0x88, sizeof(out_material->password_verifier));
    return APP_ERROR_NONE;
}

static app_error_code_t fake_password_transition_begin(void *context) {
    fake_t *fake = context;
    ++fake->password_transition_begin_calls;
    if (fake->password_transition_begin_error != APP_ERROR_NONE) {
        return fake->password_transition_begin_error;
    }
    if (fake->password_transition_active) {
        return APP_ERROR_CONFLICT;
    }
    fake->password_transition_active = true;
    return APP_ERROR_NONE;
}

static void fake_password_activate(void *context, const app_v2_device_settings_t *settings) {
    fake_t *fake = context;
    ++fake->password_activate_calls;
    TEST_CHECK(fake->password_transition_active);
    fake->active_candidate = *settings;
}

static void fake_password_transition_end(void *context) {
    fake_t *fake = context;
    ++fake->password_transition_end_calls;
    TEST_CHECK(fake->password_transition_active);
    fake->password_transition_active = false;
}

static app_error_code_t fake_invalidate_all_sessions(void *context) {
    fake_t *fake = context;
    ++fake->invalidate_calls;
    TEST_CHECK(fake->password_transition_active);
    return fake->invalidate_error;
}

static web_settings_ops_t operations(fake_t *fake) {
    return (web_settings_ops_t){
        .context = fake,
        .settings_read = fake_settings_read,
        .settings_replace = fake_settings_replace,
        .password_verify = fake_password_verify,
        .password_create = fake_password_create,
        .password_transition_begin = fake_password_transition_begin,
        .password_activate = fake_password_activate,
        .password_transition_end = fake_password_transition_end,
        .invalidate_all_sessions = fake_invalidate_all_sessions,
    };
}

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

static void build_body(char *buffer, size_t capacity, const char *json) {
    const int written = snprintf(buffer, capacity, "%s", json);
    TEST_CHECK(written > 0 && (size_t)written < capacity);
}

static bool buffer_all_zero(const char *buffer, size_t length) {
    for (size_t index = 0U; index < length; ++index) {
        if (buffer[index] != '\0') {
            return false;
        }
    }
    return true;
}

static bool json_contains(const char *json, const char *needle) {
    return json != NULL && strstr(json, needle) != NULL;
}
