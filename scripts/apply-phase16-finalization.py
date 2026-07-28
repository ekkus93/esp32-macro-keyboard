#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def write(path: str, content: str) -> None:
    target = ROOT / path
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(content, encoding="utf-8")


def replace_once(path: str, old: str, new: str) -> None:
    content = read(path)
    count = content.count(old)
    if count != 1:
        raise RuntimeError(f"{path}: expected one match, found {count}: {old!r}")
    write(path, content.replace(old, new, 1))


def regex_once(path: str, pattern: str, replacement: str) -> None:
    content = read(path)
    updated, count = re.subn(pattern, replacement, content, count=1, flags=re.MULTILINE | re.DOTALL)
    if count != 1:
        raise RuntimeError(f"{path}: expected one regex match, found {count}: {pattern!r}")
    write(path, updated)


SET_OPERATIONS_C = r'''#include "storage_repository.h"

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
#include "cJSON.h"
#include "macro_limits.h"
#include "macro_model.h"
#include "storage.h"
#include "storage_object_json.h"
#include "storage_repository_internal.h"
#include "storage_repository_lock.h"
#include "storage_repository_macros_internal.h"
#include "storage_repository_procedures_internal.h"
#include "storage_repository_sets_internal.h"

static app_error_code_t copy_text(char *destination, size_t destination_size, const char *source,
                                  size_t maximum_length) {
    if (destination == NULL || destination_size == 0U || source == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const size_t length = strlen(source);
    if (length == 0U || length > maximum_length || length >= destination_size) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(destination, 0, destination_size);
    memcpy(destination, source, length + 1U);
    return APP_ERROR_NONE;
}

static app_error_code_t write_json_file(const char *path, char *json, size_t json_length) {
    if (path == NULL || json == NULL || json_length == 0U) {
        cJSON_free(json);
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const app_error_code_t result = storage_atomic_write(path, json, json_length, true);
    cJSON_free(json);
    return result;
}

static app_error_code_t make_child_directory(const char *parent, const char *name) {
    char path[APP_PATH_MAX_BYTES];
    const int written = snprintf(path, sizeof(path), "%s/%s", parent, name);
    if (written < 0 || (size_t)written >= sizeof(path)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return storage_repository_make_directory(path);
}

static app_error_code_t write_duplicate_metadata(const char *staging,
                                                 const macro_set_t *duplicate) {
    char *json = NULL;
    size_t json_length = 0U;
    app_error_code_t result =
        storage_repository_serialize_set_json(duplicate, &json, &json_length);
    char path[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        const int written = snprintf(path, sizeof(path), "%s/set.json", staging);
        if (written < 0 || (size_t)written >= sizeof(path)) {
            result = APP_ERROR_INVALID_ARGUMENT;
        }
    }
    if (result != APP_ERROR_NONE) {
        cJSON_free(json);
        return result;
    }
    return write_json_file(path, json, json_length);
}

static app_error_code_t write_duplicate_macro(const char *staging, const macro_t *source,
                                              const app_uuid_t *duplicate_set_id) {
    macro_t duplicate = *source;
    duplicate.revision = 1U;
    duplicate.has_set_id = true;
    duplicate.set_id = *duplicate_set_id;
    char *json = NULL;
    size_t json_length = 0U;
    app_error_code_t result =
        storage_repository_serialize_macro_json(&duplicate, &json, &json_length);
    char path[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        const int written =
            snprintf(path, sizeof(path), "%s/macros/%s.json", staging, duplicate.id.value);
        if (written < 0 || (size_t)written >= sizeof(path)) {
            result = APP_ERROR_INVALID_ARGUMENT;
        }
    }
    if (result != APP_ERROR_NONE) {
        cJSON_free(json);
        return result;
    }
    return write_json_file(path, json, json_length);
}

static app_error_code_t write_duplicate_procedure(const char *staging,
                                                  const procedure_t *source,
                                                  const app_uuid_t *duplicate_set_id) {
    procedure_t duplicate = *source;
    duplicate.revision = 1U;
    duplicate.set_id = *duplicate_set_id;
    char *json = NULL;
    size_t json_length = 0U;
    app_error_code_t result =
        storage_repository_serialize_procedure_json(&duplicate, &json, &json_length);
    char path[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        const int written =
            snprintf(path, sizeof(path), "%s/procedures/%s.json", staging, duplicate.id.value);
        if (written < 0 || (size_t)written >= sizeof(path)) {
            result = APP_ERROR_INVALID_ARGUMENT;
        }
    }
    if (result != APP_ERROR_NONE) {
        cJSON_free(json);
        return result;
    }
    return write_json_file(path, json, json_length);
}

static app_error_code_t write_duplicate_order(const char *staging, const char *filename,
                                              const app_uuid_t *ids, size_t count,
                                              size_t maximum_count) {
    storage_uuid_order_t order = {.count = count};
    if (count > 0U) {
        memcpy(order.ids, ids, count * sizeof(*ids));
    }
    char *json = NULL;
    size_t json_length = 0U;
    app_error_code_t result =
        storage_repository_serialize_order_json(&order, maximum_count, &json, &json_length);
    char path[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        const int written = snprintf(path, sizeof(path), "%s/%s", staging, filename);
        if (written < 0 || (size_t)written >= sizeof(path)) {
            result = APP_ERROR_INVALID_ARGUMENT;
        }
    }
    if (result != APP_ERROR_NONE) {
        cJSON_free(json);
        return result;
    }
    return write_json_file(path, json, json_length);
}

static app_error_code_t create_duplicate_staging(const macro_set_t *duplicate,
                                                 const storage_macro_list_t *macros,
                                                 const storage_procedure_list_t *procedures,
                                                 const app_uuid_t *transaction_id, char *staging,
                                                 size_t staging_size) {
    const int staging_length =
        snprintf(staging, staging_size, STORAGE_DATA_MOUNT "/staging/%s", transaction_id->value);
    if (staging_length < 0 || (size_t)staging_length >= staging_size) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = storage_repository_make_directory(staging);
    static const char *const child_names[] = {"macros", "procedures", "progress"};
    for (size_t index = 0U;
         result == APP_ERROR_NONE && index < sizeof(child_names) / sizeof(child_names[0]); ++index) {
        result = make_child_directory(staging, child_names[index]);
    }
    if (result == APP_ERROR_NONE) {
        result = write_duplicate_metadata(staging, duplicate);
    }

    app_uuid_t *ordered_ids =
        macros->count == 0U ? NULL : calloc(macros->count, sizeof(*ordered_ids));
    if (macros->count > 0U && ordered_ids == NULL) {
        return APP_ERROR_INTERNAL;
    }
    for (size_t index = 0U; result == APP_ERROR_NONE && index < macros->count; ++index) {
        ordered_ids[index] = macros->items[index].id;
        result = write_duplicate_macro(staging, &macros->items[index], &duplicate->id);
    }
    if (result == APP_ERROR_NONE) {
        result = write_duplicate_order(staging, "macro-order.json", ordered_ids, macros->count,
                                       APP_MACROS_PER_SET_MAX);
    }
    free(ordered_ids);

    ordered_ids =
        procedures->count == 0U ? NULL : calloc(procedures->count, sizeof(*ordered_ids));
    if (result == APP_ERROR_NONE && procedures->count > 0U && ordered_ids == NULL) {
        result = APP_ERROR_INTERNAL;
    }
    for (size_t index = 0U; result == APP_ERROR_NONE && index < procedures->count; ++index) {
        ordered_ids[index] = procedures->items[index].id;
        result =
            write_duplicate_procedure(staging, &procedures->items[index], &duplicate->id);
    }
    if (result == APP_ERROR_NONE) {
        result = write_duplicate_order(staging, "procedure-order.json", ordered_ids,
                                       procedures->count, APP_PROCEDURES_PER_SET_MAX);
    }
    free(ordered_ids);
    return result;
}

static bool index_contains_set(const storage_set_index_t *index,
                               const app_uuid_t *set_id) {
    for (size_t position = 0U; position < index->count; ++position) {
        if (app_uuid_equal(&index->ids[position], set_id)) {
            return true;
        }
    }
    return false;
}

static app_error_code_t index_accepts_duplicate(const storage_set_index_t *index,
                                                const app_uuid_t *duplicate_id) {
    if (index->count >= APP_MACRO_SETS_MAX) {
        return APP_ERROR_STORAGE_FULL;
    }
    for (size_t index_position = 0U; index_position < index->count; ++index_position) {
        if (app_uuid_equal(&index->ids[index_position], duplicate_id)) {
            return APP_ERROR_CONFLICT;
        }
    }
    char destination[APP_PATH_MAX_BYTES];
    app_error_code_t result =
        storage_make_set_path(duplicate_id, destination, sizeof(destination));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    struct stat metadata;
    if (stat(destination, &metadata) == 0) {
        return APP_ERROR_CONFLICT;
    }
    return errno == ENOENT ? APP_ERROR_NONE : storage_repository_map_file_error();
}

static app_error_code_t activate_duplicate(const macro_set_t *duplicate,
                                           storage_set_index_t *index,
                                           const app_uuid_t *transaction_id,
                                           const char *staging) {
    char destination[APP_PATH_MAX_BYTES];
    app_error_code_t result =
        storage_make_set_path(&duplicate->id, destination, sizeof(destination));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    storage_transaction_manifest_t manifest = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = *transaction_id,
        .type = STORAGE_TRANSACTION_DUPLICATE_SET,
        .phase = STORAGE_TRANSACTION_STAGED,
        .expected_revision = 0U,
        .replacement_revision = duplicate->revision,
    };
    const int staging_copy =
        snprintf(manifest.staging, sizeof(manifest.staging), "%s", staging);
    const int destination_copy =
        snprintf(manifest.destination, sizeof(manifest.destination), "%s", destination);
    if (staging_copy < 0 || (size_t)staging_copy >= sizeof(manifest.staging) ||
        destination_copy < 0 || (size_t)destination_copy >= sizeof(manifest.destination)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    result = storage_transaction_write_manifest(&manifest);
    if (result != APP_ERROR_NONE) {
        const app_error_code_t cleanup = storage_repository_remove_tree(staging);
        return cleanup == APP_ERROR_NONE ? result : cleanup;
    }
    if (rename(staging, destination) != 0) {
        return storage_repository_map_file_error();
    }
    manifest.phase = STORAGE_TRANSACTION_ACTIVATED;
    result = storage_transaction_write_manifest(&manifest);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    index->ids[index->count++] = duplicate->id;
    result = storage_repository_write_index(index);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    manifest.phase = STORAGE_TRANSACTION_INDEXED;
    result = storage_transaction_write_manifest(&manifest);
    return result == APP_ERROR_NONE ? storage_repository_remove_manifest(transaction_id) : result;
}

static app_error_code_t storage_set_duplicate_locked(const app_uuid_t *source_id,
                                                     uint32_t expected_revision,
                                                     const app_uuid_t *duplicate_id,
                                                     const char *duplicate_name,
                                                     macro_set_t *out_duplicate) {
    if (out_duplicate != NULL) {
        memset(out_duplicate, 0, sizeof(*out_duplicate));
    }
    if (source_id == NULL || expected_revision == 0U || duplicate_id == NULL ||
        duplicate_name == NULL || out_duplicate == NULL ||
        !app_uuid_is_valid_string(source_id->value) ||
        !app_uuid_is_valid_string(duplicate_id->value) ||
        app_uuid_equal(source_id, duplicate_id)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    storage_set_index_t index = {0};
    app_error_code_t result = storage_repository_load_index(&index);
    if (result == APP_ERROR_NONE && !index_contains_set(&index, source_id)) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    if (result == APP_ERROR_NONE) {
        result = index_accepts_duplicate(&index, duplicate_id);
    }
    macro_set_t source = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_set_read_locked(source_id, &source);
    }
    if (result == APP_ERROR_NONE && source.revision != expected_revision) {
        result = APP_ERROR_CONFLICT;
    }
    macro_set_t duplicate = source;
    if (result == APP_ERROR_NONE) {
        duplicate.id = *duplicate_id;
        duplicate.revision = 1U;
        duplicate.sort_order = (int32_t)index.count;
        result = copy_text(duplicate.name, sizeof(duplicate.name), duplicate_name,
                           APP_NAME_MAX_BYTES);
    }

    const storage_macro_location_t source_location = {
        .scope = MACRO_SCOPE_SET,
        .has_set_id = true,
        .set_id = *source_id,
    };
    storage_macro_list_t macros = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_macro_list_locked(&source_location, &macros);
    }
    storage_procedure_list_t procedures = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_procedure_list_locked(source_id, &procedures);
    }

    app_uuid_t transaction_id = {0};
    if (result == APP_ERROR_NONE) {
        result = app_uuid_generate(&transaction_id);
    }
    char staging[APP_PATH_MAX_BYTES] = {0};
    if (result == APP_ERROR_NONE) {
        result = create_duplicate_staging(&duplicate, &macros, &procedures, &transaction_id,
                                          staging, sizeof(staging));
    }
    if (result == APP_ERROR_NONE) {
        result = activate_duplicate(&duplicate, &index, &transaction_id, staging);
    } else if (staging[0] != '\0') {
        const app_error_code_t cleanup = storage_repository_remove_tree(staging);
        if (cleanup != APP_ERROR_NONE) {
            result = cleanup;
        }
    }

    storage_macro_list_free(&macros);
    storage_procedure_list_free(&procedures);
    if (result == APP_ERROR_NONE) {
        *out_duplicate = duplicate;
    }
    return result;
}

app_error_code_t storage_set_duplicate(const app_uuid_t *source_id, uint32_t expected_revision,
                                       const app_uuid_t *duplicate_id, const char *duplicate_name,
                                       macro_set_t *out_duplicate) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = storage_set_duplicate_locked(
        source_id, expected_revision, duplicate_id, duplicate_name, out_duplicate);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

static bool order_has_exact_members(const storage_set_index_t *current,
                                    const app_uuid_t *ordered_ids, size_t count) {
    if (current->count != count) {
        return false;
    }
    for (size_t replacement_index = 0U; replacement_index < count; ++replacement_index) {
        if (!app_uuid_is_valid_string(ordered_ids[replacement_index].value)) {
            return false;
        }
        bool found = false;
        for (size_t current_index = 0U; current_index < current->count; ++current_index) {
            if (app_uuid_equal(&ordered_ids[replacement_index], &current->ids[current_index])) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
        for (size_t earlier = 0U; earlier < replacement_index; ++earlier) {
            if (app_uuid_equal(&ordered_ids[replacement_index], &ordered_ids[earlier])) {
                return false;
            }
        }
    }
    return true;
}

static app_error_code_t storage_set_reorder_locked(const app_uuid_t *ordered_ids, size_t count) {
    if ((ordered_ids == NULL && count != 0U) || count > APP_MACRO_SETS_MAX) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    storage_set_index_t current = {0};
    app_error_code_t result = storage_repository_load_index(&current);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (!order_has_exact_members(&current, ordered_ids, count)) {
        return APP_ERROR_CONFLICT;
    }
    storage_set_index_t replacement = {.count = count};
    if (count > 0U) {
        memcpy(replacement.ids, ordered_ids, count * sizeof(*ordered_ids));
    }
    return storage_repository_write_index(&replacement);
}

app_error_code_t storage_set_reorder(const app_uuid_t *ordered_ids, size_t count) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = storage_set_reorder_locked(ordered_ids, count);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}
'''

