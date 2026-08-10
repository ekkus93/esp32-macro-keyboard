#ifndef FREERTOS_STUB_TASK_H
#define FREERTOS_STUB_TASK_H

#include "freertos/FreeRTOS.h"

/* See FreeRTOS.h's header comment: a dead-path-only stand-in, not a working
 * task scheduler. web_server_async.c is the only first-party file that
 * references these two symbols; their definitions in
 * test_web_server_async_confirmation.c are hard-failure canaries. Real
 * ESP-IDF: configSTACK_DEPTH_TYPE (task.h's xTaskCreate() stack-depth
 * parameter) is either uint16_t or uint32_t depending on sdkconfig; uint32_t
 * accepts either real call site unmodified, and the exact width is
 * immaterial to a signature no test ever calls. */

typedef struct tskTaskControlBlock *TaskHandle_t;
typedef void (*TaskFunction_t)(void *);

BaseType_t xTaskCreate(TaskFunction_t task_code, const char *name, uint32_t stack_depth,
                       void *parameters, UBaseType_t priority, TaskHandle_t *out_handle);
void vTaskDelete(TaskHandle_t task);

#endif
