# Phase 18.3 API integration failure

```text
-- The C compiler identification is GNU 13.3.0
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /usr/bin/cc - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Found PkgConfig: /usr/bin/pkg-config (found version "1.8.1")
-- Checking for module 'libcjson'
--   Found libcjson, version 1.7.17
-- Configuring done (1.0s)
-- Generating done (0.1s)
-- Build files have been written to: /tmp/esp32-p183-api
ninja: error: unknown target 'web_api_set_export_tests'
```

## Pending diff

```diff
diff --git a/firmware/components/web_server/web_api_core.c b/firmware/components/web_server/web_api_core.c
index f54a2c3..c659e8e 100644
--- a/firmware/components/web_server/web_api_core.c
+++ b/firmware/components/web_server/web_api_core.c
@@ -414,7 +414,8 @@ bool web_api_physical_confirmation_required(web_api_route_t route,
     }
     return route == WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD ||
            route == WEB_API_ROUTE_DEVICE_RESTART || route == WEB_API_ROUTE_DEVICE_RESET_SETTINGS ||
-           route == WEB_API_ROUTE_DEVICE_FACTORY_RESET || route == WEB_API_ROUTE_RESTORE;
+           route == WEB_API_ROUTE_DEVICE_FACTORY_RESET || route == WEB_API_ROUTE_SET_IMPORT ||
+           route == WEB_API_ROUTE_RESTORE;
 }
 
 bool web_api_route_requires_physical_confirmation(web_api_route_t route) {
diff --git a/firmware/components/web_server/web_api_sets.c b/firmware/components/web_server/web_api_sets.c
index b706e45..99c75e9 100644
--- a/firmware/components/web_server/web_api_sets.c
+++ b/firmware/components/web_server/web_api_sets.c
@@ -261,6 +261,96 @@ static app_error_code_t handle_set_reorder(const web_api_call_t *call,
     return result;
 }
 
+
+typedef struct {
+    app_uuid_t target_set_id;
+    uint32_t expected_revision;
+    char *package_json;
+    size_t package_length;
+} web_set_import_request_t;
+
+static void free_set_import_request(web_set_import_request_t *request) {
+    if (request == NULL) {
+        return;
+    }
+    cJSON_free(request->package_json);
+    memset(request, 0, sizeof(*request));
+}
+
+static app_error_code_t parse_set_import(const web_api_call_t *call,
+                                         web_set_import_request_t *out_request) {
+    if (call == NULL || out_request == NULL || call->body == NULL || call->body_length == 0U) {
+        return APP_ERROR_INVALID_ARGUMENT;
+    }
+    memset(out_request, 0, sizeof(*out_request));
+    const char *parse_end = NULL;
+    cJSON *root = cJSON_ParseWithLengthOpts(call->body, call->body_length, &parse_end, false);
+    bool target_seen = false;
+    bool revision_seen = false;
+    bool package_seen = false;
+    app_error_code_t result =
+        root != NULL && parse_end == call->body + call->body_length && cJSON_IsObject(root)
+            ? APP_ERROR_NONE
+            : APP_ERROR_INVALID_ARGUMENT;
+    for (const cJSON *item = result == APP_ERROR_NONE ? root->child : NULL; item != NULL;
+         item = item->next) {
+        if (item->string != NULL && strcmp(item->string, "targetSetId") == 0 && !target_seen &&
+            cJSON_IsString(item) && item->valuestring != NULL &&
+            app_uuid_parse(item->valuestring, &out_request->target_set_id) == APP_ERROR_NONE) {
+            target_seen = true;
+        } else if (item->string != NULL && strcmp(item->string, "expectedRevision") == 0 &&
+                   !revision_seen && cJSON_IsNumber(item) && item->valuedouble >= 1.0 &&
+                   item->valuedouble <= (double)UINT32_MAX) {
+            const uint32_t revision = (uint32_t)item->valuedouble;
+            if ((double)revision != item->valuedouble) {
+                result = APP_ERROR_INVALID_ARGUMENT;
+                break;
+            }
+            out_request->expected_revision = revision;
+            revision_seen = true;
+        } else if (item->string != NULL && strcmp(item->string, "package") == 0 &&
+                   !package_seen && cJSON_IsObject(item)) {
+            out_request->package_json = cJSON_PrintUnformatted(item);
+            if (out_request->package_json == NULL) {
+                result = APP_ERROR_INTERNAL;
+                break;
+            }
+            out_request->package_length = strlen(out_request->package_json);
+            if (out_request->package_length == 0U ||
+                out_request->package_length > APP_IMPORT_PACKAGE_MAX_BYTES) {
+                result = APP_ERROR_INVALID_ARGUMENT;
+                break;
+            }
+            package_seen = true;
+        } else {
+            result = APP_ERROR_INVALID_ARGUMENT;
+            break;
+        }
+    }
+    cJSON_Delete(root);
+    if (result == APP_ERROR_NONE && (!target_seen || !revision_seen || !package_seen)) {
+        result = APP_ERROR_INVALID_ARGUMENT;
+    }
+    if (result != APP_ERROR_NONE) {
+        free_set_import_request(out_request);
+    }
+    return result;
+}
+
+static app_error_code_t handle_import(const web_api_call_t *call, web_api_response_t *response) {
+    web_set_import_request_t request = {0};
+    app_error_code_t result = parse_set_import(call, &request);
+    macro_set_t committed = {0};
+    if (result == APP_ERROR_NONE) {
+        result = storage_package_replace_set(&request.target_set_id, request.expected_revision,
+                                             request.package_json, request.package_length,
+                                             &committed);
+    }
+    free_set_import_request(&request);
+    return result == APP_ERROR_NONE ? send_set(response, WEB_HTTP_STATUS_OK, &committed)
+                                    : respond_result(response, result, "could not replace set");
+}
+
 static app_error_code_t handle_export(const web_api_call_t *call, web_api_response_t *response) {
     char *package_json = NULL;
     size_t package_length = 0U;
@@ -276,10 +366,6 @@ static app_error_code_t handle_export(const web_api_call_t *call, web_api_respon
     return result;
 }
 
-static app_error_code_t unavailable(web_api_response_t *response, const char *operation) {
-    return web_api_handler_error(response, APP_ERROR_STORAGE_UNAVAILABLE, operation, NULL);
-}
-
 app_error_code_t web_api_handle_sets(const web_api_call_t *call, web_api_response_t *response) {
     if (call == NULL || response == NULL) {
         return APP_ERROR_INVALID_ARGUMENT;
@@ -298,7 +384,7 @@ app_error_code_t web_api_handle_sets(const web_api_call_t *call, web_api_respons
     case WEB_API_ROUTE_SET_EXPORT:
         return handle_export(call, response);
     case WEB_API_ROUTE_SET_IMPORT:
-        return unavailable(response, "set import requires the Phase 18 package service");
+        return handle_import(call, response);
     default:
         return APP_ERROR_NOT_FOUND;
     }
diff --git a/firmware/components/web_server/web_server_api.c b/firmware/components/web_server/web_server_api.c
index 7ea542c..45f6189 100644
--- a/firmware/components/web_server/web_server_api.c
+++ b/firmware/components/web_server/web_server_api.c
@@ -113,6 +113,7 @@ static size_t route_body_limit(web_api_route_t route) {
     case WEB_API_ROUTE_PROCEDURE_PROGRESS:
         return STORAGE_PROGRESS_FILE_MAX_BYTES + WEB_API_WRAPPER_OVERHEAD_BYTES;
     case WEB_API_ROUTE_SET_IMPORT:
+        return APP_IMPORT_PACKAGE_MAX_BYTES + WEB_API_WRAPPER_OVERHEAD_BYTES;
     case WEB_API_ROUTE_RESTORE:
         return APP_IMPORT_PACKAGE_MAX_BYTES;
     default:
diff --git a/tests/host/cmake/extra_tests.cmake b/tests/host/cmake/extra_tests.cmake
index 62c968f..fefb560 100644
--- a/tests/host/cmake/extra_tests.cmake
+++ b/tests/host/cmake/extra_tests.cmake
@@ -214,6 +214,8 @@ add_executable(
     "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_parser/macro_keymap_us.c"
     "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package.c"
     "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package_export.c"
+    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package_replace.c"
+    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_set_tree.c"
     "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server/web_api_core.c"
     "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server/web_api_response.c"
     "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server/web_api_json.c"
@@ -234,7 +236,7 @@ target_include_directories(
 )
 target_compile_definitions(
     web_api_set_export_tests
-    PRIVATE STORAGE_DATA_MOUNT="${CMAKE_CURRENT_BINARY_DIR}/web-api-set-export-data"
+    PRIVATE STORAGE_DATA_MOUNT="/tmp/esp32-macro-keyboard-web-package"
 )
 target_link_libraries(web_api_set_export_tests PRIVATE PkgConfig::CJSON test_support)
 target_compile_options(web_api_set_export_tests PRIVATE ${STRICT_WARNINGS})
diff --git a/tests/host/test_web_api_core.c b/tests/host/test_web_api_core.c
index ca49d6a..c89f845 100644
--- a/tests/host/test_web_api_core.c
+++ b/tests/host/test_web_api_core.c
@@ -84,6 +84,7 @@ static void test_route_policy(void) {
     TEST_CHECK(web_api_route_requires_csrf(WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_PUT));
     TEST_CHECK(!web_api_route_requires_csrf(WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_GET));
     TEST_CHECK(web_api_route_requires_physical_confirmation(WEB_API_ROUTE_DEVICE_FACTORY_RESET));
+    TEST_CHECK(web_api_route_requires_physical_confirmation(WEB_API_ROUTE_SET_IMPORT));
     TEST_CHECK(web_api_route_requires_physical_confirmation(WEB_API_ROUTE_EXECUTIONS));
     TEST_CHECK(web_api_physical_confirmation_required(WEB_API_ROUTE_EXECUTIONS, true));
     TEST_CHECK(!web_api_physical_confirmation_required(WEB_API_ROUTE_EXECUTIONS, false));
diff --git a/tests/host/test_web_api_set_export.c b/tests/host/test_web_api_set_export.c
index d112b5a..0997355 100644
--- a/tests/host/test_web_api_set_export.c
+++ b/tests/host/test_web_api_set_export.c
@@ -9,6 +9,7 @@
 
 #include "app_error.h"
 #include "app_uuid.h"
+#include "cJSON.h"
 #include "macro_model.h"
 #include "provisioning.h"
 #include "storage_package.h"
@@ -261,6 +262,82 @@ static web_api_response_t invoke_export(const char *set_id) {
     return response;
 }
 
+
+static web_api_response_t invoke_import(const char *body) {
+    const web_api_call_t call = {
+        .method = WEB_API_METHOD_POST,
+        .path = {.route = WEB_API_ROUTE_SET_IMPORT},
+        .body = body,
+        .body_length = strlen(body),
+    };
+    web_api_response_t response = {0};
+    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_handle_sets(&call, &response));
+    TEST_CHECK(response.body != NULL);
+    return response;
+}
+
+static char *make_replacement_request(uint32_t expected_revision) {
+    web_api_response_t exported = invoke_export(SET_ID);
+    TEST_CHECK_EQ_U64(200U, exported.status);
+    const char *parse_end = NULL;
+    cJSON *package =
+        cJSON_ParseWithLengthOpts(exported.body, exported.body_length, &parse_end, false);
+    TEST_CHECK(package != NULL);
+    TEST_CHECK(parse_end == exported.body + exported.body_length);
+    cJSON *sets = cJSON_GetObjectItemCaseSensitive(package, "sets");
+    cJSON *set = cJSON_GetArrayItem(sets, 0);
+    TEST_CHECK(cJSON_IsObject(set));
+    cJSON *revision = cJSON_CreateNumber(2.0);
+    cJSON *name = cJSON_CreateString("Imported Replacement");
+    TEST_CHECK(revision != NULL);
+    TEST_CHECK(name != NULL);
+    TEST_CHECK(cJSON_ReplaceItemInObjectCaseSensitive(set, "revision", revision));
+    TEST_CHECK(cJSON_ReplaceItemInObjectCaseSensitive(set, "name", name));
+
+    cJSON *wrapper = cJSON_CreateObject();
+    TEST_CHECK(wrapper != NULL);
+    TEST_CHECK(cJSON_AddStringToObject(wrapper, "targetSetId", SET_ID) != NULL);
+    TEST_CHECK(cJSON_AddNumberToObject(wrapper, "expectedRevision", (double)expected_revision) !=
+               NULL);
+    TEST_CHECK(cJSON_AddItemToObject(wrapper, "package", package));
+    package = NULL;
+    char *request = cJSON_PrintUnformatted(wrapper);
+    TEST_CHECK(request != NULL);
+    cJSON_Delete(wrapper);
+    cJSON_Delete(package);
+    web_api_response_free(&exported);
+    return request;
+}
+
+static void test_import_route(void) {
+    char *request = make_replacement_request(1U);
+    web_api_response_t response = invoke_import(request);
+    TEST_CHECK_EQ_U64(200U, response.status);
+    TEST_CHECK(strstr(response.body, "\"ok\":true") != NULL);
+    TEST_CHECK(strstr(response.body, "Imported Replacement") != NULL);
+    TEST_CHECK(strstr(response.body, "\"revision\":2") != NULL);
+    macro_set_t committed = {0};
+    const app_uuid_t set_id = uuid(SET_ID);
+    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_read(&set_id, &committed));
+    TEST_CHECK_EQ_U64(2U, committed.revision);
+    TEST_CHECK_EQ_STRING("Imported Replacement", committed.name);
+    web_api_response_free(&response);
+    cJSON_free(request);
+
+    request = make_replacement_request(1U);
+    response = invoke_import(request);
+    TEST_CHECK_EQ_U64(409U, response.status);
+    TEST_CHECK(strstr(response.body, "\"ok\":false") != NULL);
+    TEST_CHECK(strstr(response.body, "could not replace set") != NULL);
+    web_api_response_free(&response);
+    cJSON_free(request);
+
+    response = invoke_import("{\"targetSetId\":\"" SET_ID
+                             "\",\"expectedRevision\":2,\"package\":{},\"extra\":true}");
+    TEST_CHECK_EQ_U64(422U, response.status);
+    web_api_response_free(&response);
+}
+
 static void test_export_route(void) {
     web_api_response_t response = invoke_export(SET_ID);
     TEST_CHECK_EQ_U64(200U, response.status);
@@ -301,6 +378,7 @@ int main(void) {
     populate_store();
     install_export_operations();
     test_export_route();
+    test_import_route();
     test_missing_set_error_envelope();
     storage_package_reset_export_ops_for_test();
     TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_deinit());
```
