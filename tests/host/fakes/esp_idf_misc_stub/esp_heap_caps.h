#ifndef ESP_HEAP_CAPS_H
#define ESP_HEAP_CAPS_H

#include <stddef.h>
#include <stdint.h>

/* Host-test stand-in for ESP-IDF's components/heap/include/esp_heap_caps.h
 * -- see esp_idf_misc_stub/esp_app_desc.h's header comment for why the real
 * header cannot be compiled on a host build (it pulls in multi_heap.h and
 * further ESP-IDF-only headers). web_server_diagnostics.c is the only
 * first-party file that includes this on a host build; this supplies just
 * the one capability flag and one entry point it calls
 * (heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)). Value copied from the
 * real header (ESP-IDF v5.5.5) so a host-test fake matches production. */

#define MALLOC_CAP_8BIT (1 << 2)

size_t heap_caps_get_largest_free_block(uint32_t caps);

#endif
