#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "cJSON.h"
#include "test_assert.h"
#include "web_auth_routes.h"

static void test_session_response_json(void) {
    char *json = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_auth_session_response_json(86400U, 604800U, &json));
    TEST_CHECK(json != NULL);
    cJSON *root = cJSON_Parse(json);
    TEST_CHECK(root != NULL);
    TEST_CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "authenticated")));
    TEST_CHECK_EQ_U64(
        86400U,
        (uint64_t)cJSON_GetObjectItemCaseSensitive(root, "idleExpiresInSeconds")->valuedouble);
    TEST_CHECK_EQ_U64(
        604800U,
        (uint64_t)cJSON_GetObjectItemCaseSensitive(root, "absoluteExpiresInSeconds")->valuedouble);
    /* No v1 "ok"/"data" envelope. */
    TEST_CHECK(cJSON_GetObjectItemCaseSensitive(root, "ok") == NULL);
    cJSON_Delete(root);
    free(json);

    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, web_auth_session_response_json(1U, 2U, NULL));
}

static char *dup_body(const char *text) {
    const size_t length = strlen(text);
    char *copy = malloc(length + 2U);
    TEST_CHECK(copy != NULL);
    memcpy(copy, text, length + 1U);
    copy[length + 1U] = '\0';
    return copy;
}

static void test_login_parse_valid(void) {
    char *body = dup_body("{\"adminPassword\":\"correct-horse-battery\"}");
    char password[64U] = {0};
    size_t password_length = 0U;
    TEST_CHECK_EQ_INT(WEB_AUTH_LOGIN_PARSE_OK,
                      web_auth_login_parse(body, strlen(body) + 2U, password, sizeof(password),
                                           &password_length));
    TEST_CHECK_EQ_STRING("correct-horse-battery", password);
    TEST_CHECK_EQ_U64(strlen("correct-horse-battery"), password_length);
    /* The raw body buffer is wiped before return. */
    TEST_CHECK(body[0] == '\0');
    free(body);
}

static void test_login_parse_rejects_malformed(void) {
    char password[64U] = {0};
    size_t password_length = 0U;

    static const char *const invalid_bodies[] = {
        "",
        "not-json",
        "{\"adminPassword\":\"x\",\"adminPassword\":\"y\"}",
        "{\"adminPassword\":\"x\",\"extra\":true}",
        "{}",
        "{\"other\":\"x\"}",
        "{\"adminPassword\":1234}",
        "{\"adminPassword\":null}",
        "[]",
        "{\"adminPassword\":\"x\"} trailing",
    };
    for (size_t index = 0U; index < sizeof(invalid_bodies) / sizeof(invalid_bodies[0]); ++index) {
        char *body = dup_body(invalid_bodies[index]);
        password[0] = '\0';
        password_length = 999U;
        TEST_CHECK_EQ_INT(WEB_AUTH_LOGIN_PARSE_INVALID_BODY,
                          web_auth_login_parse(body, strlen(body) + 2U, password, sizeof(password),
                                               &password_length));
        TEST_CHECK_EQ_STRING("", password);
        TEST_CHECK_EQ_U64(0U, password_length);
        free(body);
    }
}

static void test_login_parse_rejects_oversized_password(void) {
    char body_text[300U];
    (void)snprintf(body_text, sizeof(body_text), "{\"adminPassword\":\"%050d\"}", 0);
    char *body = dup_body(body_text);
    char small[8U] = {0};
    size_t password_length = 0U;
    TEST_CHECK_EQ_INT(
        WEB_AUTH_LOGIN_PARSE_INVALID_BODY,
        web_auth_login_parse(body, strlen(body) + 2U, small, sizeof(small), &password_length));
    free(body);
}

static void test_login_parse_null_arguments(void) {
    char password[64U] = {0};
    size_t password_length = 0U;
    TEST_CHECK_EQ_INT(WEB_AUTH_LOGIN_PARSE_INVALID_BODY,
                      web_auth_login_parse(NULL, 0U, password, sizeof(password), &password_length));

    char *body = dup_body("{\"adminPassword\":\"x\"}");
    TEST_CHECK_EQ_INT(
        WEB_AUTH_LOGIN_PARSE_INVALID_BODY,
        web_auth_login_parse(body, strlen(body) + 2U, NULL, sizeof(password), &password_length));
    free(body);
}

static void test_login_parse_embedded_nul_escape_rejected(void) {
    char *body = dup_body("{\"adminPassword\":\"a\\u0000b\"}");
    char password[64U] = {0};
    size_t password_length = 0U;
    TEST_CHECK_EQ_INT(WEB_AUTH_LOGIN_PARSE_INVALID_BODY,
                      web_auth_login_parse(body, strlen(body) + 2U, password, sizeof(password),
                                           &password_length));
    free(body);
}

int main(void) {
    test_session_response_json();
    test_login_parse_valid();
    test_login_parse_rejects_malformed();
    test_login_parse_rejects_oversized_password();
    test_login_parse_null_arguments();
    test_login_parse_embedded_nul_escape_rejected();
    puts("web auth routes tests passed");
    return EXIT_SUCCESS;
}
