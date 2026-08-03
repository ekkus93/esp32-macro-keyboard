#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>

#include "app_error.h"
#include "macro_parser.h"

static int failures = 0;

static void report_failure(const char *case_name, const char *message) {
    (void)fprintf(stderr, "[FAIL] %s: %s\n", case_name, message);
    ++failures;
}

static char *read_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }
    if (fseek(file, 0L, SEEK_END) != 0) {
        (void)fclose(file);
        return NULL;
    }
    const long length = ftell(file);
    if (length < 0L || fseek(file, 0L, SEEK_SET) != 0) {
        (void)fclose(file);
        return NULL;
    }
    const size_t size = (size_t)length;
    char *contents = malloc(size + 1U);
    if (contents == NULL) {
        (void)fclose(file);
        return NULL;
    }
    const size_t read_count = fread(contents, 1U, size, file);
    const int close_result = fclose(file);
    if (read_count != size || close_result != 0) {
        free(contents);
        return NULL;
    }
    contents[size] = '\0';
    return contents;
}

static const cJSON *required_item(const cJSON *object, const char *name) {
    if (!cJSON_IsObject(object)) {
        return NULL;
    }
    return cJSON_GetObjectItemCaseSensitive(object, name);
}

static bool read_u32(const cJSON *item, uint32_t *out_value) {
    if (!cJSON_IsNumber(item) || out_value == NULL || item->valuedouble < 0.0 ||
        item->valuedouble > (double)UINT32_MAX) {
        return false;
    }
    const uint32_t value = (uint32_t)item->valuedouble;
    if ((double)value != item->valuedouble) {
        return false;
    }
    *out_value = value;
    return true;
}

static bool read_size(const cJSON *item, size_t *out_value) {
    uint32_t value = 0U;
    if (!read_u32(item, &value) || out_value == NULL) {
        return false;
    }
    *out_value = (size_t)value;
    return true;
}

static app_error_code_t expected_error_code(const char *name) {
    if (strcmp(name, "macro_syntax") == 0) {
        return APP_ERROR_MACRO_SYNTAX;
    }
    if (strcmp(name, "macro_limit") == 0) {
        return APP_ERROR_MACRO_LIMIT;
    }
    if (strcmp(name, "invalid_argument") == 0) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return APP_ERROR_INTERNAL;
}

static const char *message_class(const char *message) {
    if (strcmp(message, "estimated duration limit exceeded") == 0) {
        return "duration_limit";
    }
    if (strcmp(message, "unknown key directive") == 0) {
        return "unknown_key";
    }
    if (strcmp(message, "invalid chord directive") == 0 || strcmp(message, "invalid chord") == 0) {
        return "invalid_chord";
    }
    if (strcmp(message, "invalid delay directive") == 0 ||
        strcmp(message, "delay is outside the allowed range") == 0) {
        return "invalid_delay";
    }
    if (strcmp(message, "lone carriage return") == 0 ||
        strcmp(message, "carriage return must be followed by line feed") == 0) {
        return "lone_carriage_return";
    }
    if (strcmp(message, "unmatched opening brace") == 0) {
        return "unmatched_opening_brace";
    }
    if (strcmp(message, "unmatched closing brace") == 0) {
        return "unmatched_closing_brace";
    }
    if (strcmp(message, "unsupported character") == 0 ||
        strcmp(message, "source contains unsupported character") == 0) {
        return "unsupported_character";
    }
    if (strcmp(message, "invalid directive") == 0) {
        return "invalid_directive";
    }
    if (strcmp(message, "compiled action limit exceeded") == 0 ||
        strcmp(message, "action limit exceeded") == 0) {
        return "action_limit";
    }
    if (strcmp(message, "macro source exceeds the byte limit") == 0) {
        return "source_limit";
    }
    if (strcmp(message, "invalid macro timing") == 0) {
        return "invalid_timing";
    }
    return "unclassified";
}

static bool compare_action(const cJSON *expected, const macro_action_t *actual) {
    const cJSON *kind = required_item(expected, "kind");
    if (!cJSON_IsString(kind) || kind->valuestring == NULL) {
        return false;
    }

    if (strcmp(kind->valuestring, "delay") == 0) {
        uint32_t duration = 0U;
        return actual->type == MACRO_ACTION_DELAY &&
               read_u32(required_item(expected, "durationMs"), &duration) &&
               actual->delay_ms == duration;
    }

    uint32_t usage = 0U;
    uint32_t modifiers = 0U;
    if (!read_u32(required_item(expected, "usage"), &usage) || usage > UINT8_MAX ||
        !read_u32(required_item(expected, "modifiers"), &modifiers) || modifiers > UINT8_MAX) {
        return false;
    }
    const macro_action_type_t expected_type =
        strcmp(kind->valuestring, "key") == 0 ? MACRO_ACTION_KEY : MACRO_ACTION_CHORD;
    if (expected_type == MACRO_ACTION_CHORD && strcmp(kind->valuestring, "chord") != 0) {
        return false;
    }
    return actual->type == expected_type && actual->usage == (uint8_t)usage &&
           actual->modifiers == (uint8_t)modifiers && actual->delay_ms == 0U;
}

