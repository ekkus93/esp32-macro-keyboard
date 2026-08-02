#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "provisioning_core.h"
#include "test_assert.h"

#define RECORD_REVISION_OFFSET 8U
#define RECORD_CREDENTIAL_VERSION_OFFSET 12U
#define RECORD_FLAGS_OFFSET 16U
#define RECORD_SSID_OFFSET 20U

typedef struct {
    uint8_t durable[PROVISIONING_RECORD_BYTES];
    size_t durable_size;
    bool present;
    uint8_t staged[PROVISIONING_RECORD_BYTES];
    size_t staged_size;
    bool staged_present;
    bool erase_pending;
    bool ignore_erase;
    bool corrupt_after_commit;
    app_error_code_t read_error;
    unsigned int read_error_occurrence;
    app_error_code_t write_error;
    app_error_code_t erase_error;
    app_error_code_t commit_error;
    unsigned int read_calls;
    unsigned int write_calls;
    unsigned int erase_calls;
    unsigned int commit_calls;
    unsigned int secure_zero_calls;
    size_t secure_zero_bytes;
} fake_provisioning_t;

static app_error_code_t fake_read_blob(void *context, uint8_t *output, size_t capacity,
                                       size_t *out_size) {
    fake_provisioning_t *fake = context;
    ++fake->read_calls;
    if (fake->read_error != APP_ERROR_NONE &&
        (fake->read_error_occurrence == 0U || fake->read_calls == fake->read_error_occurrence)) {
        return fake->read_error;
    }
    if (!fake->present) {
        return APP_ERROR_NOT_FOUND;
    }
    if (output == NULL || out_size == NULL || fake->durable_size > capacity) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    memcpy(output, fake->durable, fake->durable_size);
    *out_size = fake->durable_size;
    return APP_ERROR_NONE;
}

static app_error_code_t fake_write_blob(void *context, const uint8_t *data, size_t size) {
    fake_provisioning_t *fake = context;
    ++fake->write_calls;
    if (fake->write_error != APP_ERROR_NONE) {
        return fake->write_error;
    }
    if (data == NULL || size != sizeof(fake->staged)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memcpy(fake->staged, data, size);
    fake->staged_size = size;
    fake->staged_present = true;
    fake->erase_pending = false;
    return APP_ERROR_NONE;
}

static app_error_code_t fake_erase_blob(void *context) {
    fake_provisioning_t *fake = context;
    ++fake->erase_calls;
    if (fake->erase_error != APP_ERROR_NONE) {
        return fake->erase_error;
    }
    fake->erase_pending = true;
    fake->staged_present = false;
    return APP_ERROR_NONE;
}

static app_error_code_t fake_commit(void *context) {
    fake_provisioning_t *fake = context;
    ++fake->commit_calls;
    if (fake->commit_error != APP_ERROR_NONE) {
        return fake->commit_error;
    }
    if (fake->erase_pending && !fake->ignore_erase) {
        memset(fake->durable, 0, sizeof(fake->durable));
        fake->durable_size = 0U;
        fake->present = false;
    } else if (fake->staged_present) {
        memcpy(fake->durable, fake->staged, fake->staged_size);
        fake->durable_size = fake->staged_size;
        fake->present = true;
    }
    fake->erase_pending = false;
    fake->staged_present = false;
    if (fake->corrupt_after_commit && fake->present) {
        fake->durable[RECORD_SSID_OFFSET] ^= 1U;
    }
    return APP_ERROR_NONE;
}

static void fake_secure_zero(void *context, void *memory, size_t size) {
    fake_provisioning_t *fake = context;
    memset(memory, 0, size);
    ++fake->secure_zero_calls;
    fake->secure_zero_bytes += size;
}

static provisioning_ops_t fake_operations(fake_provisioning_t *fake) {
    return (provisioning_ops_t){
        .context = fake,
        .read_blob = fake_read_blob,
        .write_blob = fake_write_blob,
        .erase_blob = fake_erase_blob,
        .commit = fake_commit,
        .secure_zero = fake_secure_zero,
    };
}

static void fake_init(fake_provisioning_t *fake) {
    memset(fake, 0, sizeof(*fake));
}

static provisioning_config_t valid_configuration(uint32_t revision) {
    provisioning_config_t configuration = {
        .schema_version = APP_SCHEMA_VERSION,
        .revision = revision,
        .credential_version = PROVISIONING_CREDENTIAL_VERSION_INITIAL,
        .provisioned = true,
        .require_physical_confirmation = true,
        .always_select_set = false,
        .has_active_set = true,
    };
    TEST_CHECK_EQ_INT(
        14, snprintf(configuration.ap_ssid, sizeof(configuration.ap_ssid), "%s", "Macro Keyboard"));
    TEST_CHECK_EQ_INT(21, snprintf(configuration.ap_passphrase, sizeof(configuration.ap_passphrase),
                                   "%s", "correct-horse-battery"));
    memset(configuration.password_record.salt, 0x11, sizeof(configuration.password_record.salt));
    memset(configuration.password_record.hash, 0x22, sizeof(configuration.password_record.hash));
    configuration.password_record.iterations = AUTH_PBKDF2_ITERATIONS;
    TEST_CHECK_EQ_INT(36, snprintf(configuration.active_set_id.value,
                                   sizeof(configuration.active_set_id.value), "%s",
                                   "12345678-1234-4234-9234-123456789abc"));
    return configuration;
}

static void start_empty(provisioning_core_t *core, fake_provisioning_t *fake) {
    const provisioning_ops_t operations = fake_operations(fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_init(core, &operations));
    provisioning_config_t loaded;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_load(core, &loaded));
    TEST_CHECK(!loaded.provisioned);
    TEST_CHECK_EQ_U64(0U, loaded.revision);
}

