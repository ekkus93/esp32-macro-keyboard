#ifndef FACTORY_RESET_RECOVERY_H
#define FACTORY_RESET_RECOVERY_H

#include <stdbool.h>

#include "app_error.h"

/* Complete a previously committed factory reset before ordinary startup.
 * Missing reset state is a no-op. A pending reset repeats every durable cleanup
 * stage and clears the journal only after settings, blobs, temporary debris,
 * and subsystem teardown all report success. */
app_error_code_t factory_reset_recovery_run_if_pending(bool *out_recovered);

#endif