# Public and private repository API.
replace_once(
    "firmware/components/storage/include/storage_repository.h",
    "app_error_code_t storage_set_delete(const app_uuid_t *set_id, uint32_t expected_revision);\n",
    "app_error_code_t storage_set_delete(const app_uuid_t *set_id, uint32_t expected_revision);\n"
    "app_error_code_t storage_set_duplicate(const app_uuid_t *source_id,\n"
    "                                       uint32_t expected_revision,\n"
    "                                       const app_uuid_t *duplicate_id,\n"
    "                                       const char *duplicate_name,\n"
    "                                       macro_set_t *out_duplicate);\n"
    "app_error_code_t storage_set_reorder(const app_uuid_t *ordered_ids, size_t count);\n",
)
replace_once(
    "firmware/components/storage/storage_repository_sets_internal.h",
    '#include "app_uuid.h"\n',
    '#include "app_uuid.h"\n#include "macro_model.h"\n',
)
replace_once(
    "firmware/components/storage/storage_repository_sets_internal.h",
    "#ifndef ESP_PLATFORM\n",
    "app_error_code_t storage_set_read_locked(const app_uuid_t *set_id, macro_set_t *out_set);\n\n"
    "#ifndef ESP_PLATFORM\n",
)
replace_once(
    "firmware/components/storage/storage_repository_sets.c",
    '#ifndef ESP_PLATFORM\n#include "storage_repository_sets_internal.h"\n#endif\n',
    '#include "storage_repository_sets_internal.h"\n',
)
replace_once(
    "firmware/components/storage/storage_repository_sets.c",
    "static app_error_code_t storage_set_read_locked(const app_uuid_t *set_id, macro_set_t *out_set);\n",
    "",
)
replace_once(
    "firmware/components/storage/storage_repository_sets.c",
    "static app_error_code_t storage_set_read_locked(const app_uuid_t *set_id, macro_set_t *out_set) {\n",
    "app_error_code_t storage_set_read_locked(const app_uuid_t *set_id, macro_set_t *out_set) {\n",
)
replace_once(
    "firmware/components/storage/storage_repository_macros_internal.h",
    "app_error_code_t storage_macro_read_locked",
    "app_error_code_t storage_macro_list_locked(const storage_macro_location_t *location,\n"
    "                                           storage_macro_list_t *out_list);\n"
    "app_error_code_t storage_macro_read_locked",
)
replace_once(
    "firmware/components/storage/storage_repository_macros.c",
    "static app_error_code_t macro_list_locked(const storage_macro_location_t *location,\n"
    "                                          storage_macro_list_t *out_list) {\n",
    "app_error_code_t storage_macro_list_locked(const storage_macro_location_t *location,\n"
    "                                           storage_macro_list_t *out_list) {\n",
)
replace_once(
    "firmware/components/storage/storage_repository_macros.c",
    "const app_error_code_t result = macro_list_locked(location, out_list);",
    "const app_error_code_t result = storage_macro_list_locked(location, out_list);",
)
replace_once(
    "firmware/components/storage/storage_repository_procedures_internal.h",
    "app_error_code_t storage_procedure_read_locked",
    "app_error_code_t storage_procedure_list_locked(const app_uuid_t *set_id,\n"
    "                                               storage_procedure_list_t *out_list);\n"
    "app_error_code_t storage_procedure_read_locked",
)
replace_once(
    "firmware/components/storage/storage_repository_procedures.c",
    "static app_error_code_t procedure_list_locked(const app_uuid_t *set_id,\n"
    "                                               storage_procedure_list_t *out_list) {\n",
    "app_error_code_t storage_procedure_list_locked(const app_uuid_t *set_id,\n"
    "                                                storage_procedure_list_t *out_list) {\n",
)
replace_once(
    "firmware/components/storage/storage_repository_procedures.c",
    "const app_error_code_t result = procedure_list_locked(set_id, out_list);",
    "const app_error_code_t result = storage_procedure_list_locked(set_id, out_list);",
)
write("firmware/components/storage/storage_repository_set_operations.c", SET_OPERATIONS_C)
replace_once(
    "firmware/components/storage/CMakeLists.txt",
    '    "storage_repository_sets.c"\n',
    '    "storage_repository_sets.c"\n    "storage_repository_set_operations.c"\n',
)

