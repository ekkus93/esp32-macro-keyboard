#ifndef FREERTOS_STUB_SEMPHR_H
#define FREERTOS_STUB_SEMPHR_H

#include "freertos/queue.h"

/* See FreeRTOS.h's header comment: a dead-path-only stand-in, not a working
 * semaphore. web_server_async.c is the only first-party file that
 * references these four symbols; their definitions in
 * test_web_server_async_confirmation.c are hard-failure canaries. */

typedef QueueHandle_t SemaphoreHandle_t;

SemaphoreHandle_t xSemaphoreCreateBinary(void);
BaseType_t xSemaphoreGive(SemaphoreHandle_t semaphore);
BaseType_t xSemaphoreTake(SemaphoreHandle_t semaphore, TickType_t ticks_to_wait);
void vSemaphoreDelete(SemaphoreHandle_t semaphore);

#endif