static void commit_valid(provisioning_core_t *core, fake_provisioning_t *fake,
                         provisioning_config_t *out_committed) {
    start_empty(core, fake);
    const provisioning_config_t replacement = valid_configuration(0U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         provisioning_core_commit(core, &replacement, 0U, out_committed));
}

static void set_u32(uint8_t *bytes, size_t offset, uint32_t value) {
    bytes[offset] = (uint8_t)(value & 0xffU);
    bytes[offset + 1U] = (uint8_t)((value >> 8U) & 0xffU);
    bytes[offset + 2U] = (uint8_t)((value >> 16U) & 0xffU);
    bytes[offset + 3U] = (uint8_t)((value >> 24U) & 0xffU);
}

static void invalidate_operation(provisioning_ops_t *operations, size_t index) {
    switch (index) {
    case 0U:
        operations->read_blob = NULL;
        break;
    case 1U:
        operations->write_blob = NULL;
        break;
    case 2U:
        operations->erase_blob = NULL;
        break;
    case 3U:
        operations->commit = NULL;
        break;
    case 4U:
        operations->secure_zero = NULL;
        break;
    default:
        break;
    }
}

static void test_init_and_default_load(void) {
    fake_provisioning_t fake;
    fake_init(&fake);
    provisioning_core_t core = {0};
    provisioning_ops_t operations = fake_operations(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, provisioning_core_init(NULL, &operations));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, provisioning_core_init(&core, NULL));
    for (size_t index = 0U; index < 5U; ++index) {
        operations = fake_operations(&fake);
        invalidate_operation(&operations, index);
        TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                             provisioning_core_init(&core, &operations));
    }

    operations = fake_operations(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_init(&core, &operations));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, provisioning_core_load(NULL, NULL));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, provisioning_core_load(&core, NULL));
    provisioning_config_t loaded;
    memset(&loaded, 0xff, sizeof(loaded));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_load(&core, &loaded));
    TEST_CHECK_EQ_U64(APP_SCHEMA_VERSION, loaded.schema_version);
    TEST_CHECK_EQ_U64(0U, loaded.revision);
    TEST_CHECK(!loaded.provisioned);
    /* Off by default: the device must be usable with no button on it. */
    TEST_CHECK(!loaded.require_physical_confirmation);
    TEST_CHECK(loaded.always_select_set);
    TEST_CHECK(!loaded.has_active_set);
    TEST_CHECK(fake.secure_zero_calls > 0U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_deinit(&core));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_deinit(&core));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, provisioning_core_deinit(NULL));
}