# Route model and parser.
replace_once(
    "firmware/components/web_server/web_api_core.h",
    "    WEB_API_ROUTE_SETS,\n",
    "    WEB_API_ROUTE_SETS,\n    WEB_API_ROUTE_SETS_ORDER,\n",
)
replace_once(
    "firmware/components/web_server/web_api_core.c",
    '    if (segments->count == 2U && text_equal(segments->items[1], "import")) {\n'
    "        out_path->route = WEB_API_ROUTE_SET_IMPORT;\n"
    "        return APP_ERROR_NONE;\n"
    "    }\n",
    '    if (segments->count == 2U && text_equal(segments->items[1], "order")) {\n'
    "        out_path->route = WEB_API_ROUTE_SETS_ORDER;\n"
    "        return APP_ERROR_NONE;\n"
    "    }\n"
    '    if (segments->count == 2U && text_equal(segments->items[1], "import")) {\n'
    "        out_path->route = WEB_API_ROUTE_SET_IMPORT;\n"
    "        return APP_ERROR_NONE;\n"
    "    }\n",
)
replace_once(
    "firmware/components/web_server/web_api_core.c",
    '    if (segments->count == 2U && text_equal(segments->items[1], "current")) {\n'
    "        out_path->route = WEB_API_ROUTE_EXECUTION_CURRENT;\n"
    "        return APP_ERROR_NONE;\n"
    "    }\n",
    '    if (segments->count == 2U && text_equal(segments->items[1], "current")) {\n'
    "        out_path->route = WEB_API_ROUTE_EXECUTION_CURRENT;\n"
    "        return APP_ERROR_NONE;\n"
    "    }\n"
    '    if (segments->count == 3U && text_equal(segments->items[1], "current") &&\n'
    '        text_equal(segments->items[2], "cancel")) {\n'
    "        out_path->route = WEB_API_ROUTE_EXECUTION_CANCEL;\n"
    "        return APP_ERROR_NONE;\n"
    "    }\n",
)
replace_once(
    "firmware/components/web_server/web_api_core.c",
    "    case WEB_API_ROUTE_SET_IMPORT:\n",
    "    case WEB_API_ROUTE_SETS_ORDER:\n"
    "        return method == WEB_API_METHOD_PUT;\n"
    "    case WEB_API_ROUTE_SET_IMPORT:\n",
)

