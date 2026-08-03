#ifndef APP_CRC32_H
#define APP_CRC32_H

#include <stddef.h>
#include <stdint.h>

/* CRC-32 (IEEE 802.3, reflected, initial and final XOR 0xFFFFFFFF) -- the
 * checksum a repository carries for each of its packages (SPEC 8.7).
 *
 * Implemented here rather than called from ESP-IDF's ROM so that the host tests
 * compute the identical value: a checksum that differs between the host suite
 * and the device is worse than none, because it fails only where it cannot be
 * debugged. The table is built on first use rather than stored, which costs 1
 * KiB of RAM instead of 1 KiB of flash and keeps this file dependency-free.
 *
 * This detects flash corruption. It is not a defence against tampering: anyone
 * holding a session may rewrite a package legitimately, so a cryptographic
 * digest would protect nothing that is not already open. */
uint32_t app_crc32(const void *data, size_t length);

/* Incremental form, for a checksum assembled from fields that are not
 * contiguous in memory -- the repository's, which covers a schema version, an
 * identifier and one four-byte checksum per package (SPEC 13.7). Feeding those
 * in turn avoids building the record in a buffer first, which at the fifty
 * package limit would be 240 bytes of stack in a component whose frames are
 * already ratcheted.
 *
 * Start from APP_CRC32_INITIAL, call this for each field in order, and finish
 * with app_crc32_finish. app_crc32() is exactly that sequence over one buffer. */
#define APP_CRC32_INITIAL UINT32_MAX

uint32_t app_crc32_update(uint32_t crc, const void *data, size_t length);
uint32_t app_crc32_finish(uint32_t crc);

#endif
