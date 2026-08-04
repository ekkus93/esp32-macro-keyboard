#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define APP_V2_API_ROUTE_COUNT UINT32_C(21)

#define APP_V2_API_ROUTE_ROWS(X)                                                               \
    X("setupGet", "GET", "/api/v1/setup", "none-unprovisioned-only", "none", "", "",       \
      "application/json", UINT16_C(200), "404")                                                \
    X("setupPost", "POST", "/api/v1/setup", "none-unprovisioned-only", "setupRequest",      \
      "application/json", "jsonBodyMaxBytes", "application/json", UINT16_C(202),             \
      "400,409,413,415,422,500")                                                               \
    X("login", "POST", "/api/v1/auth/login", "none-provisioned-only", "loginRequest",       \
      "application/json", "jsonBodyMaxBytes", "application/json", UINT16_C(200),             \
      "400,403,413,415,422,429,500")                                                           \
    X("logout", "POST", "/api/v1/auth/logout", "session", "none", "", "", "",            \
      UINT16_C(204), "401,500")                                                                \
    X("session", "GET", "/api/v1/auth/session", "session", "none", "", "",               \
      "application/json", UINT16_C(200), "401,500")                                          \
    X("status", "GET", "/api/v1/status", "session", "none", "", "", "application/json", \
      UINT16_C(200), "401,500,503")                                                            \
    X("limits", "GET", "/api/v1/limits", "session", "none", "", "", "application/json", \
      UINT16_C(200), "401,500")                                                                \
    X("blobList", "GET", "/api/v1/blob", "session", "none", "", "",                    \
      "application/json", UINT16_C(200), "401,500,503")                                      \
    X("blobCreate", "POST", "/api/v1/blob", "session", "binaryBlob", "application/gzip", \
      "blobMaxBytes", "application/json", UINT16_C(201), "400,401,413,415,500,503,507")     \
    X("blobLoad", "GET", "/api/v1/blob/{blob_id}", "session", "none", "", "",           \
      "application/gzip", UINT16_C(200), "400,401,404,500,503")                              \
    X("blobDelete", "DELETE", "/api/v1/blob/{blob_id}", "session", "none", "", "",      \
      "", UINT16_C(204), "400,401,404,500,503")                                              \
    X("sendCreate", "POST", "/api/v1/send", "session", "sendRequest",                    \
      "application/json", "jsonBodyMaxBytes", "application/json", UINT16_C(202),             \
      "400,401,409,413,415,422,500,503")                                                      \
    X("sendGet", "GET", "/api/v1/send", "session", "none", "", "",                    \
      "application/json", UINT16_C(200), "401,404,500,503")                                  \
    X("sendCancel", "DELETE", "/api/v1/send", "session", "none", "", "",               \
      "application/json", UINT16_C(202), "401,404,500,503")                                  \
    X("settingsGet", "GET", "/api/v1/settings", "session", "none", "", "",             \
      "application/json", UINT16_C(200), "401,500,503")                                      \
    X("settingsPut", "PUT", "/api/v1/settings", "session", "settingsUpdateRequest",       \
      "application/json", "jsonBodyMaxBytes", "application/json", UINT16_C(200),             \
      "400,401,413,415,422,500,503")                                                          \
    X("passwordChange", "POST", "/api/v1/settings/change-password", "session",             \
      "passwordChangeRequest", "application/json", "jsonBodyMaxBytes", "", UINT16_C(204),   \
      "400,401,403,413,415,422,500,503")                                                      \
    X("restart", "POST", "/api/v1/device/restart", "session", "none", "", "",           \
      "application/json", UINT16_C(202), "401,500,503")                                      \
    X("resetSettings", "POST", "/api/v1/device/reset-settings", "session",                 \
      "resetSettingsRequest", "application/json", "jsonBodyMaxBytes", "application/json",  \
      UINT16_C(202), "400,401,413,415,422,500,503")                                           \
    X("factoryReset", "POST", "/api/v1/device/factory-reset", "session",                   \
      "factoryResetRequest", "application/json", "jsonBodyMaxBytes", "application/json",   \
      UINT16_C(202), "400,401,403,413,415,422,500,503")                                       \
    X("diagnostics", "GET", "/api/v1/diagnostics", "session", "none", "", "",           \
      "application/json", UINT16_C(200), "401,500,503")

#ifdef __cplusplus
}
#endif
