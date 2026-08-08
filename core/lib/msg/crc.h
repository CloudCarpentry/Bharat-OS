#ifndef BHARAT_LIB_MSG_CRC_H
#define BHARAT_LIB_MSG_CRC_H

#include <stdint.h>
#include <stddef.h>

// Internal generic implementation
uint32_t bharat_msg_crc32_generic(const uint8_t *data, size_t len);

typedef uint32_t (*crc_func_t)(const uint8_t *data, size_t len);

// Boot-time one-shot backend registration
void bharat_msg_crc_register_backend(crc_func_t backend);

/**
 * Compute IEEE 802.3 CRC-32 for a byte buffer.
 *
 * Selects the appropriate architecture/backend implementation while
 * preserving wire-compatible CRC semantics.
 */
uint32_t bharat_msg_crc32(const uint8_t *data, size_t len);

#endif // BHARAT_LIB_MSG_CRC_H
