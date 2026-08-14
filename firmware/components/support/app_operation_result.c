#include "app_operation_result.h"

/* The structured-result helpers are intentionally header-inline so focused host
 * targets that compile storage state machines directly do not need an otherwise
 * unrelated support-library link dependency. Keep this translation unit for the
 * existing component/test source lists and as the stable home for future
 * non-inline result logic if it becomes necessary. */
