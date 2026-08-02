#ifndef MACRO_LIMITS_H
#define MACRO_LIMITS_H

#include <stddef.h>
#include <stdint.h>

#define APP_SCHEMA_VERSION 1U
#define APP_MACRO_NAME_MAX_BYTES 64U
#define APP_MACRO_SOURCE_MAX_BYTES 4096U
#define APP_COMPILED_ACTION_MAX 4096U
#define APP_DELAY_MAX_MS 10000U
#define APP_ESTIMATED_DURATION_MAX_MS 300000U
#define APP_MACROS_PER_SET_MAX 100U
#define APP_MACRO_SETS_MAX 50U
#define APP_IMPORT_PACKAGE_MAX_BYTES (512U * 1024U)

/* SPEC 10.7. The per-object limits above bound one macro; these bound the bytes
 * actually written. Without them the table permits 50 sets x 100 macros x 4096
 * bytes = 20 MB on a 512 KiB partition, so every write is measured against them
 * before it is committed and an over-budget write is rejected with 507
 * (SPEC 17) rather than filling the filesystem. */
#define APP_SET_FILE_MAX_BYTES (32U * 1024U)
#define APP_USER_DATA_MAX_BYTES (480U * 1024U)
#define APP_KEY_PRESS_DEFAULT_MS 8U
#define APP_INTER_KEY_DEFAULT_MS 15U
#define APP_PHYSICAL_CONFIRM_TIMEOUT_MS 20000U
#define APP_UUID_STRING_LENGTH 36U
#define APP_UUID_BUFFER_LENGTH 37U
#define APP_NAME_MAX_BYTES 64U
#define APP_DESCRIPTION_MAX_BYTES 256U
#define APP_MANUFACTURER_MAX_BYTES 64U
#define APP_MODEL_MAX_BYTES 64U
#define APP_BOARD_MAX_BYTES 32U
#define APP_PATH_MAX_BYTES 256U
#define APP_SESSION_TABLE_MAX 8U
#define APP_SESSION_TOKEN_BYTES 32U
#define APP_CSRF_TOKEN_BYTES 32U

#endif
