#ifndef MACRO_PARSER_LEGACY_LIMITS_H
#define MACRO_PARSER_LEGACY_LIMITS_H

#include "../macro_model/include/macro_limits.h"

/* Retained only for the legacy macro_compile entry point. The v2 compiler uses
 * app_limits_v2.h directly and accepts keyPressMs == 0. Delete this header with
 * macro_compile during the Phase 6 executor migration. */
#define APP_KEY_PRESS_MIN_MS UINT32_C(1)
#define APP_KEY_PRESS_MAX_MS APP_V2_KEY_PRESS_MAX_MS
#define APP_INTER_KEY_MAX_MS APP_V2_INTER_KEY_MAX_MS

#endif
