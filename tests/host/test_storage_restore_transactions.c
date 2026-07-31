#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_limits.h"
#include "storage.h"
#include "storage_fs_ops.h"
#include "storage_transaction_internal.h"
#include "test_assert.h"

#define TRANSACTION_ID "11111111-1111-4111-8111-111111111111"
#define OLD_MARKER "old"
#define NEW_MARKER "new"

typedef struct {
    const char *fail_root;
    app_error_code_t failure;
    size_t calls;
} validation_context_t;

typedef struct {
    size_t next;
} uuid_context_t;

app_error_code_t storage_repository_set_index_presence(const app_uuid_t *set_id,
                                                       bool should_be_present) {
    (void)set_id;
    (void)should_be_present;
    return APP_ERROR_INTERNAL;
}

static app_uuid_t parse_uuid(const char *text) {
    app_uuid_t value = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_uuid_parse(text, &value));
    return value;
}

static void checked_path(char *output, size_t output_size, const char *root, const char *name) {
    const int written = snprintf(output, output_size, "%s/%s", root, name);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < output_size);
}

static bool exists(const char *path) {
    struct stat metadata;
    return stat(path, &metadata) == 0;
}

static void make_directory(const char *path) {
    TEST_CHECK(mkdir(path, 0700) == 0 || errno == EEXIST);
}

static void write_text(const char *path, const char *text) {
    FILE *file = fopen(path, "wb");
    TEST_CHECK(file != NULL);
    const size_t length = strlen(text);
    TEST_CHECK_EQ_U64(length, fwrite(text, 1U, length, file));
    TEST_CHECK_EQ_INT(0, fclose(file));
}

static void remove_tree_checked(const char *path) {
    char command[APP_PATH_MAX_BYTES + 32U];
    const int written = snprintf(command, sizeof(command), "rm -rf -- '%s'", path);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < sizeof(command));
    TEST_CHECK_EQ_INT(0, system(command));
}

static void reset_storage(void) {
    remove_tree_checked(STORAGE_DATA_MOUNT);
    make_directory(STORAGE_DATA_MOUNT);
    char path[APP_PATH_MAX_BYTES];
    static const char *const roots[] = {"transactions", "staging", "trash"};
    for (size_t index = 0U; index < sizeof(roots) / sizeof(roots[0]); ++index) {
        checked_path(path, sizeof(path), STORAGE_DATA_MOUNT, roots[index]);
        make_directory(path);
    }
}

static void create_repository(const char *root, const char *marker) {
    make_directory(root);
    char path[APP_PATH_MAX_BYTES];
    checked_path(path, sizeof(path), root, "set-index.json");
    write_text(path, marker);
    checked_path(path, sizeof(path), root, "sets");
    make_directory(path);
    checked_path(path, sizeof(path), root, "sets/marker.txt");
    write_text(path, marker);
    checked_path(path, sizeof(path), root, "global");
    make_directory(path);
    checked_path(path, sizeof(path), root, "global/marker.txt");
    write_text(path, marker);
}

static bool file_has_text(const char *path, const char *expected) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return false;
    }
    char buffer[16] = {0};
    const size_t count = fread(buffer, 1U, sizeof(buffer) - 1U, file);
    const int close_result = fclose(file);
    return close_result == 0 && count == strlen(expected) && strcmp(buffer, expected) == 0;
}

static bool repository_has_marker(const char *root, const char *marker) {
    char path[APP_PATH_MAX_BYTES];
    checked_path(path, sizeof(path), root, "set-index.json");
    if (!file_has_text(path, marker)) {
        return false;
    }
    checked_path(path, sizeof(path), root, "sets/marker.txt");
    if (!file_has_text(path, marker)) {
        return false;
    }
    checked_path(path, sizeof(path), root, "global/marker.txt");
    return file_has_text(path, marker);
}

static app_error_code_t validate_repository(void *context, const char *root) {
    validation_context_t *validation = context;
    ++validation->calls;
    if (validation->fail_root != NULL && strcmp(root, validation->fail_root) == 0) {
        return validation->failure;
    }
    if (repository_has_marker(root, OLD_MARKER) || repository_has_marker(root, NEW_MARKER)) {
        return APP_ERROR_NONE;
    }
    return APP_ERROR_STORAGE_CORRUPT;
}

static app_error_code_t remove_tree(void *context, const char *path) {
    (void)context;
    remove_tree_checked(path);
    return APP_ERROR_NONE;
}

