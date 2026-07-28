#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def write(path: str, content: str) -> None:
    (ROOT / path).write_text(content, encoding="utf-8")


def replace_once(text: str, old: str, new: str, path: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{path}: expected one occurrence, found {count}: {old[:100]!r}")
    return text.replace(old, new, 1)


def sub_once(text: str, pattern: str, replacement: str, path: str) -> str:
    updated, count = re.subn(pattern, replacement, text, count=1, flags=re.MULTILINE)
    if count != 1:
        raise RuntimeError(f"{path}: expected one regex occurrence, found {count}: {pattern[:100]!r}")
    return updated


# storage_paths.c: descriptive identifier.
path = "firmware/components/storage/storage_paths.c"
text = read(path)
text = sub_once(
    text,
    r"static bool valid_path_argument\(const app_uuid_t \*id, const char \*buffer, size_t buffer_size\) \{\n"
    r"\s+return id != NULL && buffer != NULL && buffer_size > 0U && app_uuid_is_valid_string\(id->value\);\n"
    r"\}",
    "static bool valid_path_argument(const app_uuid_t *object_id, const char *buffer,\n"
    "                                size_t buffer_size) {\n"
    "    return object_id != NULL && buffer != NULL && buffer_size > 0U &&\n"
    "           app_uuid_is_valid_string(object_id->value);\n"
    "}",
    path,
)
write(path, text)

# storage_json.c: directly include the defining header.
path = "firmware/components/storage/storage_json.c"
text = read(path)
text = replace_once(text, '#include "cJSON.h"\n', '#include "cJSON.h"\n#include "macro_limits.h"\n', path)
write(path, text)

# Order helper API: descriptive UUID parameter names.
for path in (
    "firmware/components/storage/storage_repository_order.h",
    "firmware/components/storage/storage_repository_order.c",
):
    text = read(path)
    text = re.sub(r"\bconst app_uuid_t \*id\b", "const app_uuid_t *item_id", text)
    text = re.sub(r"\bid\b", "item_id", text)
    write(path, text)

# Procedure helpers: remove adjacent same-type UUID parameters.
path = "firmware/components/storage/storage_repository_procedures.c"
text = read(path)
text = sub_once(
    text,
    r"static app_error_code_t validate_macro_reference_locked\(const app_uuid_t \*set_id,\n"
    r"\s+const app_uuid_t \*macro_id\) \{\n"
    r"\s+const storage_macro_location_t set_location = \{\n"
    r"\s+\.scope = MACRO_SCOPE_SET,\n"
    r"\s+\.has_set_id = true,\n"
    r"\s+\.set_id = \*set_id,\n"
    r"\s+\};\n"
    r"\s+const storage_macro_location_t global_location = \{",
    "static app_error_code_t validate_macro_reference_locked(\n"
    "    const storage_macro_location_t *set_location, const app_uuid_t *macro_id) {\n"
    "    const storage_macro_location_t global_location = {",
    path,
)
text = replace_once(
    text,
    "read_macro_candidate(&set_location, macro_id, &set_exists)",
    "read_macro_candidate(set_location, macro_id, &set_exists)",
    path,
)
text = sub_once(
    text,
    r"static app_error_code_t validate_procedure_references_locked\(const procedure_t \*procedure\) \{\n",
    "static app_error_code_t validate_procedure_references_locked(const procedure_t *procedure) {\n"
    "    const storage_macro_location_t set_location = {\n"
    "        .scope = MACRO_SCOPE_SET,\n"
    "        .has_set_id = true,\n"
    "        .set_id = procedure->set_id,\n"
    "    };\n",
    path,
)
text = replace_once(
    text,
    "validate_macro_reference_locked(&procedure->set_id, &step->macro_id)",
    "validate_macro_reference_locked(&set_location, &step->macro_id)",
    path,
)
text = sub_once(
    text,
    r"static app_error_code_t procedure_order_index\(const app_uuid_t \*set_id,\n"
    r"\s+const app_uuid_t \*procedure_id, size_t \*out_index\) \{",
    "static app_error_code_t procedure_order_index(const app_uuid_t *set_id, size_t *out_index,\n"
    "                                               const app_uuid_t *procedure_id) {",
    path,
)
text = replace_once(
    text,
    "procedure_order_index(set_id, procedure_id, &index)",
    "procedure_order_index(set_id, &index, procedure_id)",
    path,
)
write(path, text)

# Macro reference scan: split entry parsing/loading from directory traversal.
path = "firmware/components/storage/storage_repository_macros.c"
text = read(path)
start = text.index("static app_error_code_t scan_set_procedure_references(")
end = text.index("\nstatic app_error_code_t find_macro_references(", start)
replacement = r'''static const char PROCEDURE_JSON_SUFFIX[] = ".json";

typedef struct {
    const app_uuid_t *set_id;
    const app_uuid_t *macro_id;
    storage_reference_list_t *references;
} procedure_reference_scan_t;

static app_error_code_t parse_procedure_filename(const char *filename, bool *out_matches,
                                                  app_uuid_t *out_procedure_id) {
    *out_matches = false;
    const size_t suffix_length = sizeof(PROCEDURE_JSON_SUFFIX) - 1U;
    const size_t name_length = strlen(filename);
    if (name_length != APP_UUID_STRING_LENGTH + suffix_length ||
        strcmp(filename + APP_UUID_STRING_LENGTH, PROCEDURE_JSON_SUFFIX) != 0) {
        return APP_ERROR_NONE;
    }
    char uuid_text[APP_UUID_BUFFER_LENGTH];
    memcpy(uuid_text, filename, APP_UUID_STRING_LENGTH);
    uuid_text[APP_UUID_STRING_LENGTH] = '\0';
    if (app_uuid_parse(uuid_text, out_procedure_id) != APP_ERROR_NONE) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    *out_matches = true;
    return APP_ERROR_NONE;
}

static app_error_code_t read_reference_scan_procedure(const procedure_reference_scan_t *scan,
                                                       const app_uuid_t *procedure_id,
                                                       procedure_t *out_procedure) {
    char path[APP_PATH_MAX_BYTES];
    app_error_code_t result =
        storage_make_procedure_path(scan->set_id, procedure_id, path, sizeof(path));
    char *data = NULL;
    size_t length = 0U;
    if (result == APP_ERROR_NONE) {
        result = storage_repository_read_bounded_file(path, STORAGE_PROCEDURE_FILE_MAX_BYTES, &data,
                                                      &length);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_repository_parse_procedure_json(data, length, out_procedure);
    }
    free(data);
    if (result != APP_ERROR_STORAGE_CORRUPT) {
        return result;
    }
    storage_quarantine_entry_t quarantine_entry = {0};
    const app_error_code_t quarantine = storage_quarantine_file_locked(
        path, "invalid procedure during reference scan", &quarantine_entry);
    return quarantine == APP_ERROR_NONE ? result : quarantine;
}

static bool procedure_references_macro(const procedure_t *procedure, const app_uuid_t *macro_id) {
    for (size_t step_index = 0U; step_index < procedure->step_count; ++step_index) {
        if (step_references_macro(&procedure->steps[step_index], macro_id)) {
            return true;
        }
    }
    return false;
}

static app_error_code_t scan_procedure_entry(const procedure_reference_scan_t *scan,
                                             const struct dirent *entry) {
    app_uuid_t procedure_id = {0};
    bool matches = false;
    app_error_code_t result = parse_procedure_filename(entry->d_name, &matches, &procedure_id);
    if (result != APP_ERROR_NONE || !matches) {
        return result;
    }
    procedure_t procedure = {0};
    result = read_reference_scan_procedure(scan, &procedure_id, &procedure);
    if (result == APP_ERROR_NONE && procedure_references_macro(&procedure, scan->macro_id)) {
        add_reference(scan->references, &procedure.id);
    }
    macro_model_free_procedure(&procedure);
    return result;
}

static app_error_code_t scan_set_procedure_references(const procedure_reference_scan_t *scan) {
    char directory_path[APP_PATH_MAX_BYTES];
    const int written = snprintf(directory_path, sizeof(directory_path),
                                 STORAGE_DATA_MOUNT "/sets/%s/procedures", scan->set_id->value);
    if (written < 0 || (size_t)written >= sizeof(directory_path)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    DIR *directory = opendir(directory_path);
    if (directory == NULL) {
        return errno == ENOENT ? APP_ERROR_NONE : storage_repository_map_file_error();
    }
    app_error_code_t result = APP_ERROR_NONE;
    for (struct dirent *entry = readdir(directory); entry != NULL; entry = readdir(directory)) {
        result = scan_procedure_entry(scan, entry);
        if (result != APP_ERROR_NONE) {
            break;
        }
    }
    if (closedir(directory) != 0 && result == APP_ERROR_NONE) {
        result = APP_ERROR_IO;
    }
    return result;
}
'''
text = text[:start] + replacement + text[end:]
text = replace_once(
    text,
    "    if (location->scope == MACRO_SCOPE_SET) {\n"
    "        return scan_set_procedure_references(&location->set_id, macro_id, references);\n"
    "    }\n",
    "    if (location->scope == MACRO_SCOPE_SET) {\n"
    "        const procedure_reference_scan_t scan = {\n"
    "            .set_id = &location->set_id,\n"
    "            .macro_id = macro_id,\n"
    "            .references = references,\n"
    "        };\n"
    "        return scan_set_procedure_references(&scan);\n"
    "    }\n",
    path,
)
text = replace_once(
    text,
    "    for (size_t set = 0U; result == APP_ERROR_NONE && set < index.count; ++set) {\n"
    "        result = scan_set_procedure_references(&index.ids[set], macro_id, references);\n"
    "    }\n",
    "    for (size_t set = 0U; result == APP_ERROR_NONE && set < index.count; ++set) {\n"
    "        const procedure_reference_scan_t scan = {\n"
    "            .set_id = &index.ids[set],\n"
    "            .macro_id = macro_id,\n"
    "            .references = references,\n"
    "        };\n"
    "        result = scan_set_procedure_references(&scan);\n"
    "    }\n",
    path,
)
write(path, text)

# Object JSON: named buffer capacities and non-swappable parse-order API.
path = "firmware/components/storage/storage_repository_objects_json.c"
text = read(path)
text = replace_once(
    text,
    "#define KEY_PRESS_STORAGE_MAX_MS 1000U\n",
    "#define KEY_PRESS_STORAGE_MAX_MS 1000U\n"
    '#define MACRO_SCOPE_BUFFER_BYTES sizeof("global")\n'
    '#define PROCEDURE_STEP_TYPE_BUFFER_BYTES sizeof("instruction")\n',
    path,
)
text = replace_once(text, "char scope[7];", "char scope[MACRO_SCOPE_BUFFER_BYTES];", path)
text = replace_once(text, "char type[12];", "char type[PROCEDURE_STEP_TYPE_BUFFER_BYTES];", path)
text = sub_once(
    text,
    r"app_error_code_t storage_repository_parse_order_json\(const char \*data, size_t length,\n"
    r"\s+size_t maximum_count,\n"
    r"\s+storage_uuid_order_t \*out_order\) \{",
    "app_error_code_t storage_repository_parse_order_json(const char *data, size_t length,\n"
    "                                                     storage_uuid_order_t *out_order,\n"
    "                                                     size_t maximum_count) {",
    path,
)
write(path, text)

path = "firmware/components/storage/storage_repository_objects_json.h"
text = read(path)
text = sub_once(
    text,
    r"app_error_code_t storage_repository_parse_order_json\(const char \*data, size_t length,\n"
    r"\s+size_t maximum_count,\n"
    r"\s+storage_uuid_order_t \*out_order\);",
    "app_error_code_t storage_repository_parse_order_json(const char *data, size_t length,\n"
    "                                                     storage_uuid_order_t *out_order,\n"
    "                                                     size_t maximum_count);",
    path,
)
write(path, text)

# Update every known call site for the reordered parse-order API.
call_replacements = {
    "firmware/components/storage/storage_repository_order.c": (
        "storage_repository_parse_order_json(data, length, maximum_count, out_order)",
        "storage_repository_parse_order_json(data, length, out_order, maximum_count)",
    ),
    "firmware/components/storage/storage_atomic_validators.c": (
        "storage_repository_parse_order_json(data, length, maximum, &order)",
        "storage_repository_parse_order_json(data, length, &order, maximum)",
    ),
    "tests/host/test_storage_object_json.c": (
        "storage_repository_parse_order_json(json, length, 2U, &parsed)",
        "storage_repository_parse_order_json(json, length, &parsed, 2U)",
    ),
}
for path, (old, new) in call_replacements.items():
    text = read(path)
    text = replace_once(text, old, new, path)
    write(path, text)

# Fail closed if a stale call remains in first-party code.
for candidate in list((ROOT / "firmware").rglob("*.[ch]")) + list((ROOT / "tests").rglob("*.[ch]")):
    source = candidate.read_text(encoding="utf-8")
    for match in re.finditer(r"storage_repository_parse_order_json\s*\((.*?)\)", source, re.DOTALL):
        compact = " ".join(match.group(1).split())
        if "storage_uuid_order_t *out_order" in compact:
            continue
        if re.search(r"\blength\s*,\s*(maximum|maximum_count|[0-9]+U)\s*,", compact):
            raise RuntimeError(f"{candidate}: stale parse-order argument order: {compact}")

print("storage quality transformations applied")
