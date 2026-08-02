#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "app_uuid.h"
#include "macro_limits.h"
#include "macro_model.h"
#include "storage.h"
#include "storage_atomic_validators.h"
#include "storage_fs_ops.h"
#include "storage_repository_internal.h"
#include "test_assert.h"
#include "test_temp_dir.h"

#define SET_UUID "0a1b2c3d-0000-4000-8000-000000000001"
#define OTHER_UUID "0a1b2c3d-0000-4000-8000-000000000002"
#define THIRD_UUID "0a1b2c3d-0000-4000-8000-000000000003"
#define OP_UUID "0a1b2c3d-0000-4000-8000-0000000000ff"

static void make_directory(const char *path) {
    TEST_CHECK(mkdir(path, 0750) == 0 || errno == EEXIST);
}

static void write_file(const char *path, const void *data, size_t length) {
    const int descriptor = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0640);
    TEST_CHECK(descriptor >= 0);
    size_t written = 0U;
    const uint8_t *bytes = data;
    while (written < length) {
        const ssize_t count = write(descriptor, bytes + written, length - written);
        TEST_CHECK(count > 0);
        written += (size_t)count;
    }
    TEST_CHECK(close(descriptor) == 0);
}

static void write_text(const char *path, const char *text) {
    write_file(path, text, strlen(text));
}

static app_uuid_t parse_uuid(const char *text) {
    app_uuid_t uuid = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_uuid_parse(text, &uuid));
    return uuid;
}

static void reset_store(void) {
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    make_directory(STORAGE_DATA_MOUNT);
    make_directory(STORAGE_DATA_MOUNT "/sets");
    make_directory(STORAGE_DATA_MOUNT "/sets/" SET_UUID);
    make_directory(STORAGE_DATA_MOUNT "/sets/" SET_UUID "/macros");
    make_directory(STORAGE_DATA_MOUNT "/sets/" SET_UUID "/procedures");
    make_directory(STORAGE_DATA_MOUNT "/sets/" SET_UUID "/progress");
    make_directory(STORAGE_DATA_MOUNT "/transactions");
}

static macro_set_t make_set(const char *id_text) {
    macro_set_t set = {0};
    set.schema_version = APP_SCHEMA_VERSION;
    set.id = parse_uuid(id_text);
    set.revision = 1U;
    TEST_CHECK(snprintf(set.name, sizeof(set.name), "Validator Set") > 0);
    return set;
}

static void write_set_json(const char *path, const char *id_text) {
    const macro_set_t set = make_set(id_text);
    char *json = NULL;
    size_t length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_serialize_set_json(&set, &json, &length));
    TEST_CHECK(json != NULL);
    write_file(path, json, length);
    free(json);
}

static void write_macro_json(const char *path, const char *id_text, const char *set_id_text) {
    static char source[] = "a";
    macro_t macro = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = parse_uuid(id_text),
        .revision = 1U,
        .set_id = parse_uuid(set_id_text),
        .source = source,
        .source_length = strlen(source),
        .key_press_ms = APP_KEY_PRESS_DEFAULT_MS,
        .inter_key_ms = APP_INTER_KEY_DEFAULT_MS,
    };
    TEST_CHECK(snprintf(macro.name, sizeof(macro.name), "Validator Macro") > 0);
    char *json = NULL;
    size_t length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_serialize_macro_json(&macro, &json, &length));
    TEST_CHECK(json != NULL);
    write_file(path, json, length);
    free(json);
}

static void write_procedure_json(const char *path, const char *id_text, const char *set_id_text) {
    procedure_step_t steps[1] = {{
        .id = parse_uuid(OP_UUID),
        .type = PROCEDURE_STEP_MACRO,
        .required = true,
        .has_macro_id = true,
        .macro_id = parse_uuid(OP_UUID),
    }};
    TEST_CHECK(snprintf(steps[0].title, sizeof(steps[0].title), "Step") > 0);
    procedure_t procedure = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = parse_uuid(id_text),
        .revision = 1U,
        .set_id = parse_uuid(set_id_text),
        .steps = steps,
        .step_count = 1U,
    };
    TEST_CHECK(snprintf(procedure.name, sizeof(procedure.name), "Validator Procedure") > 0);
    char *json = NULL;
    size_t length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_serialize_procedure_json(&procedure, &json, &length));
    TEST_CHECK(json != NULL);
    write_file(path, json, length);
    free(json);
}

static void write_progress_json(const char *path, const char *procedure_id_text,
                                const char *set_id_text) {
    procedure_progress_t progress = {
        .schema_version = APP_SCHEMA_VERSION,
        .set_id = parse_uuid(set_id_text),
        .procedure_id = parse_uuid(procedure_id_text),
        .procedure_revision = 1U,
        .current_step_id = parse_uuid(OP_UUID),
    };
    char *json = NULL;
    size_t length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_serialize_progress_json(&progress, &json, &length));
    TEST_CHECK(json != NULL);
    write_file(path, json, length);
    free(json);
}