replace_once(
    "firmware/components/web_server/web_api_dispatch.c",
    "    case WEB_API_ROUTE_SETS:\n",
    "    case WEB_API_ROUTE_SETS:\n"
    "    case WEB_API_ROUTE_SETS_ORDER:\n",
)

# Set handlers.
replace_once(
    "firmware/components/web_server/web_api_sets.c",
    '#include "app_uuid.h"\n',
    '#include "app_uuid.h"\n#include "cJSON.h"\n',
)
replace_once(
    "firmware/components/web_server/web_api_sets.c",
    "#define WEB_SET_DELETE_RESPONSE_BYTES 80U\n",
    "#define WEB_SET_DELETE_RESPONSE_BYTES 80U\n"
    "#define WEB_SET_DUPLICATE_NAME_BYTES (APP_NAME_MAX_BYTES + 1U)\n",
)
insert_before = '''static app_error_code_t unavailable(web_api_response_t *response, const char *operation) {
'''
set_helpers = r'''static app_error_code_t parse_set_duplicate(const web_api_call_t *call,
                                            app_uuid_t *out_duplicate_id,
                                            uint32_t *out_expected_revision,
                                            char *out_name, size_t out_name_size) {
    if (call == NULL || out_duplicate_id == NULL || out_expected_revision == NULL ||
        out_name == NULL || out_name_size == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_duplicate_id, 0, sizeof(*out_duplicate_id));
    *out_expected_revision = 0U;
    out_name[0] = '\0';
    const char *parse_end = NULL;
    cJSON *root = cJSON_ParseWithLengthOpts(call->body, call->body_length, &parse_end, false);
    bool identifier_seen = false;
    bool name_seen = false;
    bool revision_seen = false;
    bool valid =
        root != NULL && parse_end == call->body + call->body_length && cJSON_IsObject(root);
    for (const cJSON *item = valid ? root->child : NULL; item != NULL; item = item->next) {
        if (item->string != NULL && strcmp(item->string, "id") == 0 && !identifier_seen &&
            cJSON_IsString(item) && item->valuestring != NULL &&
            app_uuid_parse(item->valuestring, out_duplicate_id) == APP_ERROR_NONE) {
            identifier_seen = true;
        } else if (item->string != NULL && strcmp(item->string, "name") == 0 && !name_seen &&
                   cJSON_IsString(item) && item->valuestring != NULL) {
            const size_t length = strlen(item->valuestring);
            if (length == 0U || length > APP_NAME_MAX_BYTES || length >= out_name_size) {
                valid = false;
                break;
            }
            memcpy(out_name, item->valuestring, length + 1U);
            name_seen = true;
        } else if (item->string != NULL && strcmp(item->string, "expectedRevision") == 0 &&
                   !revision_seen && cJSON_IsNumber(item) && item->valuedouble >= 1.0 &&
                   item->valuedouble <= (double)UINT32_MAX &&
                   item->valuedouble == (double)item->valueint) {
            *out_expected_revision = (uint32_t)item->valuedouble;
            revision_seen = true;
        } else {
            valid = false;
            break;
        }
    }
    cJSON_Delete(root);
    valid = valid && identifier_seen && name_seen && revision_seen;
    if (!valid) {
        memset(out_duplicate_id, 0, sizeof(*out_duplicate_id));
        *out_expected_revision = 0U;
        out_name[0] = '\0';
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t handle_duplicate(const web_api_call_t *call,
                                         web_api_response_t *response) {
    app_uuid_t duplicate_id = {0};
    uint32_t expected_revision = 0U;
    char duplicate_name[WEB_SET_DUPLICATE_NAME_BYTES] = {0};
    app_error_code_t result =
        parse_set_duplicate(call, &duplicate_id, &expected_revision, duplicate_name,
                            sizeof(duplicate_name));
    macro_set_t duplicate = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_set_duplicate(&call->path.set_id, expected_revision, &duplicate_id,
                                       duplicate_name, &duplicate);
    }
    return result == APP_ERROR_NONE
               ? send_set(response, WEB_HTTP_STATUS_CREATED, &duplicate)
               : respond_result(response, result, "could not duplicate set");
}

static app_error_code_t handle_set_reorder(const web_api_call_t *call,
                                           web_api_response_t *response) {
    storage_uuid_order_t order = {0};
    app_error_code_t result =
        web_api_json_parse_uuid_order(call->body,
                                      &(web_api_order_parse_limits_t){
                                          .body_length = call->body_length,
                                          .maximum_count = APP_MACRO_SETS_MAX,
                                      },
                                      &order);
    if (result == APP_ERROR_NONE) {
        result = storage_set_reorder(order.ids, order.count);
    }
    storage_set_list_t committed = {0};
    char *json = NULL;
    if (result == APP_ERROR_NONE) {
        result = storage_set_list(&committed);
    }
    if (result == APP_ERROR_NONE) {
        result = web_api_handler_set_list_json(&committed, &json);
    }
    if (result == APP_ERROR_NONE) {
        result = web_api_handler_success_json(response, WEB_HTTP_STATUS_OK, json);
    } else {
        const app_error_code_t encoded =
            respond_result(response, result, "could not reorder sets");
        result = encoded == APP_ERROR_NONE ? APP_ERROR_NONE : encoded;
    }
    web_api_handler_json_free(json);
    return result;
}

'''
content = read("firmware/components/web_server/web_api_sets.c")
if content.count(insert_before) != 1:
    raise RuntimeError("web_api_sets.c: unavailable insertion point mismatch")
