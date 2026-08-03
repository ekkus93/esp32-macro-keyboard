#ifndef MACRO_LIMITS_H
#define MACRO_LIMITS_H

#include <stddef.h>
#include <stdint.h>

#define APP_SCHEMA_VERSION 1U

/*
 * Temporary v1 compatibility values. The authoritative v2 constants live in
 * app_contracts_v2/include/app_limits_v2.h. scripts/check-v2-limits.py verifies
 * that every shared value below remains identical until Phase 2 deletes the
 * retired repository architecture.
 */
#define APP_MACRO_NAME_MAX_BYTES UINT32_C(64)
#define APP_MACRO_SOURCE_MAX_BYTES UINT32_C(4096)
#define APP_COMPILED_ACTION_MAX UINT32_C(4096)
#define APP_DELAY_MAX_MS UINT32_C(10000)
#define APP_ESTIMATED_DURATION_MAX_MS UINT32_C(300000)
#define APP_MACROS_PER_SET_MAX 100U
#define APP_MACRO_SETS_MAX 50U
#define APP_IMPORT_PACKAGE_MAX_BYTES (512U * 1024U)

/* Retired v1 repository limits remain only until the Phase 2 repository removal.
 * New v2 production code must use app_limits_v2.h directly. */
#define APP_SET_FILE_MAX_BYTES (32U * 1024U)
#define APP_USER_DATA_MAX_BYTES (480U * 1024U)
#define APP_KEY_PRESS_DEFAULT_MS 8U
#define APP_INTER_KEY_DEFAULT_MS 15U
#define APP_PHYSICAL_CONFIRM_TIMEOUT_MS UINT32_C(60000)
#define APP_UUID_STRING_LENGTH 36U
#define APP_UUID_BUFFER_LENGTH 37U
#define APP_NAME_MAX_BYTES UINT32_C(64)
#define APP_DESCRIPTION_MAX_BYTES 256U
#define APP_MANUFACTURER_MAX_BYTES 64U
#define APP_MODEL_MAX_BYTES 64U
#define APP_BOARD_MAX_BYTES 32U
#define APP_PATH_MAX_BYTES 256U
#define APP_SESSION_TABLE_MAX UINT32_C(8)
#define APP_SESSION_TOKEN_BYTES 32U

#endif
