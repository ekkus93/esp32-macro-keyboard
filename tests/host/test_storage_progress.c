#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "storage.h"
#include "storage_repository.h"
#include "storage_repository_lock.h"
#include "test_assert.h"
#include "test_temp_dir.h"

static app_uuid_t make_uuid(uint32_t value) {
    char text[APP_UUID_BUFFER_LENGTH];
    const int written = snprintf(text, sizeof(text), "%08" PRIx32 "-0000-4000-8000-%012" PRIx64,
                                 value, (uint64_t)value);
    TEST_CHECK_EQ_INT((int)APP_UUID_STRING_LENGTH, written);
    app_uuid_t uuid = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_uuid_parse(text, &uuid));
    return uuid;
}

static void make_directory(const char *path) {
    TEST_CHECK(mkdir(path, 0750) == 0 || errno == EEXIST);
}

static bool path_exists(const char *path) {
    struct stat metadata;
    return stat(path, &metadata) == 0;
}

static void reset_store(void) {
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    static const char *const paths[] = {
        STORAGE_DATA_MOUNT,
        STORAGE_DATA_MOUNT "/sets",
        STORAGE_DATA_MOUNT "/staging",
        STORAGE_DATA_MOUNT "/trash",
        STORAGE_DATA_MOUNT "/transactions",
    };
    for (size_t index = 0U; index < (sizeof(paths) / sizeof(paths[0])); ++index) {
        make_directory(paths[index]);
    }
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_init());
}

static macro_set_t make_set(void) {
    macro_set_t set = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = make_uuid(100U),
        .revision = 1U,
    };
    TEST_CHECK(snprintf(set.name, sizeof(set.name), "Progress set") > 0);
    TEST_CHECK(snprintf(set.description, sizeof(set.description), "Progress tests") > 0);
    TEST_CHECK(snprintf(set.manufacturer, sizeof(set.manufacturer), "Test") > 0);
    TEST_CHECK(snprintf(set.model, sizeof(set.model), "Model") > 0);
    TEST_CHECK(snprintf(set.board, sizeof(set.board), "board") > 0);
    TEST_CHECK(snprintf(set.keyboard_layout, sizeof(set.keyboard_layout), "en-US") > 0);
    return set;
}

static procedure_t make_procedure(const macro_set_t *set, uint32_t revision) {
    procedure_t procedure = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = make_uuid(200U),
        .revision = revision,
        .set_id = set->id,
        .step_count = 3U,
    };
    TEST_CHECK(snprintf(procedure.name, sizeof(procedure.name), "Progress procedure") > 0);
    TEST_CHECK(snprintf(procedure.description, sizeof(procedure.description), "Three steps") > 0);
    procedure.steps = calloc(procedure.step_count, sizeof(*procedure.steps));
    TEST_CHECK(procedure.steps != NULL);
    for (size_t index = 0U; index < procedure.step_count; ++index) {
        procedure.steps[index] = (procedure_step_t){
            .id = make_uuid(300U + (uint32_t)index),
            .type = PROCEDURE_STEP_INSTRUCTION,
            .required = true,
        };
        TEST_CHECK(snprintf(procedure.steps[index].title, sizeof(procedure.steps[index].title),
                            "Step %zu", index + 1U) > 0);
        static const char body[] = "Perform the step";
        procedure.steps[index].body = malloc(sizeof(body));
        TEST_CHECK(procedure.steps[index].body != NULL);
        memcpy(procedure.steps[index].body, body, sizeof(body));
        procedure.steps[index].body_length = sizeof(body) - 1U;
    }
    return procedure;
}

static storage_procedure_identity_t identity_for(const macro_set_t *set,
                                                 const procedure_t *procedure) {
    return (storage_procedure_identity_t){
        .set_id = set->id,
        .procedure_id = procedure->id,
    };
}

static void prepare(macro_set_t *out_set, procedure_t *out_procedure,
                    storage_procedure_identity_t *out_identity) {
    *out_set = make_set();
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(out_set));
    *out_procedure = make_procedure(out_set, 1U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_procedure_create(&out_set->id, out_procedure));
    *out_identity = identity_for(out_set, out_procedure);
}

static void test_reset_read_and_update(void) {
    reset_store();
    macro_set_t set;
    procedure_t procedure;
    storage_procedure_identity_t identity;
    prepare(&set, &procedure, &identity);

    storage_progress_snapshot_t snapshot = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, storage_progress_read(&identity, &snapshot));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_progress_reset(&identity, 1U, &snapshot));
    TEST_CHECK_EQ_INT(STORAGE_PROGRESS_STATUS_CURRENT, snapshot.status);
    TEST_CHECK_EQ_U64(1U, snapshot.current_procedure_revision);
    TEST_CHECK_EQ_UUID(&procedure.steps[0].id, &snapshot.progress.current_step_id);
    TEST_CHECK_EQ_U64(0U, snapshot.progress.completed_step_count);

    procedure_progress_t replacement = snapshot.progress;
    replacement.current_step_id = procedure.steps[1].id;
    replacement.completed_step_ids[0] = procedure.steps[0].id;
    replacement.completed_step_count = 1U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_progress_update(&identity, &replacement, &snapshot));
    TEST_CHECK_EQ_UUID(&procedure.steps[1].id, &snapshot.progress.current_step_id);
    TEST_CHECK_EQ_UUID(&procedure.steps[0].id, &snapshot.progress.completed_step_ids[0]);

    storage_progress_snapshot_t readback = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_progress_read(&identity, &readback));
    TEST_CHECK_EQ_INT(STORAGE_PROGRESS_STATUS_CURRENT, readback.status);
    TEST_CHECK_EQ_UUID(&procedure.steps[1].id, &readback.progress.current_step_id);

    macro_model_free_procedure(&procedure);
}