static void verify_valid_case(const char *case_name, const cJSON *valid,
                              const macro_plan_t *plan) {
    uint32_t duration = 0U;
    const cJSON *actions = required_item(valid, "actions");
    if (!read_u32(required_item(valid, "estimatedDurationMs"), &duration) ||
        !cJSON_IsArray(actions)) {
        report_failure(case_name, "invalid valid-case expectation object");
        return;
    }
    const int expected_count = cJSON_GetArraySize(actions);
    if (expected_count < 0 || plan->action_count != (size_t)expected_count ||
        plan->estimated_duration_ms != duration) {
        report_failure(case_name, "action count or estimated duration differs");
        return;
    }
    for (int index = 0; index < expected_count; ++index) {
        const cJSON *expected_action = cJSON_GetArrayItem(actions, index);
        if (expected_action == NULL || !compare_action(expected_action, &plan->actions[index])) {
            report_failure(case_name, "compiled action differs");
            return;
        }
    }
}

static void verify_invalid_case(const char *case_name, const cJSON *invalid,
                                app_error_code_t result, const macro_parse_error_t *error) {
    const cJSON *code = required_item(invalid, "code");
    const cJSON *expected_class = required_item(invalid, "messageClass");
    size_t byte_offset = 0U;
    size_t line = 0U;
    size_t column = 0U;
    if (!cJSON_IsString(code) || code->valuestring == NULL || !cJSON_IsString(expected_class) ||
        expected_class->valuestring == NULL ||
        !read_size(required_item(invalid, "byteOffset"), &byte_offset) ||
        !read_size(required_item(invalid, "line"), &line) ||
        !read_size(required_item(invalid, "column"), &column)) {
        report_failure(case_name, "invalid invalid-case expectation object");
        return;
    }
    if (result != expected_error_code(code->valuestring) || error->code != result ||
        error->byte_offset != byte_offset || error->line != line || error->column != column ||
        strcmp(message_class(error->message), expected_class->valuestring) != 0) {
        report_failure(case_name, "error code, location, or message class differs");
    }
}

static void run_case(const cJSON *test_case) {
    const cJSON *name = required_item(test_case, "name");
    const cJSON *source = required_item(test_case, "source");
    uint32_t key_press_ms = 0U;
    uint32_t inter_key_ms = 0U;
    if (!cJSON_IsString(name) || name->valuestring == NULL || !cJSON_IsString(source) ||
        source->valuestring == NULL ||
        !read_u32(required_item(test_case, "keyPressMs"), &key_press_ms) ||
        !read_u32(required_item(test_case, "interKeyMs"), &inter_key_ms)) {
        report_failure("<invalid case>", "case header is malformed");
        return;
    }

    const cJSON *valid = required_item(test_case, "valid");
    const cJSON *invalid = required_item(test_case, "invalid");
    if ((valid == NULL) == (invalid == NULL)) {
        report_failure(name->valuestring, "case must contain exactly one result object");
        return;
    }

    const macro_compile_options_t options = {
        .key_press_ms = key_press_ms,
        .inter_key_ms = inter_key_ms,
    };
    macro_plan_t plan = {0};
    macro_parse_error_t error = {0};
    const app_error_code_t result =
        macro_compile(source->valuestring, strlen(source->valuestring), &options, &plan, &error);

    if (valid != NULL) {
        if (result != APP_ERROR_NONE) {
            report_failure(name->valuestring, "compiler rejected a valid corpus case");
        } else {
            verify_valid_case(name->valuestring, valid, &plan);
        }
    } else if (result == APP_ERROR_NONE) {
        report_failure(name->valuestring, "compiler accepted an invalid corpus case");
    } else {
        verify_invalid_case(name->valuestring, invalid, result, &error);
    }
    macro_plan_free(&plan);
    if (failures == 0) {
        (void)printf("[PASS] %s\n", name->valuestring);
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        (void)fprintf(stderr, "usage: %s <macro-conformance.json>\n", argv[0]);
        return 2;
    }
    char *text = read_file(argv[1]);
    if (text == NULL) {
        (void)fprintf(stderr, "failed to read %s\n", argv[1]);
        return 2;
    }
    cJSON *root = cJSON_Parse(text);
    free(text);
    if (!cJSON_IsObject(root)) {
        cJSON_Delete(root);
        (void)fprintf(stderr, "macro corpus is not a JSON object\n");
        return 2;
    }
    const cJSON *format = required_item(root, "format");
    const cJSON *version = required_item(root, "version");
    const cJSON *cases = required_item(root, "cases");
    uint32_t version_number = 0U;
    if (!cJSON_IsString(format) || format->valuestring == NULL ||
        strcmp(format->valuestring, "esp32-macro-keyboard-macro-conformance") != 0 ||
        !read_u32(version, &version_number) || version_number != 1U || !cJSON_IsArray(cases)) {
        cJSON_Delete(root);
        (void)fprintf(stderr, "macro corpus identity is invalid\n");
        return 2;
    }

    const int case_count = cJSON_GetArraySize(cases);
    for (int index = 0; index < case_count; ++index) {
        const cJSON *test_case = cJSON_GetArrayItem(cases, index);
        run_case(test_case);
    }
    cJSON_Delete(root);

    if (failures != 0) {
        (void)fprintf(stderr, "%d macro conformance case(s) failed\n", failures);
        return 1;
    }
    (void)printf("all %d native macro conformance cases passed\n", case_count);
    return 0;
}