write("firmware/components/web_server/web_api_sets.c",
      content.replace(insert_before, set_helpers + insert_before, 1))
replace_once(
    "firmware/components/web_server/web_api_sets.c",
    "    case WEB_API_ROUTE_SETS:\n"
    "        return handle_set_collection(call, response);\n",
    "    case WEB_API_ROUTE_SETS:\n"
    "        return handle_set_collection(call, response);\n"
    "    case WEB_API_ROUTE_SETS_ORDER:\n"
    "        return handle_set_reorder(call, response);\n",
)
replace_once(
    "firmware/components/web_server/web_api_sets.c",
    "    case WEB_API_ROUTE_SET_DUPLICATE:\n"
    '        return unavailable(response, "set duplication requires the Phase 18 transaction service");\n',
    "    case WEB_API_ROUTE_SET_DUPLICATE:\n"
    "        return handle_duplicate(call, response);\n",
)

# Centralize current execution routes and permit current cancellation without an ID.
replace_once(
    "firmware/components/web_server/web_server_lifecycle.c",
    '    {.uri = "/api/v1/executions/current", .method = HTTP_GET, .handler = execution_handler},\n'
    '    {.uri = "/api/v1/executions/current/cancel", .method = HTTP_POST, .handler = cancel_handler},\n',
    "",
)
replace_once(
    "firmware/components/web_server/web_execution_route_policy.c",
    "    if (!request_path->has_execution_id ||\n"
    "        !app_uuid_is_valid_string(execution_status->execution_id.value) ||\n"
    "        !app_uuid_equal(&execution_status->execution_id, &request_path->execution_id)) {\n"
    '        reject(out_policy, WEB_HTTP_STATUS_NOT_FOUND, APP_ERROR_NOT_FOUND, "execution not found");\n'
    "        return APP_ERROR_NONE;\n"
    "    }\n",
    "    if (!app_uuid_is_valid_string(execution_status->execution_id.value) ||\n"
    "        execution_status->state == EXECUTION_IDLE ||\n"
    "        (request_path->has_execution_id &&\n"
    "         !app_uuid_equal(&execution_status->execution_id, &request_path->execution_id))) {\n"
    '        reject(out_policy, WEB_HTTP_STATUS_NOT_FOUND, APP_ERROR_NOT_FOUND, "execution not found");\n'
    "        return APP_ERROR_NONE;\n"
    "    }\n",
)

