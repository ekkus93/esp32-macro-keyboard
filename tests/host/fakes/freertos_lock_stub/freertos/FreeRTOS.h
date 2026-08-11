#ifndef FREERTOS_LOCK_STUB_FREERTOS_H
#define FREERTOS_LOCK_STUB_FREERTOS_H

/* Host-only semantic stand-in for FreeRTOS portMUX critical sections. Unlike
 * the broader dead-path FreeRTOS stub, this maps them to a real pthread mutex
 * so focused host stress tests exercise actual concurrent exclusion. */

#include <pthread.h>

typedef pthread_mutex_t portMUX_TYPE;

#define portMUX_INITIALIZER_UNLOCKED PTHREAD_MUTEX_INITIALIZER
#define portENTER_CRITICAL(mux) ((void)pthread_mutex_lock((mux)))
#define portEXIT_CRITICAL(mux) ((void)pthread_mutex_unlock((mux)))

#endif