static void test_invalid_step_and_overlap_are_rejected(void) {
    reset_store();
    macro_set_t set;
    procedure_t procedure;
    storage_procedure_identity_t identity;
    prepare(&set, &procedure, &identity);

    storage_progress_snapshot_t snapshot = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_progress_reset(&identity, 1U, &snapshot));
    procedure_progress_t invalid = snapshot.progress;
    invalid.current_step_id = make_uuid(999U);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_progress_update(&identity, &invalid, &snapshot));

    invalid = snapshot.progress;
    invalid.completed_step_ids[0] = procedure.steps[0].id;
    invalid.completed_step_count = 1U;
    invalid.skipped_step_ids[0] = procedure.steps[0].id;
    invalid.skipped_step_count = 1U;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_progress_update(&identity, &invalid, &snapshot));
    macro_model_free_procedure(&procedure);
}

static void test_procedure_revision_change_is_visible_as_stale(void) {
    reset_store();
    macro_set_t set;
    procedure_t procedure;
    storage_procedure_identity_t identity;
    prepare(&set, &procedure, &identity);

    storage_progress_snapshot_t snapshot = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_progress_reset(&identity, 1U, &snapshot));
    procedure_t updated = procedure;
    TEST_CHECK(snprintf(updated.name, sizeof(updated.name), "Updated procedure") > 0);
    procedure_t committed = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_procedure_update(&set.id, &updated, 1U, &committed));
    TEST_CHECK_EQ_U64(2U, committed.revision);
    macro_model_free_procedure(&committed);

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_progress_read(&identity, &snapshot));
    TEST_CHECK_EQ_INT(STORAGE_PROGRESS_STATUS_STALE, snapshot.status);
    TEST_CHECK_EQ_U64(1U, snapshot.progress.procedure_revision);
    TEST_CHECK_EQ_U64(2U, snapshot.current_procedure_revision);
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT,
                         storage_progress_update(&identity, &snapshot.progress, &snapshot));
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, storage_progress_reset(&identity, 1U, &snapshot));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_progress_reset(&identity, 2U, &snapshot));
    TEST_CHECK_EQ_INT(STORAGE_PROGRESS_STATUS_CURRENT, snapshot.status);
    TEST_CHECK_EQ_U64(2U, snapshot.progress.procedure_revision);

    macro_model_free_procedure(&procedure);
}

static void test_corrupt_progress_is_discarded(void) {
    reset_store();
    macro_set_t set;
    procedure_t procedure;
    storage_procedure_identity_t identity;
    prepare(&set, &procedure, &identity);

    storage_progress_snapshot_t snapshot = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_progress_reset(&identity, 1U, &snapshot));
    char path[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE,
        storage_make_progress_path(&identity.set_id, &identity.procedure_id, path, sizeof(path)));
    FILE *file = fopen(path, "wb");
    TEST_CHECK(file != NULL);
    static const char corrupt[] = "{not-json";
    TEST_CHECK_EQ_U64(sizeof(corrupt) - 1U, fwrite(corrupt, 1U, sizeof(corrupt) - 1U, file));
    TEST_CHECK(fclose(file) == 0);

    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, storage_progress_read(&identity, &snapshot));
    TEST_CHECK(!path_exists(path));
    macro_model_free_procedure(&procedure);
}

static void test_procedure_delete_removes_progress(void) {
    reset_store();
    macro_set_t set;
    procedure_t procedure;
    storage_procedure_identity_t identity;
    prepare(&set, &procedure, &identity);

    storage_progress_snapshot_t snapshot = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_progress_reset(&identity, 1U, &snapshot));
    char path[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE,
        storage_make_progress_path(&identity.set_id, &identity.procedure_id, path, sizeof(path)));
    TEST_CHECK(path_exists(path));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_procedure_delete(&set.id, &procedure.id, 1U));
    TEST_CHECK(!path_exists(path));
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, storage_progress_read(&identity, &snapshot));
    macro_model_free_procedure(&procedure);
}

static void test_argument_validation(void) {
    reset_store();
    storage_progress_snapshot_t snapshot = {0};
    storage_procedure_identity_t identity = {0};
    procedure_progress_t progress = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_progress_read(NULL, &snapshot));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_progress_read(&identity, &snapshot));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_progress_read(&identity, NULL));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_progress_update(&identity, &progress, &snapshot));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_progress_reset(&identity, 0U, &snapshot));
}

int main(void) {
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_lock_init());
    test_argument_validation();
    test_reset_read_and_update();
    test_invalid_step_and_overlap_are_rejected();
    test_procedure_revision_change_is_visible_as_stale();
    test_corrupt_progress_is_discarded();
    test_procedure_delete_removes_progress();
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_lock_deinit());
    puts("storage progress repository tests passed");
    return EXIT_SUCCESS;
}
