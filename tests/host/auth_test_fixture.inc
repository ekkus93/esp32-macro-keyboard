typedef struct {
    bool locked;
    uint64_t now_us;
    uint32_t random_state;
    bool random_fail;
    bool derive_fail;
    bool known_vector_derive;
    size_t derive_count;
    size_t zero_count;
    size_t last_zero_length;
    fake_call_log_t calls;
} auth_fake_t;

static void fake_reset(auth_fake_t *fake)
{
    memset(fake, 0, sizeof(*fake));
    fake->random_state = UINT32_C(0x12345678);
    fake_call_log_reset(&fake->calls);
}

static bool fake_lock(void *context)
{
    auth_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    TEST_CHECK(!fake->locked);
    if (fake_call_log_record(&fake->calls, "lock", 0U, 0U)) {
        return false;
    }
    fake->locked = true;
    return true;
}

static bool fake_unlock(void *context)
{
    auth_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    TEST_CHECK(fake->locked);
    const bool fail = fake_call_log_record(&fake->calls, "unlock", 0U, 0U);
    fake->locked = false;
    return !fail;
}

static bool fake_random(void *context, uint8_t *output, size_t length)
{
    auth_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    TEST_CHECK(output != NULL || length == 0U);
    (void)fake_call_log_record(&fake->calls, "random", length, fake->random_state);
    if (fake->random_fail) {
        return false;
    }
    for (size_t index = 0U; index < length; ++index) {
        fake->random_state = fake->random_state * UINT32_C(1664525) +
                             UINT32_C(1013904223);
        output[index] = (uint8_t)(fake->random_state >> 24U);
    }
    return true;
}

static uint64_t fake_now(void *context)
{
    auth_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    (void)fake_call_log_record(&fake->calls, "now", fake->now_us, 0U);
    return fake->now_us;
}

static int fake_derive(void *context,
                       const char *password,
                       size_t password_length,
                       const uint8_t *salt,
                       uint32_t iterations,
                       uint8_t *output)
{
    auth_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    TEST_CHECK(password != NULL);
    TEST_CHECK(password_length > 0U);
    TEST_CHECK(salt != NULL);
    TEST_CHECK(output != NULL);
    ++fake->derive_count;
    (void)fake_call_log_record(&fake->calls, "derive", password_length, iterations);
    if (fake->derive_fail) {
        return -1;
    }
    if (fake->known_vector_derive) {
        static const char vector_password[] = "correct horse battery staple";
        static const uint8_t vector_salt[AUTH_SALT_BYTES] = {
            0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U,
            0x08U, 0x09U, 0x0aU, 0x0bU, 0x0cU, 0x0dU, 0x0eU, 0x0fU,
        };
        static const uint8_t vector_hash[AUTH_HASH_BYTES] = {
            0xe9U, 0x62U, 0xebU, 0xd8U, 0x26U, 0x7bU, 0xc8U, 0x39U,
            0x38U, 0x6dU, 0x46U, 0x08U, 0xbbU, 0xc3U, 0xc8U, 0xacU,
            0x36U, 0xbfU, 0xb2U, 0x15U, 0xfaU, 0xa8U, 0x54U, 0x4eU,
            0x3aU, 0x4eU, 0x2cU, 0xbcU, 0xceU, 0xc8U, 0x48U, 0x06U,
        };
        if (password_length != sizeof(vector_password) - 1U ||
            memcmp(password, vector_password, sizeof(vector_password) - 1U) != 0 ||
            memcmp(salt, vector_salt, sizeof(vector_salt)) != 0 || iterations != 120000U) {
            return -1;
        }
        memcpy(output, vector_hash, sizeof(vector_hash));
        return 0;
    }
    for (size_t index = 0U; index < AUTH_HASH_BYTES; ++index) {
        const uint8_t iteration_byte =
            (uint8_t)(iterations >> (uint32_t)((index % 4U) * 8U));
        output[index] = (uint8_t)((uint8_t)password[index % password_length] ^
                                  salt[index % AUTH_SALT_BYTES] ^ iteration_byte);
    }
    return 0;
}

static void fake_zero(void *context, void *memory, size_t length)
{
    auth_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    TEST_CHECK(memory != NULL || length == 0U);
    volatile uint8_t *bytes = memory;
    for (size_t index = 0U; index < length; ++index) {
        bytes[index] = 0U;
    }
    ++fake->zero_count;
    fake->last_zero_length = length;
    (void)fake_call_log_record(&fake->calls, "zero", length, 0U);
}

static void init_core(auth_core_t *core, auth_fake_t *fake)
{
    const auth_ops_t ops = {
        .context = fake,
        .lock = fake_lock,
        .unlock = fake_unlock,
        .random_fill = fake_random,
        .now_us = fake_now,
        .derive = fake_derive,
        .secure_zero = fake_zero,
    };
    TEST_CHECK(auth_core_init(core, &ops) == APP_ERROR_NONE);
}