static app_error_code_t generate_uuid(void *context, app_uuid_t *out_uuid) {
    uuid_context_t *sequence = context;
    ++sequence->next;
    const int written = snprintf(out_uuid->value, sizeof(out_uuid->value),
                                 "ffffffff-ffff-4fff-8fff-%012zu", sequence->next);
    return written == (int)APP_UUID_STRING_LENGTH ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}

static storage_transaction_manifest_t manifest(storage_transaction_phase_t phase) {
    storage_transaction_manifest_t value = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = parse_uuid(TRANSACTION_ID),
        .type = STORAGE_TRANSACTION_RESTORE,
        .phase = phase,
        .expected_revision = 0U,
        .replacement_revision = 0U,
    };
    snprintf(value.source, sizeof(value.source), "%s", STORAGE_DATA_MOUNT);
    snprintf(value.destination, sizeof(value.destination), "%s", STORAGE_DATA_MOUNT);
    snprintf(value.staging, sizeof(value.staging), STORAGE_DATA_MOUNT "/staging/%s",
             value.id.value);
    snprintf(value.backup, sizeof(value.backup), STORAGE_DATA_MOUNT "/trash/restore-%s",
             value.id.value);
    return value;
}

static char *manifest_path(char *path, size_t path_size) {
    const int written = snprintf(path, path_size, STORAGE_DATA_MOUNT "/transactions/%s.bin",
                                 TRANSACTION_ID);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < path_size);
    return path;
}

static void write_manifest(storage_transaction_manifest_t *value, uuid_context_t *uuid_sequence) {
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE,
        storage_transaction_write_manifest_with_ops(value, storage_fs_ops_posix(), generate_uuid,
                                                     uuid_sequence));
}

static void move_item(const char *source_root, const char *destination_root, const char *name) {
    char source[APP_PATH_MAX_BYTES];
    char destination[APP_PATH_MAX_BYTES];
    checked_path(source, sizeof(source), source_root, name);
    checked_path(destination, sizeof(destination), destination_root, name);
    TEST_CHECK_EQ_INT(0, rename(source, destination));
}

static app_error_code_t recover(storage_transaction_manifest_t *value,
                                validation_context_t *validation,
                                uuid_context_t *uuid_sequence) {
    return storage_transaction_recover_restore_with_ops(
        value, storage_fs_ops_posix(), generate_uuid, uuid_sequence, validate_repository,
        validation, remove_tree, NULL);
}

static void assert_complete(const storage_transaction_manifest_t *value) {
    char path[APP_PATH_MAX_BYTES];
    TEST_CHECK(repository_has_marker(STORAGE_DATA_MOUNT, NEW_MARKER));
    TEST_CHECK(!exists(value->staging));
    TEST_CHECK(!exists(value->backup));
    TEST_CHECK(!exists(manifest_path(path, sizeof(path))));
}

static void prepare_staged(storage_transaction_manifest_t *value) {
    create_repository(STORAGE_DATA_MOUNT, OLD_MARKER);
    create_repository(value->staging, NEW_MARKER);
    make_directory(value->backup);
}

static void test_prepared_rolls_back_partial_staging(void) {
    reset_storage();
    storage_transaction_manifest_t value = manifest(STORAGE_TRANSACTION_PREPARED);
    create_repository(STORAGE_DATA_MOUNT, OLD_MARKER);
    make_directory(value.staging);
    char partial[APP_PATH_MAX_BYTES];
    checked_path(partial, sizeof(partial), value.staging, "partial");
    write_text(partial, "partial");
    uuid_context_t uuid_sequence = {0};
    validation_context_t validation = {0};
    write_manifest(&value, &uuid_sequence);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, recover(&value, &validation, &uuid_sequence));
    TEST_CHECK(repository_has_marker(STORAGE_DATA_MOUNT, OLD_MARKER));
    TEST_CHECK(!exists(value.staging));
}

static void test_every_durable_phase_recovers_to_new_repository(void) {
    static const storage_transaction_phase_t phases[] = {
        STORAGE_TRANSACTION_STAGED, STORAGE_TRANSACTION_BACKED_UP,
        STORAGE_TRANSACTION_ACTIVATED, STORAGE_TRANSACTION_INDEXED,
        STORAGE_TRANSACTION_COMPLETE,
    };
    for (size_t index = 0U; index < sizeof(phases) / sizeof(phases[0]); ++index) {
        reset_storage();
        storage_transaction_manifest_t value = manifest(phases[index]);
        if (phases[index] == STORAGE_TRANSACTION_STAGED) {
            prepare_staged(&value);
        } else if (phases[index] == STORAGE_TRANSACTION_BACKED_UP) {
            create_repository(value.backup, OLD_MARKER);
            create_repository(value.staging, NEW_MARKER);
        } else {
            create_repository(STORAGE_DATA_MOUNT, NEW_MARKER);
            create_repository(value.backup, OLD_MARKER);
            make_directory(value.staging);
        }
        uuid_context_t uuid_sequence = {0};
        validation_context_t validation = {0};
        write_manifest(&value, &uuid_sequence);
        TEST_CHECK_APP_ERROR(APP_ERROR_NONE, recover(&value, &validation, &uuid_sequence));
        assert_complete(&value);
        TEST_CHECK_APP_ERROR(APP_ERROR_NONE, recover(&value, &validation, &uuid_sequence));
        assert_complete(&value);
    }
}