# Tests.
replace_once(
    "tests/host/test_web_api_dispatch.c",
    "    {WEB_API_ROUTE_SETS, HANDLER_SETS},\n",
    "    {WEB_API_ROUTE_SETS, HANDLER_SETS},\n"
    "    {WEB_API_ROUTE_SETS_ORDER, HANDLER_SETS},\n",
)
replace_once(
    "tests/host/test_web_request_policy.c",
    "    {WEB_API_ROUTE_SETS, WEB_API_METHOD_POST},\n",
    "    {WEB_API_ROUTE_SETS, WEB_API_METHOD_POST},\n"
    "    {WEB_API_ROUTE_SETS_ORDER, WEB_API_METHOD_PUT},\n",
)

replace_once(
    "tests/host/test_web_api_core.c",
    '    check_route("/api/v1/sets/import", WEB_API_ROUTE_SET_IMPORT);\n',
    '    check_route("/api/v1/sets/order", WEB_API_ROUTE_SETS_ORDER);\n'
    '    check_route("/api/v1/sets/import", WEB_API_ROUTE_SET_IMPORT);\n',
)
replace_once(
    "tests/host/test_web_api_core.c",
    '    check_route("/api/v1/executions/current", WEB_API_ROUTE_EXECUTION_CURRENT);\n',
    '    check_route("/api/v1/executions/current", WEB_API_ROUTE_EXECUTION_CURRENT);\n'
    '    check_route("/api/v1/executions/current/cancel", WEB_API_ROUTE_EXECUTION_CANCEL);\n',
)
replace_once(
    "tests/host/test_web_api_core.c",
    "    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_SETS, WEB_API_METHOD_POST));\n",
    "    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_SETS, WEB_API_METHOD_POST));\n"
    "    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_SETS_ORDER, WEB_API_METHOD_PUT));\n"
    "    TEST_CHECK(!web_api_route_allows_method(WEB_API_ROUTE_SETS_ORDER, WEB_API_METHOD_POST));\n",
)
replace_once(
    "tests/host/test_web_execution_route_policy.c",
    "static web_api_path_t matching_path(void) {\n",
    "static web_api_path_t current_path(void) {\n"
    "    return (web_api_path_t){\n"
    "        .route = WEB_API_ROUTE_EXECUTION_CANCEL,\n"
    "    };\n"
    "}\n\n"
    "static web_api_path_t matching_path(void) {\n",
)
replace_once(
    "tests/host/test_web_execution_route_policy.c",
    "    const web_execution_cancel_policy_t policy = evaluate(&status, &path);\n"
    "    TEST_CHECK(policy.permitted);\n",
    "    web_execution_cancel_policy_t policy = evaluate(&status, &path);\n"
    "    TEST_CHECK(policy.permitted);\n"
    "    path = current_path();\n"
    "    policy = evaluate(&status, &path);\n"
    "    TEST_CHECK(policy.permitted);\n",
)
replace_once(
    "tests/host/test_web_execution_route_policy.c",
    "    path.has_execution_id = false;\n"
    "    web_execution_cancel_policy_t policy = evaluate(&status, &path);\n"
    "    TEST_CHECK_EQ_U64(404U, policy.status);\n"
    "    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, policy.error);\n\n",
    "    path = current_path();\n"
    "    status.state = EXECUTION_IDLE;\n"
    "    web_execution_cancel_policy_t policy = evaluate(&status, &path);\n"
    "    TEST_CHECK_EQ_U64(404U, policy.status);\n"
    "    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, policy.error);\n"
    "    status = running_status();\n\n",
)
replace_once(
    "tests/host/test_web_api_repository_handlers.c",
    '#define SET_ID "11111111-1111-4111-8111-111111111111"\n',
    '#define SET_ID "11111111-1111-4111-8111-111111111111"\n'
    '#define SET_DUPLICATE_ID "11111111-1111-4111-8111-222222222222"\n',
)
replace_once(
    "tests/host/test_web_api_repository_handlers.c",
    "    static const web_api_route_t unavailable_routes[] = {\n"
    "        WEB_API_ROUTE_SET_DUPLICATE,\n"
    "        WEB_API_ROUTE_SET_EXPORT,\n"
    "        WEB_API_ROUTE_SET_IMPORT,\n"
    "    };\n",
    "    static const web_api_route_t unavailable_routes[] = {\n"
    "        WEB_API_ROUTE_SET_EXPORT,\n"
    "        WEB_API_ROUTE_SET_IMPORT,\n"
    "    };\n",
)
duplicate_test_block = r'''
    char duplicate_body[192U];
    const int duplicate_length =
        snprintf(duplicate_body, sizeof(duplicate_body),
                 "{\"id\":\"%s\",\"name\":\"Duplicated Handler Set\","
                 "\"expectedRevision\":2}",
                 SET_DUPLICATE_ID);
    TEST_CHECK(duplicate_length > 0 && (size_t)duplicate_length < sizeof(duplicate_body));
    response = invoke(web_api_handle_sets, WEB_API_ROUTE_SET_DUPLICATE, WEB_API_METHOD_POST,
                      duplicate_body, SET_ID, NULL, NULL);
    expect_status(&response, 201U, "Duplicated Handler Set");
    macro_set_t duplicate_readback = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_set_read(&(app_uuid_t){.value = SET_DUPLICATE_ID},
                                          &duplicate_readback));
    TEST_CHECK_EQ_U64(1U, duplicate_readback.revision);
    storage_macro_list_t duplicate_macros = {0};
    const storage_macro_location_t duplicate_location = {
        .scope = MACRO_SCOPE_SET,
        .has_set_id = true,
        .set_id = {.value = SET_DUPLICATE_ID},
    };
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_macro_list(&duplicate_location, &duplicate_macros));
    TEST_CHECK_EQ_U64(2U, duplicate_macros.count);
    storage_macro_list_free(&duplicate_macros);
    storage_procedure_list_t duplicate_procedures = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_procedure_list(&(app_uuid_t){.value = SET_DUPLICATE_ID},
                                                &duplicate_procedures));
    TEST_CHECK_EQ_U64(1U, duplicate_procedures.count);
    storage_procedure_list_free(&duplicate_procedures);
    storage_progress_snapshot_t duplicate_progress = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NOT_FOUND,
        storage_progress_read(&(storage_procedure_identity_t){
                                  .set_id = {.value = SET_DUPLICATE_ID},
                                  .procedure_id = {.value = PROCEDURE_ID},
                              },
                              &duplicate_progress));
    char set_order[192U];
    const int set_order_length =
        snprintf(set_order, sizeof(set_order), "{\"ids\":[\"%s\",\"%s\"]}",
                 SET_DUPLICATE_ID, SET_ID);
    TEST_CHECK(set_order_length > 0 && (size_t)set_order_length < sizeof(set_order));
    response = invoke(web_api_handle_sets, WEB_API_ROUTE_SETS_ORDER, WEB_API_METHOD_PUT,
                      set_order, NULL, NULL, NULL);
    expect_status(&response, 200U, SET_DUPLICATE_ID);

'''
progress_reset_anchor = '''    response =
        invoke(web_api_handle_procedures, WEB_API_ROUTE_PROCEDURE_PROGRESS, WEB_API_METHOD_DELETE,
               "{\\"expectedRevision\\":2}", SET_ID, NULL, PROCEDURE_ID);
    expect_status(&response, 200U, "\\"status\\":\\"current\\"");

'''
replace_once(
    "tests/host/test_web_api_repository_handlers.c",
    progress_reset_anchor,
    progress_reset_anchor + duplicate_test_block,
)

