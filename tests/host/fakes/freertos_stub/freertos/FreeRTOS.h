#ifndef FREERTOS_STUB_FREERTOS_H
#define FREERTOS_STUB_FREERTOS_H

/* Host-test stand-in for ESP-IDF's FreeRTOS-Kernel component headers
 * (freertos/FreeRTOS.h plus the xtensa freertos/portmacro.h it pulls in).
 *
 * This is deliberately NOT a faithful FreeRTOS emulation, unlike
 * esp_http_server_stub or esp_idf_misc_stub's headers. Every type/macro here
 * exists only so web_server_async.c -- the one first-party file this
 * repository compiles for a host target that calls FreeRTOS APIs directly
 * (xQueueCreate/xQueueSend/xQueueReceive/xTaskCreate/xSemaphoreCreateBinary/
 * portENTER_CRITICAL/portEXIT_CRITICAL, none behind a backend interface the
 * way macro_executor's FreeRTOS use is) -- can compile and link on a host
 * build at all.
 *
 * Every target that links this stub set (see
 * tests/host/test_web_server_async_confirmation.c) deliberately never calls
 * web_server_async_start()/web_server_async_stop(). As long as those two
 * entry points are never invoked, web_server_async.c's own static
 * async_queue/async_task_handle globals stay at their zero-initialized NULL
 * state for the whole process, which sends every
 * web_server_async_dispatch() call down its documented "worker unavailable:
 * answer on the httpd task rather than drop the request" fallback branch
 * (see that function's own comment in web_server_async.c) -- itself a real
 * production code path (the same one a real device takes if the worker task
 * ever fails to start), not a test-only shortcut. claim_in_flight()/
 * release_in_flight() (the only callers of portENTER_CRITICAL/
 * portEXIT_CRITICAL) and every queue/semaphore/task primitive declared
 * across this directory (queue.h, semphr.h, task.h) are therefore never
 * reached by that fallback branch either.
 *
 * Their definitions (in test_web_server_async_confirmation.c) are
 * deliberate hard-failure canaries, not working stand-ins: if a future
 * change to web_server_async.c ever makes one of these dead paths reachable
 * without updating this stub, the test fails loudly instead of silently
 * approximating real FreeRTOS concurrency semantics. That risk -- a fake
 * queue/task pair subtle enough to get blocking semantics right, or wrong in
 * a way that hides a real race -- is exactly what four independent prior
 * V2-057 investigation rounds concluded made a *faithful* FreeRTOS host mock
 * too large/risky to build (see docs/implementation-v2/V2_057_*.md and
 * V2_051_057_ROUTE_ACCESS_MATRIX_2026-08-09.md's "Item 2" section). This
 * stub does not attempt that; it only unblocks testing the one branch of
 * web_server_async_dispatch() that needs no concurrency at all. See
 * docs/implementation-v2/V2_057_PHASE5_HARDENING_2026-08-09.md for the full
 * account of what this track built and what remains genuinely open. */

#include <stdint.h>

typedef long BaseType_t;
typedef unsigned long UBaseType_t;
typedef uint32_t TickType_t;

#define pdFALSE ((BaseType_t)0)
#define pdTRUE ((BaseType_t)1)
#define pdPASS (pdTRUE)
#define pdFAIL (pdFALSE)

#define portMAX_DELAY ((TickType_t)0xffffffffUL)
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))

/* Real ESP-IDF: spinlock_t (soc/spinlock.h), aliased to portMUX_TYPE. Never
 * actually locked/unlocked under test (see the header comment above), so the
 * exact field layout does not matter -- only that it is a real type an
 * async_lock global can be statically initialized with. */
typedef struct {
    volatile uint32_t placeholder;
} portMUX_TYPE;

#define portMUX_INITIALIZER_UNLOCKED { 0U }

void vPortEnterCritical(portMUX_TYPE *mux);
void vPortExitCritical(portMUX_TYPE *mux);
#define portENTER_CRITICAL(mux) vPortEnterCritical(mux)
#define portEXIT_CRITICAL(mux) vPortExitCritical(mux)

#endif
