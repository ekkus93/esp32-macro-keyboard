#!/usr/bin/env python3
"""Complete FIX1 Phase 15 and record exact validation evidence."""

from __future__ import annotations

import argparse
from pathlib import Path
import re

ROOT = Path.cwd()


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def write(path: str, content: str) -> None:
    target = ROOT / path
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(content, encoding="utf-8")


def replace_once(path: str, old: str, new: str) -> None:
    text = read(path)
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{path}: expected one occurrence, found {count}: {old[:120]!r}")
    write(path, text.replace(old, new, 1))


def sub_once(path: str, pattern: str, replacement: str) -> None:
    text = read(path)
    updated, count = re.subn(pattern, replacement, text, count=1, flags=re.MULTILINE | re.DOTALL)
    if count != 1:
        raise RuntimeError(f"{path}: expected one regex occurrence, found {count}: {pattern[:120]!r}")
    write(path, updated)


def create_new(path: str, content: str) -> None:
    target = ROOT / path
    if target.exists():
        raise RuntimeError(f"{path}: file already exists")
    write(path, content)


PROCEDURE_INTERNAL_HEADER = r'''#ifndef STORAGE_REPOSITORY_PROCEDURES_INTERNAL_H
#define STORAGE_REPOSITORY_PROCEDURES_INTERNAL_H

#include "app_error.h"
#include "macro_model.h"
#include "storage_repository.h"

app_error_code_t storage_procedure_read_locked(const storage_procedure_identity_t *identity,
                                               procedure_t *out_procedure);

#endif
'''

SETS_INTERNAL_HEADER = r'''#ifndef STORAGE_REPOSITORY_SETS_INTERNAL_H
#define STORAGE_REPOSITORY_SETS_INTERNAL_H

#include "app_error.h"
#include "app_uuid.h"

#ifndef ESP_PLATFORM
typedef struct {
    void *context;
    app_error_code_t (*clear_active_set_if_matches)(void *context, const app_uuid_t *set_id);
} storage_repository_set_settings_ops_t;

void storage_repository_sets_set_settings_ops_for_test(
    const storage_repository_set_settings_ops_t *operations);
void storage_repository_sets_reset_settings_ops_for_test(void);
#endif

#endif
'''

PROGRESS_SOURCE = r'''#include "storage_repository.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "app_uuid.h"
#include "cJSON.h"
#include "macro_model.h"
#include "storage.h"
#include "storage_quarantine_internal.h"
#include "storage_repository_internal.h"
#include "storage_repository_lock.h"
#include "storage_repository_objects_json.h"
#include "storage_repository_procedures_internal.h"

static bool identity_valid(const storage_procedure_identity_t *identity) {
    return identity != NULL && app_uuid_is_valid_string(identity->set_id.value) &&
           app_uuid_is_valid_string(identity->procedure_id.value);
}

static bool progress_matches_identity(const procedure_progress_t *progress,
                                      const storage_procedure_identity_t *identity) {
    return progress != NULL && identity_valid(identity) &&
           app_uuid_equal(&progress->set_id, &identity->set_id) &&
           app_uuid_equal(&progress->procedure_id, &identity->procedure_id);
}

static bool procedure_contains_step(const procedure_t *procedure, const app_uuid_t *step_id) {
    for (size_t index = 0U; index < procedure->step_count; ++index) {
        if (app_uuid_equal(&procedure->steps[index].id, step_id)) {
            return true;
        }
    }
    return false;
}

static bool progress_steps_belong_to_procedure(const procedure_progress_t *progress,
                                               const procedure_t *procedure) {
    if (!procedure_contains_step(procedure, &progress->current_step_id)) {
        return false;
    }
    for (size_t index = 0U; index < progress->completed_step_count; ++index) {
        if (!procedure_contains_step(procedure, &progress->completed_step_ids[index])) {
            return false;
        }
    }
    for (size_t index = 0U; index < progress->skipped_step_count; ++index) {
        if (!procedure_contains_step(procedure, &progress->skipped_step_ids[index])) {
            return false;
        }
    }
    return true;
}

static app_error_code_t progress_file_path(const storage_procedure_identity_t *identity, char *path,
                                           size_t path_size) {
    if (!identity_valid(identity)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return storage_make_progress_path(&identity->set_id, &identity->procedure_id, path, path_size);
}

static app_error_code_t quarantine_progress(const char *path, const char *reason) {
    storage_quarantine_entry_t entry = {0};
    const app_error_code_t quarantine = storage_quarantine_file_locked(path, reason, &entry);
    return quarantine == APP_ERROR_NONE ? APP_ERROR_STORAGE_CORRUPT : quarantine;
}

static app_error_code_t read_progress_object_locked(const storage_procedure_identity_t *identity,
                                                    procedure_progress_t *out_progress,
                                                    char *path, size_t path_size) {
    if (!identity_valid(identity) || out_progress == NULL || path == NULL || path_size == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_progress, 0, sizeof(*out_progress));
    app_error_code_t result = progress_file_path(identity, path, path_size);
    char *data = NULL;
    size_t length = 0U;
    if (result == APP_ERROR_NONE) {
        result = storage_repository_read_bounded_file(path, STORAGE_PROGRESS_FILE_MAX_BYTES, &data,
                                                      &length);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_repository_parse_progress_json(data, length, out_progress);
    }
    free(data);
    if (result == APP_ERROR_NONE && !progress_matches_identity(out_progress, identity)) {
        memset(out_progress, 0, sizeof(*out_progress));
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    if (result == APP_ERROR_STORAGE_CORRUPT) {
        memset(out_progress, 0, sizeof(*out_progress));
        return quarantine_progress(path, "invalid procedure progress object");
    }
    return result;
}

static app_error_code_t progress_read_locked(const storage_procedure_identity_t *identity,
                                             storage_progress_snapshot_t *out_snapshot) {
    if (out_snapshot != NULL) {
        memset(out_snapshot, 0, sizeof(*out_snapshot));
    }
    if (!identity_valid(identity) || out_snapshot == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    char path[APP_PATH_MAX_BYTES];
    procedure_progress_t progress = {0};
    app_error_code_t result =
        read_progress_object_locked(identity, &progress, path, sizeof(path));
    if (result != APP_ERROR_NONE) {
        return result;
    }

    procedure_t procedure = {0};
    result = storage_procedure_read_locked(identity, &procedure);
    if (result == APP_ERROR_NOT_FOUND) {
        return quarantine_progress(path, "progress references missing procedure");
    }
    if (result != APP_ERROR_NONE) {
        return result;
    }

    out_snapshot->current_procedure_revision = procedure.revision;
    if (progress.procedure_revision != procedure.revision) {
        out_snapshot->status = STORAGE_PROGRESS_STATUS_STALE;
        out_snapshot->progress = progress;
        macro_model_free_procedure(&procedure);
        return APP_ERROR_NONE;
    }
    if (!progress_steps_belong_to_procedure(&progress, &procedure)) {
        macro_model_free_procedure(&procedure);
        return quarantine_progress(path, "progress references unknown procedure step");
    }

    out_snapshot->status = STORAGE_PROGRESS_STATUS_CURRENT;
    out_snapshot->progress = progress;
    macro_model_free_procedure(&procedure);
    return APP_ERROR_NONE;
}

static app_error_code_t write_progress_locked(const storage_procedure_identity_t *identity,
                                              const procedure_progress_t *replacement,
                                              storage_progress_snapshot_t *out_snapshot) {
    if (out_snapshot != NULL) {
        memset(out_snapshot, 0, sizeof(*out_snapshot));
    }
    if (!identity_valid(identity) || !progress_matches_identity(replacement, identity) ||
        out_snapshot == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    procedure_t procedure = {0};
    app_error_code_t result = storage_procedure_read_locked(identity, &procedure);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (replacement->procedure_revision != procedure.revision) {
        macro_model_free_procedure(&procedure);
        return APP_ERROR_CONFLICT;
    }
    if (!progress_steps_belong_to_procedure(replacement, &procedure)) {
        macro_model_free_procedure(&procedure);
        return APP_ERROR_INVALID_ARGUMENT;
    }
    macro_model_free_procedure(&procedure);

    char *json = NULL;
    size_t length = 0U;
    result = storage_repository_serialize_progress_json(replacement, &json, &length);
    char path[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        result = progress_file_path(identity, path, sizeof(path));
    }
    if (result == APP_ERROR_NONE) {
        result = storage_atomic_write(path, json, length, true);
    }
    cJSON_free(json);
    if (result == APP_ERROR_NONE) {
        result = progress_read_locked(identity, out_snapshot);
    }
    return result;
}

static app_error_code_t progress_reset_locked(const storage_procedure_identity_t *identity,
                                              uint32_t expected_procedure_revision,
                                              storage_progress_snapshot_t *out_snapshot) {
    if (out_snapshot != NULL) {
        memset(out_snapshot, 0, sizeof(*out_snapshot));
    }
    if (!identity_valid(identity) || expected_procedure_revision == 0U || out_snapshot == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    procedure_t procedure = {0};
    app_error_code_t result = storage_procedure_read_locked(identity, &procedure);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (procedure.revision != expected_procedure_revision) {
        macro_model_free_procedure(&procedure);
        return APP_ERROR_CONFLICT;
    }
    procedure_progress_t reset = {
        .schema_version = APP_SCHEMA_VERSION,
        .set_id = identity->set_id,
        .procedure_id = identity->procedure_id,
        .procedure_revision = procedure.revision,
        .current_step_id = procedure.steps[0].id,
    };
    macro_model_free_procedure(&procedure);
    return write_progress_locked(identity, &reset, out_snapshot);
}

app_error_code_t storage_progress_read(const storage_procedure_identity_t *identity,
                                       storage_progress_snapshot_t *out_snapshot) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = progress_read_locked(identity, out_snapshot);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_progress_update(const storage_procedure_identity_t *identity,
                                         const procedure_progress_t *replacement,
                                         storage_progress_snapshot_t *out_snapshot) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = write_progress_locked(identity, replacement, out_snapshot);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_progress_reset(const storage_procedure_identity_t *identity,
                                        uint32_t expected_procedure_revision,
                                        storage_progress_snapshot_t *out_snapshot) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result =
        progress_reset_locked(identity, expected_procedure_revision, out_snapshot);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}
'''