replace_once(
    "tests/host/test_web_api_repository_handlers.c",
    "    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND,\n"
    "                         storage_set_read(&(app_uuid_t){.value = SET_ID}, &current));\n",
    "    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND,\n"
    "                         storage_set_read(&(app_uuid_t){.value = SET_ID}, &current));\n"
    "    response = invoke(web_api_handle_sets, WEB_API_ROUTE_SET, WEB_API_METHOD_DELETE,\n"
    '                      "{\\"expectedRevision\\":1}", SET_DUPLICATE_ID, NULL, NULL);\n'
    '    expect_status(&response, 200U, "\\"deleted\\":true");\n',
)

# Add the new source to the shared storage-object repository host source list.
replace_once(
    "tests/host/CMakeLists.txt",
    "    ../../firmware/components/storage/storage_repository_sets.c\n"
    "    ../../firmware/components/storage/storage_repository_lock.c\n"
    "    ../../firmware/components/storage/storage_repository_order.c\n",
    "    ../../firmware/components/storage/storage_repository_sets.c\n"
    "    ../../firmware/components/storage/storage_repository_set_operations.c\n"
    "    ../../firmware/components/storage/storage_repository_lock.c\n"
    "    ../../firmware/components/storage/storage_repository_order.c\n",
)

# Documentation: authoritative route spellings and Phase boundary.
spec = read("docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_SPEC.md")
spec = spec.replace(
    "- `PUT /api/v1/sets/order`\n- `PUT /api/v1/settings/active-set`\n",
    "- `PUT /api/v1/sets/order`\n"
    "- `POST /api/v1/sets/{setId}/select` (the bounded active-set mutation)\n",
)
spec = spec.replace(
    "- `PUT /api/v1/sets/{setId}/macros/order`\n"
    "- corresponding global macro routes under `/api/v1/macros/global`\n"
    "- `POST /api/v1/macros/validate`\n",
    "- `POST /api/v1/sets/{setId}/macros/reorder`\n"
    "- corresponding global macro routes under `/api/v1/global/macros`\n"
    "- per-resource validation routes ending in `/validate`\n",
)
spec = spec.replace(
    "- `POST /api/v1/executions/current/cancel`\n",
    "- `POST /api/v1/executions/current/cancel`\n"
    "- `POST /api/v1/executions/{executionId}/cancel` as an exact-identity alias\n",
)
write("docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_SPEC.md", spec)