static void write_manifest(const char *path, const char *id_text) {
    storage_transaction_manifest_t manifest = {0};
    manifest.schema_version = APP_SCHEMA_VERSION;
    manifest.id = parse_uuid(id_text);
    manifest.type = STORAGE_TRANSACTION_IMPORT_SET;
    manifest.phase = STORAGE_TRANSACTION_PREPARED;
    manifest.expected_revision = 0U;
    manifest.replacement_revision = 1U;
    write_file(path, &manifest, sizeof(manifest));
}

static app_error_code_t validate(const char *destination, const char *candidate) {
    return storage_atomic_validate_candidate(storage_fs_ops_posix(), destination, candidate);
}

static void test_classifier(void) {
    TEST_CHECK_EQ_INT(STORAGE_ATOMIC_OBJECT_SCHEMA_MARKER,
                      storage_atomic_classify_destination(STORAGE_DATA_MOUNT "/schema.json"));
    TEST_CHECK_EQ_INT(STORAGE_ATOMIC_OBJECT_SET_INDEX,
                      storage_atomic_classify_destination(STORAGE_DATA_MOUNT "/set-index.json"));
    /* The global macro store was removed (SPEC §7.2); its order file is no
     * longer a recognized atomic-write object. */
    TEST_CHECK_EQ_INT(
        STORAGE_ATOMIC_OBJECT_UNKNOWN,
        storage_atomic_classify_destination(STORAGE_DATA_MOUNT "/global/macro-order.json"));
    TEST_CHECK_EQ_INT(
        STORAGE_ATOMIC_OBJECT_TRANSACTION_MANIFEST,
        storage_atomic_classify_destination(STORAGE_DATA_MOUNT "/transactions/" SET_UUID ".bin"));
    /* Quarantine was removed (SPEC §13.6); a /data/quarantine path is not a
     * recognized atomic-write object. */
    TEST_CHECK_EQ_INT(
        STORAGE_ATOMIC_OBJECT_UNKNOWN,
        storage_atomic_classify_destination(STORAGE_DATA_MOUNT "/quarantine/" SET_UUID ".json"));
    TEST_CHECK_EQ_INT(
        STORAGE_ATOMIC_OBJECT_SET_METADATA,
        storage_atomic_classify_destination(STORAGE_DATA_MOUNT "/sets/" SET_UUID "/set.json"));
    TEST_CHECK_EQ_INT(STORAGE_ATOMIC_OBJECT_MACRO,
                      storage_atomic_classify_destination(
                          STORAGE_DATA_MOUNT "/sets/" SET_UUID "/macros/" OTHER_UUID ".json"));
    TEST_CHECK_EQ_INT(STORAGE_ATOMIC_OBJECT_PROCEDURE,
                      storage_atomic_classify_destination(
                          STORAGE_DATA_MOUNT "/sets/" SET_UUID "/procedures/" OTHER_UUID ".json"));
    TEST_CHECK_EQ_INT(STORAGE_ATOMIC_OBJECT_PROGRESS,
                      storage_atomic_classify_destination(
                          STORAGE_DATA_MOUNT "/sets/" SET_UUID "/progress/" OTHER_UUID ".json"));
    TEST_CHECK_EQ_INT(STORAGE_ATOMIC_OBJECT_UNKNOWN,
                      storage_atomic_classify_destination(STORAGE_DATA_MOUNT "/mystery.dat"));
    TEST_CHECK_EQ_INT(STORAGE_ATOMIC_OBJECT_UNKNOWN, storage_atomic_classify_destination(NULL));
}

static void test_schema_marker_validator(void) {
    reset_store();
    const char *destination = STORAGE_DATA_MOUNT "/schema.json";
    const char *candidate = STORAGE_DATA_MOUNT "/schema.json.tmp." OP_UUID;
    write_text(candidate, "{\"schema_version\":1}");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, validate(destination, candidate));

    write_text(candidate, "{\"schema_version\":2}");
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, validate(destination, candidate));
    write_text(candidate, "not json");
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, validate(destination, candidate));
}

static void test_index_validators(void) {
    reset_store();
    const char *set_dest = STORAGE_DATA_MOUNT "/set-index.json";
    const char *set_candidate = STORAGE_DATA_MOUNT "/set-index.json.tmp." OP_UUID;
    write_text(set_candidate, "{\"schema_version\":1,\"ids\":[]}");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, validate(set_dest, set_candidate));
    write_text(set_candidate, "{\"schema_version\":9,\"ids\":[]}");
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, validate(set_dest, set_candidate));
}