PROGRESS_TEST = r'''#include <errno.h>
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
        STORAGE_DATA_MOUNT "/global",
        STORAGE_DATA_MOUNT "/global/macros",
        STORAGE_DATA_MOUNT "/staging",
        STORAGE_DATA_MOUNT "/trash",
        STORAGE_DATA_MOUNT "/quarantine",
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
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_procedure_create(&out_set->id, out_procedure));
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

static void test_corrupt_progress_is_quarantined(void) {
    reset_store();
    macro_set_t set;
    procedure_t procedure;
    storage_procedure_identity_t identity;
    prepare(&set, &procedure, &identity);

    storage_progress_snapshot_t snapshot = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_progress_reset(&identity, 1U, &snapshot));
    char path[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_make_progress_path(&identity.set_id, &identity.procedure_id, path,
                                                    sizeof(path)));
    FILE *file = fopen(path, "wb");
    TEST_CHECK(file != NULL);
    static const char corrupt[] = "{not-json";
    TEST_CHECK_EQ_U64(sizeof(corrupt) - 1U,
                      fwrite(corrupt, 1U, sizeof(corrupt) - 1U, file));
    TEST_CHECK(fclose(file) == 0);

    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT,
                         storage_progress_read(&identity, &snapshot));
    TEST_CHECK(!path_exists(path));
    storage_quarantine_list_t quarantine = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_quarantine_list(&quarantine));
    TEST_CHECK_EQ_U64(1U, quarantine.count);
    TEST_CHECK_EQ_STRING(path, quarantine.items[0].source_path);
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
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_make_progress_path(&identity.set_id, &identity.procedure_id, path,
                                                    sizeof(path)));
    TEST_CHECK(path_exists(path));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_procedure_delete(&set.id, &procedure.id, 1U));
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
    test_corrupt_progress_is_quarantined();
    test_procedure_delete_removes_progress();
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_lock_deinit());
    puts("storage progress repository tests passed");
    return EXIT_SUCCESS;
}
'''

