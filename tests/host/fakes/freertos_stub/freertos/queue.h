#ifndef FREERTOS_STUB_QUEUE_H
#define FREERTOS_STUB_QUEUE_H

#include "freertos/FreeRTOS.h"

/* See FreeRTOS.h's header comment: a dead-path-only stand-in, not a working
 * queue. web_server_async.c is the only first-party file that references
 * these four symbols; their definitions in test_web_server_async_confirmation.c
 * are hard-failure canaries. */

typedef struct QueueDefinition *QueueHandle_t;

QueueHandle_t xQueueCreate(UBaseType_t queue_length, UBaseType_t item_size);
BaseType_t xQueueSend(QueueHandle_t queue, const void *item, TickType_t ticks_to_wait);
BaseType_t xQueueReceive(QueueHandle_t queue, void *out_item, TickType_t ticks_to_wait);
void vQueueDelete(QueueHandle_t queue);

#endif