static void test_set_metadata_validator(void) {
    reset_store();
    const char *destination = STORAGE_DATA_MOUNT "/sets/" SET_UUID "/set.json";
    const char *candidate = STORAGE_DATA_MOUNT "/sets/" SET_UUID "/set.json.tmp." OP_UUID;

    /* A candidate whose id matches the destination's set is valid. */
    write_set_json(candidate, SET_UUID);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, validate(destination, candidate));

    /* A candidate for a different set id is rejected even though it parses. */
    write_set_json(candidate, OTHER_UUID);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, validate(destination, candidate));

    write_text(candidate, "{\"not\":\"a set\"}");
    TEST_CHECK(validate(destination, candidate) != APP_ERROR_NONE);
}

static void test_macro_validator(void) {
    reset_store();
    const char *destination = STORAGE_DATA_MOUNT "/sets/" SET_UUID "/macros/" OTHER_UUID ".json";
    const char *candidate =
        STORAGE_DATA_MOUNT "/sets/" SET_UUID "/macros/" OTHER_UUID ".json.tmp." OP_UUID;

    /* A candidate whose id and set both match the destination is valid. */
    write_macro_json(candidate, OTHER_UUID, SET_UUID);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, validate(destination, candidate));

    /* A candidate with the destination's id but a different owning set is
     * rejected even though it parses. */
    write_macro_json(candidate, OTHER_UUID, THIRD_UUID);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, validate(destination, candidate));

    /* A candidate for a different macro id entirely is rejected. */
    write_macro_json(candidate, THIRD_UUID, SET_UUID);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, validate(destination, candidate));

    write_text(candidate, "{\"not\":\"a macro\"}");
    TEST_CHECK(validate(destination, candidate) != APP_ERROR_NONE);
}

static void test_procedure_validator(void) {
    reset_store();
    const char *destination =
        STORAGE_DATA_MOUNT "/sets/" SET_UUID "/procedures/" OTHER_UUID ".json";
    const char *candidate =
        STORAGE_DATA_MOUNT "/sets/" SET_UUID "/procedures/" OTHER_UUID ".json.tmp." OP_UUID;

    write_procedure_json(candidate, OTHER_UUID, SET_UUID);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, validate(destination, candidate));

    write_procedure_json(candidate, OTHER_UUID, THIRD_UUID);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, validate(destination, candidate));

    write_procedure_json(candidate, THIRD_UUID, SET_UUID);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, validate(destination, candidate));

    write_text(candidate, "{\"not\":\"a procedure\"}");
    TEST_CHECK(validate(destination, candidate) != APP_ERROR_NONE);
}

static void test_progress_validator(void) {
    reset_store();
    const char *destination = STORAGE_DATA_MOUNT "/sets/" SET_UUID "/progress/" OTHER_UUID ".json";
    const char *candidate =
        STORAGE_DATA_MOUNT "/sets/" SET_UUID "/progress/" OTHER_UUID ".json.tmp." OP_UUID;

    write_progress_json(candidate, OTHER_UUID, SET_UUID);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, validate(destination, candidate));

    write_progress_json(candidate, OTHER_UUID, THIRD_UUID);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, validate(destination, candidate));

    write_progress_json(candidate, THIRD_UUID, SET_UUID);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, validate(destination, candidate));

    write_text(candidate, "{\"not\":\"progress\"}");
    TEST_CHECK(validate(destination, candidate) != APP_ERROR_NONE);
}

static void test_transaction_manifest_validator(void) {
    reset_store();
    const char *destination = STORAGE_DATA_MOUNT "/transactions/" SET_UUID ".bin";
    const char *candidate = STORAGE_DATA_MOUNT "/transactions/" SET_UUID ".bin.tmp." OP_UUID;

    write_manifest(candidate, SET_UUID);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, validate(destination, candidate));

    /* An id that disagrees with the destination is rejected. */
    write_manifest(candidate, OTHER_UUID);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, validate(destination, candidate));

    /* A wrong-sized blob is not a manifest. */
    write_text(candidate, "too short");
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, validate(destination, candidate));
}

static void test_dispatch_refuses_without_validator(void) {
    reset_store();
    /* Every classification storage_atomic_classify_destination() can produce
     * now has a validator, so the only reachable "no validator" case is a
     * genuinely unrecognized destination. Recovery must refuse to activate
     * that candidate. (Settings are not represented here at all: they live in
     * encrypted NVS rather than as a LittleFS object, so there is no
     * atomic-write destination for a settings validator to guard.) */
    const char *unknown_dest = STORAGE_DATA_MOUNT "/mystery.dat";
    const char *unknown_candidate = STORAGE_DATA_MOUNT "/mystery.dat.tmp." OP_UUID;
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, validate(unknown_dest, unknown_candidate));
}

int main(void) {
    test_classifier();
    test_schema_marker_validator();
    test_index_validators();
    test_set_metadata_validator();
    test_macro_validator();
    test_procedure_validator();
    test_progress_validator();
    test_transaction_manifest_validator();
    test_dispatch_refuses_without_validator();
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    puts("storage atomic validators tests passed");
    return EXIT_SUCCESS;
}