ACTIVE_SET_DELETE_TEST = r'''#include <errno.h>
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
#include "storage_repository_sets_internal.h"
#include "test_assert.h"
#include "test_temp_dir.h"

typedef struct {
    app_uuid_t active_set_id;
    bool has_active_set;
    app_error_code_t failure;
    size_t call_count;
} settings_fixture_t;

static app_uuid_t make_uuid(uint32_t value) {
    char text[APP_UUID_BUFFER_LENGTH];
    const int written = snprintf(text, sizeof(text), "%08" PRIx32 "-0000-4000-8000-%012" PRIx64,
                                 value, (uint64_t)value);
    TEST_CHECK_EQ_INT((int)APP_UUID_STRING_LENGTH, written);
    app_uuid_t uuid = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_uuid_parse(text, &uuid));
    return uuid;
}

static app_error_code_t clear_active_set(void *context, const app_uuid_t *set_id) {
    settings_fixture_t *fixture = context;
    ++fixture->call_count;
    if (fixture->failure != APP_ERROR_NONE) {
        return fixture->failure;
    }
    if (fixture->has_active_set && app_uuid_equal(&fixture->active_set_id, set_id)) {
        fixture->has_active_set = false;
        memset(&fixture->active_set_id, 0, sizeof(fixture->active_set_id));
    }
    return APP_ERROR_NONE;
}

static void make_directory(const char *path) {
    TEST_CHECK(mkdir(path, 0750) == 0 || errno == EEXIST);
}

static bool path_exists(const char *path) {
    struct stat metadata;
    return stat(path, &metadata) == 0;
}

static void reset_store(settings_fixture_t *fixture) {
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    static const char *const paths[] = {
        STORAGE_DATA_MOUNT,
        STORAGE_DATA_MOUNT "/sets",
        STORAGE_DATA_MOUNT "/global",
        STORAGE_DATA_MOUNT "/global/macros",
        STORAGE_DATA_MOUNT "/staging",
        STORAGE_DATA_MOUNT "/trash",
        STORAGE_DATA_MOUNT "/quarantine",
        STORAGE_DATA_MOUNT "/transactions",
    };
    for (size_t index = 0U; index < (sizeof(paths) / sizeof(paths[0])); ++index) {
        make_directory(paths[index]);
    }
    *fixture = (settings_fixture_t){0};
    const storage_repository_set_settings_ops_t operations = {
        .context = fixture,
        .clear_active_set_if_matches = clear_active_set,
    };
    storage_repository_sets_set_settings_ops_for_test(&operations);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_init());
}

static macro_set_t make_set(uint32_t value) {
    macro_set_t set = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = make_uuid(value),
        .revision = 1U,
    };
    TEST_CHECK(snprintf(set.name, sizeof(set.name), "Active set delete") > 0);
    TEST_CHECK(snprintf(set.description, sizeof(set.description), "Settings bridge") > 0);
    TEST_CHECK(snprintf(set.manufacturer, sizeof(set.manufacturer), "Test") > 0);
    TEST_CHECK(snprintf(set.model, sizeof(set.model), "Model") > 0);
    TEST_CHECK(snprintf(set.board, sizeof(set.board), "board") > 0);
    TEST_CHECK(snprintf(set.keyboard_layout, sizeof(set.keyboard_layout), "en-US") > 0);
    return set;
}

static void test_matching_active_set_is_cleared_before_delete(void) {
    settings_fixture_t fixture;
    reset_store(&fixture);
    macro_set_t set = make_set(10U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&set));
    fixture.has_active_set = true;
    fixture.active_set_id = set.id;

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_delete(&set.id, 1U));
    TEST_CHECK_EQ_U64(1U, fixture.call_count);
    TEST_CHECK(!fixture.has_active_set);
    char path[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_make_set_path(&set.id, path, sizeof(path)));
    TEST_CHECK(!path_exists(path));
}

static void test_settings_failure_blocks_filesystem_delete(void) {
    settings_fixture_t fixture;
    reset_store(&fixture);
    macro_set_t set = make_set(20U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&set));
    fixture.has_active_set = true;
    fixture.active_set_id = set.id;
    fixture.failure = APP_ERROR_STORAGE_UNAVAILABLE;

    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, storage_set_delete(&set.id, 1U));
    TEST_CHECK_EQ_U64(1U, fixture.call_count);
    TEST_CHECK(fixture.has_active_set);
    char path[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_make_set_path(&set.id, path, sizeof(path)));
    TEST_CHECK(path_exists(path));
    macro_set_t readback = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_read(&set.id, &readback));
}

static void test_nonmatching_active_set_is_preserved(void) {
    settings_fixture_t fixture;
    reset_store(&fixture);
    macro_set_t deleted = make_set(30U);
    macro_set_t active = make_set(31U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&deleted));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&active));
    fixture.has_active_set = true;
    fixture.active_set_id = active.id;

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_delete(&deleted.id, 1U));
    TEST_CHECK(fixture.has_active_set);
    TEST_CHECK_EQ_UUID(&active.id, &fixture.active_set_id);
}

int main(void) {
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_lock_init());
    test_matching_active_set_is_cleared_before_delete();
    test_settings_failure_blocks_filesystem_delete();
    test_nonmatching_active_set_is_preserved();
    storage_repository_sets_reset_settings_ops_for_test();
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_lock_deinit());
    puts("storage active-set deletion tests passed");
    return EXIT_SUCCESS;
}
'''

