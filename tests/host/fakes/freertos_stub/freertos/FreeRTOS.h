#ifndef FREERTOS_STUB_FREERTOS_H
#define FREERTOS_STUB_FREERTOS_H

/* Host-test stand-in for ESP-IDF's FreeRTOS-Kernel component headers
 * (freertos/FreeRTOS.h plus the xtensa freertos/portmacro.h it pulls in).
 *
 * This is deliberately not a faithful FreeRTOS emulation. The
 * web_server_async_confirmation host target intentionally never starts the
 * async worker. Under H6-060, confirmation-gated dispatch must therefore fail
 * closed with 503 before queue, task, semaphore, critical-section, or async
 * httpd operations are reached. The corresponding definitions in
 * test_web_server_async_confirmation.c are hard-failure canaries so a future
 * code change cannot silently turn this compile-only stub into an approximate
 * concurrency model. */

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

#define portMUX_INITIALIZER_UNLOCKED                                                               \
    { 0U }

void vPortEnterCritical(portMUX_TYPE *mux);
void vPortExitCritical(portMUX_TYPE *mux);
#define portENTER_CRITICAL(mux) vPortEnterCritical(mux)
#define portEXIT_CRITICAL(mux) vPortExitCritical(mux)

#endif