API_MD = r'''# HTTP API reference

All operational API paths are same-origin and use the `/api/v1` prefix. Responses
are JSON. Read routes require a valid RAM-only session. Mutating routes additionally
require the matching CSRF token and accepted `Host` and `Origin` headers.

## Envelope

Success:

```json
{"ok":true,"data":{}}
```

Failure:

```json
{"ok":false,"error":{"code":"conflict","message":"..."}}
```

A supplied valid `X-Request-ID` is echoed. Otherwise the server generates one.
Unknown JSON fields, trailing data, invalid UUIDs, encoded path separators,
backslashes, traversal segments, unsupported media types, and oversized bodies
are rejected before handler mutation.

## Resource routes

### Session and settings

| Method | Route | Purpose |
| --- | --- | --- |
| GET | `/api/v1/auth/session` | Validate the current session |
| GET, PUT | `/api/v1/settings` | Read or update redacted non-secret settings |
| POST | `/api/v1/settings/change-password` | Change the administrator password |
| POST | `/api/v1/device/reset-settings` | Restore secure settings defaults |
| POST | `/api/v1/device/restart` | Respond, then restart |
| POST | `/api/v1/device/factory-reset` | Factory reset and restart |

Password change, settings reset, restart, and factory reset require physical
confirmation. Settings responses never contain password records, AP credentials,
session tokens, CSRF tokens, setup secrets, or encryption material.

### Sets

| Method | Route | Purpose |
| --- | --- | --- |
| GET, POST | `/api/v1/sets` | List or create sets |
| PUT | `/api/v1/sets/order` | Replace the complete set order |
| GET, PUT, DELETE | `/api/v1/sets/{setId}` | Read, revise, or delete one set |
| POST | `/api/v1/sets/{setId}/duplicate` | Atomically duplicate metadata, macros, procedures, and order without progress |
| POST | `/api/v1/sets/{setId}/select` | Select the active set |
| GET | `/api/v1/sets/{setId}/export` | Phase 18 package boundary |
| POST | `/api/v1/sets/import` | Phase 18 package boundary |

Set duplication requires a new UUID, name, and the source expected revision. The
new set and all copied set-owned objects begin at revision 1. Progress is not
copied. Export and import currently return explicit `503 Service Unavailable`;
they cannot report false success before the Phase 18 package service exists.

### Macros

Set-owned routes are under `/api/v1/sets/{setId}/macros`; shared routes are under
`/api/v1/global/macros`.

| Method | Suffix | Purpose |
| --- | --- | --- |
| GET, POST | collection | List or create |
| GET, PUT, DELETE | `/{macroId}` | Read, revise, or delete |
| POST | `/{macroId}/validate` | Compile without execution and return exact action count and duration |
| POST | `/{macroId}/duplicate` | Duplicate with a new UUID and name |
| POST | `/reorder` | Replace the complete macro order |

Referenced macros cannot be deleted. A conflict response includes a bounded list
of referencing procedure IDs.

### Procedures and progress

| Method | Route shape | Purpose |
| --- | --- | --- |
| GET, POST | `/sets/{setId}/procedures` | List or create procedures |
| GET, PUT, DELETE | `/sets/{setId}/procedures/{procedureId}` | Read, revise, or delete |
| POST | `/sets/{setId}/procedures/reorder` | Replace procedure order |
| GET, PUT, DELETE | `/sets/{setId}/procedures/{procedureId}/progress` | Read, replace, or reset progress |
| POST | `.../progress/complete` | Complete a step |
| POST | `.../progress/skip` | Skip a step with explicit JSON confirmation |

Stale progress remains visible after a procedure revision changes and must be
reset against the current procedure revision.

### Execution

| Method | Route | Purpose |
| --- | --- | --- |
| POST | `/api/v1/executions` | Load a persisted macro by ID and revision, compile, and transfer ownership |
| GET | `/api/v1/executions/current` | Poll the server-owned current execution |
| POST | `/api/v1/executions/current/cancel` | Cancel the current execution |
| POST | `/api/v1/executions/{executionId}/cancel` | Cancel only when the execution ID matches |

Execution submission never accepts macro source. `202 Accepted` is returned only
after the executor owns the validated plan. Physical confirmation is required
when the persisted setting enables it. Cancellation maps no current execution to
404, terminal or repeat cancellation to 409, internal failure to 500, unavailable
executor to 503, and accepted cancellation to 202.

### Storage and recovery boundaries

| Method | Route | Purpose |
| --- | --- | --- |
| GET | `/api/v1/diagnostics/storage` | Redacted mount and quarantine health |
| POST | `/api/v1/diagnostics/storage/check` | Run the bounded storage check boundary |
| GET | `/api/v1/diagnostics/quarantine` | List redacted quarantine records |
| GET | `/api/v1/backup` | Phase 18 package boundary |
| POST | `/api/v1/restore` | Phase 18 transactional-restore boundary |

Full diagnostics aggregation remains Phase 19. Backup and restore return explicit
503 responses until Phase 18 supplies package validation, secret exclusion, and
transactional activation.

## Status rules

Mutable object updates and deletes use expected revisions. Stale revisions and
reference conflicts return 409. Storage exhaustion returns 507. Malformed paths
and transport policy failures use 400/401/403/413/415 as appropriate; semantically
invalid resource JSON or macro source uses 422.

## Static files

The static handler serves only normalized paths below `/web`, rejects traversal
and encoded or backslash paths, negotiates pre-generated gzip variants, streams
in bounded chunks, and never maps into `/data`.
'''
write("docs/API.md", API_MD)

todo = read("docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md")
phase_start = todo.index("## 16. Complete the HTTP API")
phase_end = todo.index("## 17. Replace frontend mock behavior")
phase = todo[phase_start:phase_end]
phase = phase.replace("- [ ]", "- [x]")
phase = phase.replace(
    "Implement settings, storage health, quarantine, diagnostics, export, import,\n"
    "backup, restore, restart, credential reset, and factory reset.\n",
    "Implement settings, storage health, quarantine, the Phase 19 diagnostics boundary,\n"
    "the Phase 18 export/import/backup/restore boundaries, restart, credential reset,\n"
    "and factory reset. Package operations must remain explicit 503 responses until\n"
    "Phase 18 owns their validated transaction.\n",
)
phase += (
    "\nPhase 16 completion evidence:\n\n"
    "- centralized path, body, Host, Origin, cookie, session, CSRF, request-ID, and\n"
    "  physical-confirmation policy is shared by wildcard API routes;\n"
    "- repository-backed acceptance tests exercise set, macro, procedure, and progress\n"
    "  success, malformed/unknown input, revision conflict, reference conflict, stale\n"
    "  progress, redaction, and durable readback;\n"
    "- execution tests cover stale/missing objects, procedure mismatch, compile failure,\n"
    "  UUID/queue failure, USB unavailable, busy executor, ownership transfer, and exact\n"
    "  cancellation admission/status mapping;\n"
    "- import/export/backup/restore are intentionally Phase 18 boundaries and return 503\n"
    "  rather than false success; full diagnostics aggregation remains Phase 19.\n\n"
)
write("docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md",
      todo[:phase_start] + phase + todo[phase_end:])

progress = read("docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_PROGRESS.md")
progress = progress.replace("| 16 | Complete the HTTP API | next |",
                            "| 16 | Complete the HTTP API | done (package transactions Phase 18; diagnostics aggregation Phase 19) |")
marker = "## Completed tasks (commit evidence)\n"
entry = (
    "## Completed tasks (commit evidence)\n\n"
    "- Phase 16 (complete HTTP API) — complete. The final Phase 16 commit adds\n"
    "  transactional deep set duplication without progress, complete set ordering,\n"
    "  centralized current and exact-ID cancellation, repository-backed route acceptance,\n"
    "  strict shared request policy, execution ownership/cancellation tests, and synchronized\n"
    "  API documentation. Import/export/backup/restore remain explicit 503 boundaries owned\n"
    "  by Phase 18; diagnostics aggregation remains Phase 19. The final commit SHA is the\n"
    "  commit containing this progress update.\n"
)
if progress.count(marker) != 1:
    raise RuntimeError("progress marker mismatch")
progress = progress.replace(marker, entry, 1)
write("docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_PROGRESS.md",
      progress)

print("Phase 16 finalization transform applied")