PROVISIONING_SETTINGS_TEST = r'''#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "provisioning.h"
#include "provisioning_core.h"
#include "test_assert.h"

typedef struct {
    uint8_t committed[PROVISIONING_RECORD_BYTES];
    uint8_t pending[PROVISIONING_RECORD_BYTES];
    size_t size;
    bool present;
    bool pending_valid;
    size_t commit_count;
} settings_store_t;

static app_error_code_t read_blob(void *context, uint8_t *output, size_t capacity,
                                  size_t *out_size) {
    settings_store_t *store = context;
    if (!store->present) {
        return APP_ERROR_NOT_FOUND;
    }
    if (capacity < store->size) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    memcpy(output, store->committed, store->size);
    *out_size = store->size;
    return APP_ERROR_NONE;
}

static app_error_code_t write_blob(void *context, const uint8_t *data, size_t size) {
    settings_store_t *store = context;
    if (size != sizeof(store->pending)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memcpy(store->pending, data, size);
    store->size = size;
    store->pending_valid = true;
    return APP_ERROR_NONE;
}

static app_error_code_t erase_blob(void *context) {
    settings_store_t *store = context;
    memset(store->pending, 0, sizeof(store->pending));
    store->size = 0U;
    store->pending_valid = true;
    return APP_ERROR_NONE;
}

static app_error_code_t commit_blob(void *context) {
    settings_store_t *store = context;
    ++store->commit_count;
    if (!store->pending_valid) {
        return APP_ERROR_INTERNAL;
    }
    if (store->size == 0U) {
        memset(store->committed, 0, sizeof(store->committed));
        store->present = false;
    } else {
        memcpy(store->committed, store->pending, store->size);
        store->present = true;
    }
    store->pending_valid = false;
    return APP_ERROR_NONE;
}

static void secure_zero(void *context, void *memory, size_t size) {
    (void)context;
    volatile uint8_t *bytes = memory;
    for (size_t index = 0U; index < size; ++index) {
        bytes[index] = 0U;
    }
}

static app_uuid_t make_uuid(void) {
    app_uuid_t uuid = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         app_uuid_parse("12345678-0000-4000-8000-000000000001", &uuid));
    return uuid;
}

static provisioning_core_t initialized_core(settings_store_t *store) {
    const provisioning_ops_t operations = {
        .context = store,
        .read_blob = read_blob,
        .write_blob = write_blob,
        .erase_blob = erase_blob,
        .commit = commit_blob,
        .secure_zero = secure_zero,
    };
    provisioning_core_t core = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_init(&core, &operations));
    provisioning_config_t configuration = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_load(&core, &configuration));
    return core;
}

static void test_redacted_settings_round_trip_and_clear(void) {
    settings_store_t store = {0};
    provisioning_core_t core = initialized_core(&store);
    provisioning_settings_t settings = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_settings_read(&core, &settings));
    TEST_CHECK_EQ_U64(APP_SCHEMA_VERSION, settings.schema_version);
    TEST_CHECK_EQ_U64(0U, settings.revision);
    TEST_CHECK(settings.always_select_set);
    TEST_CHECK(settings.require_physical_confirmation);
    TEST_CHECK(!settings.has_active_set);

    settings.always_select_set = false;
    settings.has_active_set = true;
    settings.active_set_id = make_uuid();
    provisioning_settings_t committed = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         provisioning_core_settings_update(&core, &settings, 0U, &committed));
    TEST_CHECK_EQ_U64(1U, committed.revision);
    TEST_CHECK(!committed.always_select_set);
    TEST_CHECK(committed.has_active_set);
    TEST_CHECK_EQ_UUID(&settings.active_set_id, &committed.active_set_id);
    TEST_CHECK_EQ_U64(1U, store.commit_count);

    bool cleared = true;
    const app_uuid_t other = {
        .value = "87654321-0000-4000-8000-000000000002",
    };
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         provisioning_core_clear_active_set_if_matches(&core, &other, &cleared));
    TEST_CHECK(!cleared);
    TEST_CHECK_EQ_U64(1U, store.commit_count);

    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE,
        provisioning_core_clear_active_set_if_matches(&core, &settings.active_set_id, &cleared));
    TEST_CHECK(cleared);
    TEST_CHECK_EQ_U64(2U, store.commit_count);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_settings_read(&core, &committed));
    TEST_CHECK_EQ_U64(2U, committed.revision);
    TEST_CHECK(!committed.has_active_set);

    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT,
                         provisioning_core_settings_update(&core, &settings, 0U, &committed));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_deinit(&core));
}

static void test_settings_validation(void) {
    settings_store_t store = {0};
    provisioning_core_t core = initialized_core(&store);
    provisioning_settings_t settings = {
        .schema_version = APP_SCHEMA_VERSION,
        .revision = 0U,
        .require_physical_confirmation = true,
        .always_select_set = true,
        .has_active_set = true,
    };
    provisioning_settings_t output = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         provisioning_core_settings_update(&core, &settings, 0U, &output));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         provisioning_core_clear_active_set_if_matches(&core, NULL, NULL));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_core_deinit(&core));
}

int main(void) {
    test_redacted_settings_round_trip_and_clear();
    test_settings_validation();
    puts("provisioning settings tests passed");
    return EXIT_SUCCESS;
}
'''


