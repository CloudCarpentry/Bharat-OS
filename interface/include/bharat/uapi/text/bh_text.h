#ifndef BHARAT_UAPI_TEXT_H
#define BHARAT_UAPI_TEXT_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Text encoding types supported by Bharat-OS.
 */
typedef enum bh_text_encoding {
    BH_TEXT_ENCODING_ASCII = 0,
    BH_TEXT_ENCODING_UTF8  = 1,
    BH_TEXT_ENCODING_BYTES = 2,
} bh_text_encoding_t;

/**
 * @brief Length-bounded text span.
 *
 * Bharat-OS prefers length-bounded strings over NUL-terminated strings
 * for UAPI boundaries to improve safety and performance.
 */
typedef struct bh_text_span {
    const char *data;
    size_t len;
    bh_text_encoding_t encoding;
} bh_text_span_t;

/**
 * @brief UTF-8 validation and decoding status codes.
 */
typedef enum bh_utf8_status {
    BH_UTF8_OK = 0,
    BH_UTF8_ERR_TRUNCATED,
    BH_UTF8_ERR_INVALID,
    BH_UTF8_ERR_OVERLONG,
    BH_UTF8_ERR_SURROGATE,
    BH_UTF8_ERR_OUT_OF_RANGE,
} bh_utf8_status_t;

/**
 * @brief Maximum text length for common IPC messages to ensure deterministic latency.
 */
#define BHARAT_TEXT_MAX_IPC_SHORT  256
#define BHARAT_TEXT_MAX_IPC_MEDIUM 1024
#define BHARAT_TEXT_MAX_IPC_LONG   4096

#endif /* BHARAT_UAPI_TEXT_H */
