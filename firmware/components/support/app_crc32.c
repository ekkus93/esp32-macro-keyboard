#include "app_crc32.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CRC32_POLYNOMIAL UINT32_C(0xEDB88320)
#define CRC32_TABLE_SIZE 256U
#define CRC32_BYTE_MASK 0xFFU
#define CRC32_BITS_PER_BYTE 8U

static uint32_t crc32_table[CRC32_TABLE_SIZE];
static bool crc32_table_ready = false;

static void build_table(void) {
    for (uint32_t index = 0U; index < CRC32_TABLE_SIZE; ++index) {
        uint32_t value = index;
        for (unsigned bit = 0U; bit < CRC32_BITS_PER_BYTE; ++bit) {
            value = ((value & 1U) != 0U) ? ((value >> 1U) ^ CRC32_POLYNOMIAL) : (value >> 1U);
        }
        crc32_table[index] = value;
    }
    crc32_table_ready = true;
}

uint32_t app_crc32(const void *data, size_t length) {
    if (data == NULL) {
        return 0U;
    }
    if (!crc32_table_ready) {
        build_table();
    }
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t crc = UINT32_MAX;
    for (size_t index = 0U; index < length; ++index) {
        crc = crc32_table[(crc ^ bytes[index]) & CRC32_BYTE_MASK] ^ (crc >> CRC32_BITS_PER_BYTE);
    }
    return crc ^ UINT32_MAX;
}