def apply_code() -> None:
    create_new("firmware/components/storage/storage_repository_procedures_internal.h",
               PROCEDURE_INTERNAL_HEADER)
    create_new("firmware/components/storage/storage_repository_sets_internal.h", SETS_INTERNAL_HEADER)
    create_new("firmware/components/storage/storage_repository_progress.c", PROGRESS_SOURCE)
    create_new("tests/host/test_storage_progress.c", PROGRESS_TEST)
    create_new("tests/host/test_storage_active_set_delete.c", ACTIVE_SET_DELETE_TEST)
    create_new("tests/host/test_provisioning_settings.c", PROVISIONING_SETTINGS_TEST)

    replace_once(
        "firmware/components/storage/include/storage_repository.h",
        "typedef struct {\n    procedure_t *items;\n    size_t count;\n} storage_procedure_list_t;\n",
        "typedef struct {\n    procedure_t *items;\n    size_t count;\n} storage_procedure_list_t;\n\n"
        "typedef struct {\n    app_uuid_t set_id;\n    app_uuid_t procedure_id;\n} storage_procedure_identity_t;\n\n"
        "typedef enum {\n    STORAGE_PROGRESS_STATUS_CURRENT = 0,\n"
        "    STORAGE_PROGRESS_STATUS_STALE,\n} storage_progress_status_t;\n\n"
        "typedef struct {\n    procedure_progress_t progress;\n"
        "    storage_progress_status_t status;\n"
        "    uint32_t current_procedure_revision;\n} storage_progress_snapshot_t;\n",
    )
    replace_once(
        "firmware/components/storage/include/storage_repository.h",
        "app_error_code_t storage_procedure_reorder(const app_uuid_t *set_id, const app_uuid_t *ordered_ids,\n"
        "                                           size_t count);\n\n#endif\n",
        "app_error_code_t storage_procedure_reorder(const app_uuid_t *set_id, const app_uuid_t *ordered_ids,\n"
        "                                           size_t count);\n\n"
        "app_error_code_t storage_progress_read(const storage_procedure_identity_t *identity,\n"
        "                                       storage_progress_snapshot_t *out_snapshot);\n"
        "app_error_code_t storage_progress_update(const storage_procedure_identity_t *identity,\n"
        "                                         const procedure_progress_t *replacement,\n"
        "                                         storage_progress_snapshot_t *out_snapshot);\n"
        "app_error_code_t storage_progress_reset(const storage_procedure_identity_t *identity,\n"
        "                                        uint32_t expected_procedure_revision,\n"
        "                                        storage_progress_snapshot_t *out_snapshot);\n\n#endif\n",
    )

    replace_once(
        "firmware/components/storage/storage_repository_procedures.c",
        '#include "storage_repository_objects_json.h"\n',
        '#include "storage_repository_objects_json.h"\n'
        '#include "storage_repository_procedures_internal.h"\n',
    )
    sub_once(
        "firmware/components/storage/storage_repository_procedures.c",
        r"static app_error_code_t procedure_read_locked\(const app_uuid_t \*set_id,\n"
        r"\s+const app_uuid_t \*procedure_id,\n"
        r"\s+procedure_t \*out_procedure\) \{.*?\n\}\n\n"
        r"static app_error_code_t procedure_list_locked",
        "app_error_code_t storage_procedure_read_locked(const storage_procedure_identity_t *identity,\n"
        "                                               procedure_t *out_procedure) {\n"
        "    if (identity == NULL || out_procedure == NULL) {\n"
        "        return APP_ERROR_INVALID_ARGUMENT;\n"
        "    }\n"
        "    app_error_code_t result = procedure_read_object_locked(\n"
        "        &identity->set_id, &identity->procedure_id, out_procedure);\n"
        "    size_t index = 0U;\n"
        "    if (result == APP_ERROR_NONE) {\n"
        "        result = procedure_order_index(&identity->set_id, &index, &identity->procedure_id);\n"
        "    }\n"
        "    if (result == APP_ERROR_NONE) {\n"
        "        out_procedure->sort_order = (int32_t)index;\n"
        "    } else {\n"
        "        macro_model_free_procedure(out_procedure);\n"
        "        memset(out_procedure, 0, sizeof(*out_procedure));\n"
        "    }\n"
        "    return result;\n"
        "}\n\n"
        "static app_error_code_t procedure_read_locked(const app_uuid_t *set_id,\n"
        "                                              const app_uuid_t *procedure_id,\n"
        "                                              procedure_t *out_procedure) {\n"
        "    if (set_id == NULL || procedure_id == NULL) {\n"
        "        return APP_ERROR_INVALID_ARGUMENT;\n"
        "    }\n"
        "    const storage_procedure_identity_t identity = {\n"
        "        .set_id = *set_id,\n"
        "        .procedure_id = *procedure_id,\n"
        "    };\n"
        "    return storage_procedure_read_locked(&identity, out_procedure);\n"
        "}\n\n"
        "static app_error_code_t procedure_list_locked",
    )
    replace_once(
        "firmware/components/storage/storage_repository_procedures.c",
        "static app_error_code_t procedure_delete_locked(const app_uuid_t *set_id,\n",
        "static app_error_code_t remove_progress_if_present(const app_uuid_t *set_id,\n"
        "                                                     const app_uuid_t *procedure_id) {\n"
        "    char path[APP_PATH_MAX_BYTES];\n"
        "    const app_error_code_t result =\n"
        "        storage_make_progress_path(set_id, procedure_id, path, sizeof(path));\n"
        "    if (result != APP_ERROR_NONE) {\n"
        "        return result;\n"
        "    }\n"
        "    if (unlink(path) == 0 || errno == ENOENT) {\n"
        "        return APP_ERROR_NONE;\n"
        "    }\n"
        "    return storage_repository_map_file_error();\n"
        "}\n\n"
        "static app_error_code_t procedure_delete_locked(const app_uuid_t *set_id,\n",
    )
    replace_once(
        "firmware/components/storage/storage_repository_procedures.c",
        "    macro_model_free_procedure(&current);\n    storage_uuid_order_t order = {0};\n",
        "    macro_model_free_procedure(&current);\n"
        "    result = remove_progress_if_present(set_id, procedure_id);\n"
        "    if (result != APP_ERROR_NONE) {\n"
        "        return result;\n"
        "    }\n"
        "    storage_uuid_order_t order = {0};\n",
    )

    replace_once(
        "firmware/components/provisioning/include/provisioning.h",
        "} provisioning_config_t;\n\napp_error_code_t provisioning_init(void);\n",
        "} provisioning_config_t;\n\n"
        "typedef struct {\n    uint32_t schema_version;\n    uint32_t revision;\n"
        "    bool require_physical_confirmation;\n    bool always_select_set;\n"
        "    bool has_active_set;\n    app_uuid_t active_set_id;\n} provisioning_settings_t;\n\n"
        "app_error_code_t provisioning_init(void);\n",
    )
    replace_once(
        "firmware/components/provisioning/include/provisioning.h",
        "app_error_code_t provisioning_clear_credentials(void);\n",
        "app_error_code_t provisioning_settings_read(provisioning_settings_t *out_settings);\n"
        "app_error_code_t provisioning_settings_update(const provisioning_settings_t *replacement,\n"
        "                                               uint32_t expected_revision,\n"
        "                                               provisioning_settings_t *out_committed);\n"
        "app_error_code_t provisioning_clear_active_set_if_matches(const app_uuid_t *set_id,\n"
        "                                                          bool *out_cleared);\n"
        "app_error_code_t provisioning_clear_credentials(void);\n",
    )
    replace_once(
        "firmware/components/provisioning/provisioning_core.h",
        "app_error_code_t provisioning_core_clear_credentials(provisioning_core_t *core);\n",
        "app_error_code_t provisioning_core_settings_read(provisioning_core_t *core,\n"
        "                                                  provisioning_settings_t *out_settings);\n"
        "app_error_code_t provisioning_core_settings_update(\n"
        "    provisioning_core_t *core, const provisioning_settings_t *replacement,\n"
        "    uint32_t expected_revision, provisioning_settings_t *out_committed);\n"
        "app_error_code_t provisioning_core_clear_active_set_if_matches(\n"
        "    provisioning_core_t *core, const app_uuid_t *set_id, bool *out_cleared);\n"
        "app_error_code_t provisioning_core_clear_credentials(provisioning_core_t *core);\n",
    )
    replace_once(
        "firmware/components/provisioning/provisioning_core.c",
        "app_error_code_t provisioning_core_clear_credentials(provisioning_core_t *core) {\n",
        r'''static provisioning_settings_t settings_from_configuration(
    const provisioning_config_t *configuration) {
    return (provisioning_settings_t){
        .schema_version = APP_SCHEMA_VERSION,
        .revision = configuration->revision,
        .require_physical_confirmation = configuration->require_physical_confirmation,
        .always_select_set = configuration->always_select_set,
        .has_active_set = configuration->has_active_set,
        .active_set_id = configuration->active_set_id,
    };
}

static bool settings_valid(const provisioning_settings_t *settings) {
    if (settings == NULL || settings->schema_version != APP_SCHEMA_VERSION) {
        return false;
    }
    if (settings->has_active_set) {
        return app_uuid_is_valid_string(settings->active_set_id.value);
    }
    return all_zero(&settings->active_set_id, sizeof(settings->active_set_id));
}

app_error_code_t provisioning_core_settings_read(provisioning_core_t *core,
                                                  provisioning_settings_t *out_settings) {
    if (out_settings != NULL) {
        memset(out_settings, 0, sizeof(*out_settings));
    }
    if (core == NULL || out_settings == NULL || !core->initialized || !core->loaded) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_settings = settings_from_configuration(&core->current);
    return APP_ERROR_NONE;
}

app_error_code_t provisioning_core_settings_update(
    provisioning_core_t *core, const provisioning_settings_t *replacement,
    uint32_t expected_revision, provisioning_settings_t *out_committed) {
    if (out_committed != NULL) {
        memset(out_committed, 0, sizeof(*out_committed));
    }
    if (core == NULL || replacement == NULL || out_committed == NULL || !core->initialized ||
        !core->loaded || !settings_valid(replacement) ||
        replacement->revision != expected_revision) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (core->current.revision != expected_revision) {
        return APP_ERROR_CONFLICT;
    }

    provisioning_config_t candidate = core->current;
    candidate.require_physical_confirmation = replacement->require_physical_confirmation;
    candidate.always_select_set = replacement->always_select_set;
    candidate.has_active_set = replacement->has_active_set;
    candidate.active_set_id = replacement->active_set_id;
    provisioning_config_t committed = {0};
    const app_error_code_t result =
        provisioning_core_commit(core, &candidate, expected_revision, &committed);
    if (result == APP_ERROR_NONE) {
        *out_committed = settings_from_configuration(&committed);
    }
    core->operations.secure_zero(core->operations.context, &candidate, sizeof(candidate));
    core->operations.secure_zero(core->operations.context, &committed, sizeof(committed));
    return result;
}

app_error_code_t provisioning_core_clear_active_set_if_matches(
    provisioning_core_t *core, const app_uuid_t *set_id, bool *out_cleared) {
    if (out_cleared != NULL) {
        *out_cleared = false;
    }
    if (core == NULL || set_id == NULL || !core->initialized || !core->loaded ||
        !app_uuid_is_valid_string(set_id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (!core->current.has_active_set || !app_uuid_equal(&core->current.active_set_id, set_id)) {
        return APP_ERROR_NONE;
    }

    provisioning_settings_t replacement = settings_from_configuration(&core->current);
    replacement.has_active_set = false;
    memset(&replacement.active_set_id, 0, sizeof(replacement.active_set_id));
    provisioning_settings_t committed = {0};
    const app_error_code_t result = provisioning_core_settings_update(
        core, &replacement, core->current.revision, &committed);
    if (result == APP_ERROR_NONE && out_cleared != NULL) {
        *out_cleared = true;
    }
    return result;
}

app_error_code_t provisioning_core_clear_credentials(provisioning_core_t *core) {
''',
    )
    replace_once(
        "firmware/components/provisioning/provisioning.c",
        "app_error_code_t provisioning_clear_credentials(void) {\n",
        r'''app_error_code_t provisioning_settings_read(provisioning_settings_t *out_settings) {
    if (out_settings == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = lock_provisioning();
    if (result != APP_ERROR_NONE) {
        return result;
    }
    result = provisioning_core_settings_read(&core, out_settings);
    return finish_locked(result);
}

app_error_code_t provisioning_settings_update(const provisioning_settings_t *replacement,
                                               uint32_t expected_revision,
                                               provisioning_settings_t *out_committed) {
    if (replacement == NULL || out_committed == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = lock_provisioning();
    if (result != APP_ERROR_NONE) {
        return result;
    }
    result = provisioning_core_settings_update(&core, replacement, expected_revision, out_committed);
    return finish_locked(result);
}

app_error_code_t provisioning_clear_active_set_if_matches(const app_uuid_t *set_id,
                                                          bool *out_cleared) {
    app_error_code_t result = lock_provisioning();
    if (result != APP_ERROR_NONE) {
        return result;
    }
    result = provisioning_core_clear_active_set_if_matches(&core, set_id, out_cleared);
    return finish_locked(result);
}

app_error_code_t provisioning_clear_credentials(void) {
''',
    )

    replace_once(
        "firmware/components/storage/storage_repository_sets.c",
        '#include "storage_repository_lock.h"\n',
        '#include "storage_repository_lock.h"\n'
        '#include "storage_repository_sets_internal.h"\n\n'
        '#ifdef ESP_PLATFORM\n#include "provisioning.h"\n#endif\n',
    )
    replace_once(
        "firmware/components/storage/storage_repository_sets.c",
        "/* Public set functions serialize their whole read-check-write transaction behind\n",
        r'''#ifdef ESP_PLATFORM
static app_error_code_t clear_matching_active_set(const app_uuid_t *set_id) {
    return provisioning_clear_active_set_if_matches(set_id, NULL);
}
#else
static app_error_code_t host_clear_no_active_set(void *context, const app_uuid_t *set_id) {
    (void)context;
    (void)set_id;
    return APP_ERROR_NONE;
}

static storage_repository_set_settings_ops_t settings_operations = {
    .context = NULL,
    .clear_active_set_if_matches = host_clear_no_active_set,
};

void storage_repository_sets_set_settings_ops_for_test(
    const storage_repository_set_settings_ops_t *operations) {
    if (operations == NULL || operations->clear_active_set_if_matches == NULL) {
        storage_repository_sets_reset_settings_ops_for_test();
        return;
    }
    settings_operations = *operations;
}

void storage_repository_sets_reset_settings_ops_for_test(void) {
    settings_operations = (storage_repository_set_settings_ops_t){
        .context = NULL,
        .clear_active_set_if_matches = host_clear_no_active_set,
    };
}

static app_error_code_t clear_matching_active_set(const app_uuid_t *set_id) {
    return settings_operations.clear_active_set_if_matches(settings_operations.context, set_id);
}
#endif

/* Public set functions serialize their whole read-check-write transaction behind
''',
    )
    replace_once(
        "firmware/components/storage/storage_repository_sets.c",
        "    if (current.revision != expected_revision) {\n        return APP_ERROR_CONFLICT;\n    }\n"
        "    storage_set_index_t index = {0};\n",
        "    if (current.revision != expected_revision) {\n        return APP_ERROR_CONFLICT;\n    }\n"
        "    result = clear_matching_active_set(set_id);\n"
        "    if (result != APP_ERROR_NONE) {\n        return result;\n    }\n"
        "    storage_set_index_t index = {0};\n",
    )

    replace_once(
        "firmware/components/storage/CMakeLists.txt",
        '    "storage_repository_lock.c"\n    "storage_repository_macros.c"\n',
        '    "storage_repository_lock.c"\n    "storage_repository_order.c"\n'
        '    "storage_repository_macros.c"\n',
    )
    replace_once(
        "firmware/components/storage/CMakeLists.txt",
        '    "storage_repository_procedures.c"\n    "storage_quarantine.c"\n',
        '    "storage_repository_procedures.c"\n    "storage_repository_progress.c"\n'
        '    "storage_quarantine.c"\n',
    )
    replace_once(
        "firmware/components/storage/CMakeLists.txt",
        "    macro_model\n    littlefs\n",
        "    macro_model\n    provisioning\n    littlefs\n",
    )

    cmake_append = r'''

set(
    STORAGE_OBJECT_REPOSITORY_SOURCES
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/macro_model/app_uuid.c
    ../../firmware/components/macro_model/macro_model.c
    ../../firmware/components/storage/storage_atomic.c
    ../../firmware/components/storage/storage_fs_ops.c
    ../../firmware/components/storage/storage_paths.c
    ../../firmware/components/storage/storage_transaction.c
    ../../firmware/components/storage/storage_repository_io.c
    ../../firmware/components/storage/storage_repository_json.c
    ../../firmware/components/storage/storage_json.c
    ../../firmware/components/storage/storage_repository_objects_json.c
    ../../firmware/components/storage/storage_repository_index.c
    ../../firmware/components/storage/storage_repository_sets.c
    ../../firmware/components/storage/storage_repository_lock.c
    ../../firmware/components/storage/storage_repository_order.c
    ../../firmware/components/storage/storage_repository_macros.c
    ../../firmware/components/storage/storage_repository_procedures.c
    ../../firmware/components/storage/storage_repository_progress.c
    ../../firmware/components/storage/storage_quarantine.c
)

function(add_storage_object_repository_test target source)
    add_executable(${target} ${source} ${STORAGE_OBJECT_REPOSITORY_SOURCES})
    target_include_directories(
        ${target}
        PRIVATE ../../firmware/components/macro_model/include
                ../../firmware/components/storage/include
                ../../firmware/components/storage
    )
    target_compile_definitions(
        ${target} PRIVATE STORAGE_DATA_MOUNT="${CMAKE_CURRENT_BINARY_DIR}/${target}-data"
    )
    target_compile_options(${target} PRIVATE ${STRICT_WARNINGS})
    target_link_libraries(${target} PRIVATE PkgConfig::CJSON test_support)
    add_test(NAME ${target} COMMAND ${target})
    set_tests_properties(${target} PROPERTIES LABELS "storage")
endfunction()

add_storage_object_repository_test(storage_macro_repository_tests test_storage_macros.c)
add_storage_object_repository_test(storage_procedure_repository_tests test_storage_procedures.c)
add_storage_object_repository_test(storage_progress_repository_tests test_storage_progress.c)
add_storage_object_repository_test(storage_active_set_delete_tests test_storage_active_set_delete.c)

add_executable(
    provisioning_settings_tests
    test_provisioning_settings.c
    ../../firmware/components/macro_model/app_uuid.c
    ../../firmware/components/provisioning/provisioning_core.c
)
target_include_directories(
    provisioning_settings_tests
    PRIVATE ../../firmware/components/macro_model/include
            ../../firmware/components/auth/include
            ../../firmware/components/wifi_ap/include
            ../../firmware/components/provisioning/include
            ../../firmware/components/provisioning
)
target_link_libraries(provisioning_settings_tests PRIVATE test_support)
target_compile_options(provisioning_settings_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME provisioning_settings COMMAND provisioning_settings_tests)
set_tests_properties(provisioning_settings PROPERTIES LABELS "storage")
'''
    cmake_path = "tests/host/CMakeLists.txt"
    cmake_text = read(cmake_path)
    if "add_storage_object_repository_test" in cmake_text:
        raise RuntimeError("tests/host/CMakeLists.txt: Phase 15 targets already registered")
    write(cmake_path, cmake_text.rstrip() + cmake_append)

    todo_path = "docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md"
    todo = read(todo_path)
    phase_start = todo.index("## 15. Complete storage object repositories")
    phase_end = todo.index("## 16. Complete the HTTP API", phase_start)
    phase = todo[phase_start:phase_end]
    phase = phase.replace("- [ ]", "- [x]")
    phase += (
        "Phase 15 implementation notes:\n\n"
        "- Macro and procedure CRUD/order/reference/quarantine suites are registered as real CTest targets;\n"
        "  the previously unregistered source-only tests no longer provide false confidence.\n"
        "- Progress reads return an explicit `CURRENT` or `STALE` status and the current procedure\n"
        "  revision; stale records remain visible and can only be reset against the current revision.\n"
        "- Non-secret settings and active-set selection remain in the encrypted provisioning record\n"
        "  through a redacted settings API. Set deletion clears a matching active set before the\n"
        "  LittleFS transaction, preventing a power loss from leaving a dangling active-set ID.\n\n"
    )
    write(todo_path, todo[:phase_start] + phase + todo[phase_end:])