static void test_partial_backup_and_activation_are_idempotent(void) {
    reset_storage();
    storage_transaction_manifest_t staged = manifest(STORAGE_TRANSACTION_STAGED);
    prepare_staged(&staged);
    move_item(STORAGE_DATA_MOUNT, staged.backup, "set-index.json");
    uuid_context_t uuid_sequence = {0};
    validation_context_t validation = {0};
    write_manifest(&staged, &uuid_sequence);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, recover(&staged, &validation, &uuid_sequence));
    assert_complete(&staged);

    reset_storage();
    storage_transaction_manifest_t backed = manifest(STORAGE_TRANSACTION_BACKED_UP);
    create_repository(backed.backup, OLD_MARKER);
    create_repository(backed.staging, NEW_MARKER);
    move_item(backed.staging, STORAGE_DATA_MOUNT, "set-index.json");
    uuid_sequence = (uuid_context_t){0};
    validation = (validation_context_t){0};
    write_manifest(&backed, &uuid_sequence);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, recover(&backed, &validation, &uuid_sequence));
    assert_complete(&backed);
}

static void test_duplicate_item_and_validation_failure_preserve_evidence(void) {
    reset_storage();
    storage_transaction_manifest_t duplicate = manifest(STORAGE_TRANSACTION_STAGED);
    prepare_staged(&duplicate);
    char backup_index[APP_PATH_MAX_BYTES];
    checked_path(backup_index, sizeof(backup_index), duplicate.backup, "set-index.json");
    write_text(backup_index, OLD_MARKER);
    uuid_context_t uuid_sequence = {0};
    validation_context_t validation = {0};
    write_manifest(&duplicate, &uuid_sequence);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT,
                         recover(&duplicate, &validation, &uuid_sequence));
    TEST_CHECK(repository_has_marker(STORAGE_DATA_MOUNT, OLD_MARKER));
    TEST_CHECK(exists(duplicate.staging));
    TEST_CHECK(exists(duplicate.backup));

    reset_storage();
    storage_transaction_manifest_t invalid = manifest(STORAGE_TRANSACTION_STAGED);
    prepare_staged(&invalid);
    uuid_sequence = (uuid_context_t){0};
    validation = (validation_context_t){
        .fail_root = invalid.staging,
        .failure = APP_ERROR_STORAGE_CORRUPT,
    };
    write_manifest(&invalid, &uuid_sequence);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT,
                         recover(&invalid, &validation, &uuid_sequence));
    TEST_CHECK(repository_has_marker(STORAGE_DATA_MOUNT, OLD_MARKER));
    TEST_CHECK(exists(invalid.staging));
    TEST_CHECK(exists(invalid.backup));
}

static void test_prepared_never_deletes_existing_backup(void) {
    reset_storage();
    storage_transaction_manifest_t value = manifest(STORAGE_TRANSACTION_PREPARED);
    create_repository(STORAGE_DATA_MOUNT, OLD_MARKER);
    create_repository(value.backup, OLD_MARKER);
    uuid_context_t uuid_sequence = {0};
    validation_context_t validation = {0};
    write_manifest(&value, &uuid_sequence);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT,
                         recover(&value, &validation, &uuid_sequence));
    TEST_CHECK(repository_has_marker(value.backup, OLD_MARKER));
    TEST_CHECK(repository_has_marker(STORAGE_DATA_MOUNT, OLD_MARKER));
}

int main(void) {
    test_prepared_rolls_back_partial_staging();
    test_every_durable_phase_recovers_to_new_repository();
    test_partial_backup_and_activation_are_idempotent();
    test_duplicate_item_and_validation_failure_preserve_evidence();
    test_prepared_never_deletes_existing_backup();
    remove_tree_checked(STORAGE_DATA_MOUNT);
    puts("storage restore transaction tests passed");
    return 0;
}