static void test_configuration_validation(void) {
    provisioning_config_t configuration = valid_configuration(0U);
    TEST_CHECK(provisioning_config_is_valid(&configuration));
    TEST_CHECK(!provisioning_config_is_valid(NULL));

    configuration.schema_version = 99U;
    TEST_CHECK(!provisioning_config_is_valid(&configuration));
    configuration = valid_configuration(0U);
    configuration.ap_ssid[0] = '\0';
    TEST_CHECK(!provisioning_config_is_valid(&configuration));
    configuration = valid_configuration(0U);
    configuration.ap_passphrase[11] = '\0';
    TEST_CHECK(!provisioning_config_is_valid(&configuration));
    configuration = valid_configuration(0U);
    memset(configuration.ap_ssid, 'x', sizeof(configuration.ap_ssid));
    TEST_CHECK(!provisioning_config_is_valid(&configuration));
    configuration = valid_configuration(0U);
    memset(configuration.ap_passphrase, 'x', sizeof(configuration.ap_passphrase));
    TEST_CHECK(!provisioning_config_is_valid(&configuration));
    configuration = valid_configuration(0U);
    configuration.password_record.iterations = AUTH_PBKDF2_ITERATIONS + 1U;
    TEST_CHECK(!provisioning_config_is_valid(&configuration));
    configuration = valid_configuration(0U);
    memset(configuration.password_record.salt, 0, sizeof(configuration.password_record.salt));
    TEST_CHECK(!provisioning_config_is_valid(&configuration));
    configuration = valid_configuration(0U);
    memset(configuration.password_record.hash, 0, sizeof(configuration.password_record.hash));
    TEST_CHECK(!provisioning_config_is_valid(&configuration));
    configuration = valid_configuration(0U);
    configuration.credential_version = 0U;
    TEST_CHECK(!provisioning_config_is_valid(&configuration));
    configuration = valid_configuration(0U);
    configuration.active_set_id.value[14] = 'g';
    TEST_CHECK(!provisioning_config_is_valid(&configuration));
    configuration = valid_configuration(0U);
    configuration.has_active_set = false;
    TEST_CHECK(!provisioning_config_is_valid(&configuration));
    memset(&configuration.active_set_id, 0, sizeof(configuration.active_set_id));
    TEST_CHECK(provisioning_config_is_valid(&configuration));

    provisioning_config_t unprovisioned = {
        .schema_version = APP_SCHEMA_VERSION,
        .require_physical_confirmation = true,
        .always_select_set = true,
    };
    TEST_CHECK(provisioning_config_is_valid(&unprovisioned));
    unprovisioned.ap_ssid[0] = 'x';
    TEST_CHECK(!provisioning_config_is_valid(&unprovisioned));
}

static void test_commit_and_revision_conflicts(void) {
    fake_provisioning_t fake;
    fake_init(&fake);
    provisioning_core_t core;
    const provisioning_ops_t operations = fake_operations(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_init(&core, &operations));
    provisioning_config_t replacement = valid_configuration(0U);
    provisioning_config_t committed;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         provisioning_core_commit(&core, &replacement, 0U, &committed));
    provisioning_config_t loaded;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_load(&core, &loaded));

    replacement.schema_version = 99U;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         provisioning_core_commit(&core, &replacement, 0U, &committed));
    replacement = valid_configuration(1U);
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT,
                         provisioning_core_commit(&core, &replacement, 0U, &committed));
    replacement = valid_configuration(0U);
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT,
                         provisioning_core_commit(&core, &replacement, 1U, &committed));
    TEST_CHECK_EQ_U64(0U, fake.write_calls);

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         provisioning_core_commit(&core, &replacement, 0U, &committed));
    TEST_CHECK_EQ_U64(1U, committed.revision);
    TEST_CHECK_EQ_U64(1U, fake.write_calls);
    TEST_CHECK_EQ_U64(1U, fake.commit_calls);
    TEST_CHECK(fake.present);
    TEST_CHECK(fake.secure_zero_calls >= 4U);

    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT,
                         provisioning_core_commit(&core, &replacement, 0U, &committed));
    TEST_CHECK_EQ_U64(0U, committed.revision);
}

static void test_storage_failures_and_readback(void) {
    fake_provisioning_t fake;
    provisioning_core_t core;
    provisioning_config_t replacement = valid_configuration(0U);
    provisioning_config_t committed;

    fake_init(&fake);
    start_empty(&core, &fake);
    fake.write_error = APP_ERROR_STORAGE_FULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_FULL,
                         provisioning_core_commit(&core, &replacement, 0U, &committed));
    TEST_CHECK_EQ_U64(0U, fake.commit_calls);
    TEST_CHECK(!fake.present);

    fake_init(&fake);
    start_empty(&core, &fake);
    fake.commit_error = APP_ERROR_IO;
    TEST_CHECK_APP_ERROR(APP_ERROR_IO,
                         provisioning_core_commit(&core, &replacement, 0U, &committed));
    TEST_CHECK(!fake.present);

    fake_init(&fake);
    start_empty(&core, &fake);
    fake.read_error = APP_ERROR_IO;
    fake.read_error_occurrence = 2U;
    TEST_CHECK_APP_ERROR(APP_ERROR_IO,
                         provisioning_core_commit(&core, &replacement, 0U, &committed));
    TEST_CHECK(fake.present);
    fake.read_error = APP_ERROR_NONE;
    provisioning_config_t loaded;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_load(&core, &loaded));
    TEST_CHECK_EQ_U64(1U, loaded.revision);

    fake_init(&fake);
    start_empty(&core, &fake);
    fake.corrupt_after_commit = true;
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT,
                         provisioning_core_commit(&core, &replacement, 0U, &committed));
    TEST_CHECK(fake.present);
}