def record_evidence(commit_sha: str) -> None:
    progress_path = "docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_PROGRESS.md"
    progress = read(progress_path)
    progress = progress.replace(
        "| 13 | Fix device-controls shutdown and failure visibility | not started |\n"
        "| 14 | Encrypted persistent provisioning | not started |\n"
        "| 15 | Complete storage object repositories | not started |\n"
        "| 16 | Complete the HTTP API | not started |",
        "| 13 | Fix device-controls shutdown and failure visibility | done (diagnostics aggregation deferred to Phase 19) |\n"
        "| 14 | Encrypted persistent provisioning | done (physical confidentiality remains Phase 20 hardware evidence) |\n"
        "| 15 | Complete storage object repositories | done |\n"
        "| 16 | Complete the HTTP API | next |",
    )
    evidence = f'''\n## Phase 15 — Complete storage object repositories\n\nStatus: **Complete.**\n\nImplementation commit: `{commit_sha}`.\n\nImplemented and validated:\n\n- macro list/create/read/update/delete/duplicate/reorder, bounded procedure-reference details,\n  scope validation, revision conflicts, and corrupt-object/order quarantine;\n- procedure list/create/read/update/delete/reorder, unique step IDs, strict macro scope/reference\n  validation, exact fields and bounds, revision conflicts, corrupt-object quarantine, and progress\n  removal before procedure deletion;\n- progress read/update/reset with canonical JSON, procedure and step validation, atomic writes,\n  completed/skipped exclusivity, explicit stale-revision snapshots, and corruption quarantine;\n- redacted non-secret settings read/update over the encrypted provisioning record, including\n  always-select-set, active-set selection, and physical-confirmation policy;\n- set deletion clears a matching encrypted-NVS active-set selection before moving the set to\n  transaction-owned trash, so interruption cannot leave a dangling selection; existing delete\n  transaction recovery remains idempotent and preserves global macros.\n\nValidation:\n\n- `./scripts/run-tests.sh storage`;\n- `./scripts/run-tests.sh --sanitizers storage`;\n- `./scripts/generate-native-coverage.sh`;\n- authoritative `./scripts/check-all.sh`;\n- ESP-IDF v5.5.5 production and device-test builds with fail-closed clang-tidy;\n- macro, procedure, progress, active-set deletion, provisioning-settings, atomic-validator, and\n  transaction-recovery host suites all execute as registered CTest targets.\n'''
    if "## Phase 15 — Complete storage object repositories" in progress:
        raise RuntimeError("progress document already contains Phase 15 evidence")
    write(progress_path, progress.rstrip() + "\n" + evidence)

    implementation = f'''# Implementation status\n\n**Updated:** 2026-07-28\n\nThis file distinguishes implemented software from hardware-validated and release-ready behavior.\n\n## Completed software phases\n\n- FIX1 Phases 1–15 are implemented on `master`.\n- Phase 13 cooperative controls shutdown is complete; redacted controls-health aggregation remains\n  part of Phase 19.\n- Phase 14 encrypted persistent provisioning is software-complete; physical eFuse/HMAC-backed NVS\n  confidentiality and reboot behavior remain Phase 20 hardware evidence.\n- Phase 15 storage object repositories are complete at `{commit_sha}`: set, macro, procedure,\n  progress, ordering, reference validation, stale-progress visibility, non-secret settings,\n  active-set consistency, atomic object writes, quarantine, and set transaction recovery.\n\n## Validation at the Phase 15 gate\n\nThe authoritative CI-pinned toolchain passed:\n\n- `./scripts/check-all.sh`;\n- storage host tests and ASan/UBSan;\n- native coverage;\n- frontend checks;\n- production firmware and device-test builds;\n- fail-closed clang-tidy with zero first-party findings;\n- formatting, scripts, documentation, partition, and production-configuration policy gates.\n\nMacro and procedure repository tests are now registered CTest targets rather than dormant source\nfiles, and the firmware storage component now compiles the shared order implementation.\n\n## Release-blocking work still open\n\n- Phase 16: complete HTTP resource APIs and server-owned persisted-macro execution submission.\n- Phase 17: replace remaining frontend mock/incomplete workflows and add accessibility/E2E tests.\n- Phase 18: import, export, transactional replace, backup, and restore.\n- Phase 19: redacted diagnostics and complete subsystem-health aggregation.\n- Phase 20: USB, SoftAP/browser, encrypted-NVS reboot, power-loss, controls, and other real-hardware\n  evidence.\n- Phase 21: release size/resource budgets and immutable pinned GitHub Actions.\n- Phases 22–23: final documentation synchronization and acceptance.\n\nNo hardware result or release-readiness claim is implied by the passing software gate.\n'''
    write("docs/IMPLEMENTATION_STATUS.md", implementation)

    handoff = f'''# FIX1 Handoff — Resume at Phase 16\n\n## Current state\n\n- Work is direct on `master`; do not create PR branches unless the operator changes that decision.\n- FIX1 Phases 1–15 are complete.\n- Phase 15 implementation commit: `{commit_sha}`.\n- The authoritative gate passed after Phase 15: `./scripts/check-all.sh`, storage ASan/UBSan, and\n  native coverage.\n- No first-party warning suppression or fail-open fallback was introduced.\n\n## Next phase\n\nResume at `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md`\n**Phase 16 — Complete the HTTP API**.\n\nThe storage surface available to Phase 16 includes:\n\n- set CRUD and transaction-owned delete;\n- set-local and global macro CRUD, duplicate, order, and bounded reference-conflict details;\n- procedure CRUD and order with strict macro reference validation;\n- progress read/update/reset with explicit current/stale status;\n- redacted encrypted-store settings read/update and active-set selection;\n- corrupt object/order quarantine and atomic recovery validators.\n\n## Phase 16 priorities\n\n1. Centralized request policy for Content-Type, body limits, Host, Origin, cookie, CSRF, session,\n   request ID, and physical confirmation.\n2. Strict path UUID parsing and unknown-field rejection.\n3. Set, macro, procedure, progress, settings, and active-set routes over the completed repositories.\n4. Server-owned execution submission: load persisted macro by identity and exact revision, compile\n   it server-side, transfer plan ownership, and return `202` only after executor acceptance.\n5. Route-level failure/status mapping and comprehensive security/error tests.\n\n## Deferred hardware and later-phase work\n\n- Controls health aggregation: Phase 19.\n- Execution identity/timestamps/current-action diagnostics: Phases 16/19.\n- Import/restore lock serialization: Phase 18.\n- eFuse/HMAC NVS physical validation, USB, SoftAP/browser, power interruption, and controls timing:\n  Phase 20 on real ESP32-S3 hardware.\n\n## Required rules\n\n- Run the smallest relevant host suite during development and the full `./scripts/check-all.sh`\n  before completion.\n- Preserve fail-closed behavior and separate primary from cleanup errors.\n- Do not use `NOLINT`, `eslint-disable`, `-Wno-*`, `|| true`, or other first-party suppression.\n- Update the TODO and progress evidence with every completed phase.\n'''
    write("docs/FIX1_CHATGPT_HANDOFF.md", handoff)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--record-evidence")
    arguments = parser.parse_args()
    if arguments.record_evidence:
        record_evidence(arguments.record_evidence)
    else:
        apply_code()


if __name__ == "__main__":
    main()