static void test_corrupt_persisted_records(void) {
    fake_provisioning_t fake;
    provisioning_core_t core;
    provisioning_config_t committed;
    fake_init(&fake);
    commit_valid(&core, &fake, &committed);

    provisioning_ops_t operations = fake_operations(&fake);
    provisioning_core_t reader;
    provisioning_config_t loaded;

    fake.durable_size -= 1U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_init(&reader, &operations));
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, provisioning_core_load(&reader, &loaded));
    fake.durable_size += 1U;

    fake.durable[0] = 'X';
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_init(&reader, &operations));
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, provisioning_core_load(&reader, &loaded));
    fake.durable[0] = 'P';

    fake.durable[RECORD_FLAGS_OFFSET] = 2U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_init(&reader, &operations));
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, provisioning_core_load(&reader, &loaded));
    fake.durable[RECORD_FLAGS_OFFSET] = 1U;

    fake.durable[RECORD_SSID_OFFSET + strlen("Macro Keyboard") + 1U] = 0x7fU;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_init(&reader, &operations));
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, provisioning_core_load(&reader, &loaded));
}

static void test_clear_credentials(void) {
    fake_provisioning_t fake;
    fake_init(&fake);
    provisioning_core_t core;
    provisioning_config_t committed;
    commit_valid(&core, &fake, &committed);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_clear_credentials(&core));
    provisioning_config_t loaded;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_load(&core, &loaded));
    TEST_CHECK(!loaded.provisioned);
    TEST_CHECK_EQ_U64(2U, loaded.revision);
    TEST_CHECK_EQ_U64(2U, loaded.credential_version);
    TEST_CHECK_EQ_STRING("", loaded.ap_ssid);
    TEST_CHECK_EQ_STRING("", loaded.ap_passphrase);
    TEST_CHECK(loaded.has_active_set);

    set_u32(fake.durable, RECORD_CREDENTIAL_VERSION_OFFSET, UINT32_MAX);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_load(&core, &loaded));
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, provisioning_core_clear_credentials(&core));
}

static void test_revision_exhaustion(void) {
    fake_provisioning_t fake;
    fake_init(&fake);
    provisioning_core_t core;
    provisioning_config_t committed;
    commit_valid(&core, &fake, &committed);
    set_u32(fake.durable, RECORD_REVISION_OFFSET, UINT32_MAX);
    provisioning_config_t loaded;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_load(&core, &loaded));
    provisioning_config_t replacement = loaded;
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT,
                         provisioning_core_commit(&core, &replacement, UINT32_MAX, &committed));
}

static void test_factory_reset(void) {
    fake_provisioning_t fake;
    provisioning_core_t core;
    provisioning_config_t committed;

    fake_init(&fake);
    commit_valid(&core, &fake, &committed);
    fake.erase_error = APP_ERROR_IO;
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, provisioning_core_factory_reset(&core));
    TEST_CHECK(fake.present);

    fake.erase_error = APP_ERROR_NONE;
    fake.commit_error = APP_ERROR_INTERNAL;
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, provisioning_core_factory_reset(&core));
    TEST_CHECK(fake.present);

    fake.commit_error = APP_ERROR_NONE;
    fake.ignore_erase = true;
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, provisioning_core_factory_reset(&core));
    TEST_CHECK(fake.present);

    fake.ignore_erase = false;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_factory_reset(&core));
    TEST_CHECK(!fake.present);
    provisioning_config_t loaded;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_load(&core, &loaded));
    TEST_CHECK_EQ_U64(0U, loaded.revision);
    TEST_CHECK(!loaded.provisioned);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_factory_reset(&core));
}

static void test_load_error_and_uninitialized_calls(void) {
    fake_provisioning_t fake;
    fake_init(&fake);
    provisioning_core_t core = {0};
    provisioning_config_t configuration;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, provisioning_core_load(&core, &configuration));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, provisioning_core_clear_credentials(&core));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, provisioning_core_factory_reset(&core));

    const provisioning_ops_t operations = fake_operations(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_init(&core, &operations));
    fake.read_error = APP_ERROR_STORAGE_UNAVAILABLE;
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE,
                         provisioning_core_load(&core, &configuration));
    TEST_CHECK_EQ_U64(0U, configuration.schema_version);
}

int main(void) {
    test_init_and_default_load();
    test_configuration_validation();
    test_commit_and_revision_conflicts();
    test_storage_failures_and_readback();
    test_corrupt_persisted_records();
    test_clear_credentials();
    test_revision_exhaustion();
    test_factory_reset();
    test_load_error_and_uninitialized_calls();
    puts("provisioning tests passed");
    return EXIT_SUCCESS;
}
