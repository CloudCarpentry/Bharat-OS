#ifndef BHARAT_LIB_TEXT_BH_UTF8_H
#define BHARAT_LIB_TEXT_BH_UTF8_H

#include <bharat/uapi/text/bh_text.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Validates a UTF-8 string of a given length.
 *
 * Performs strict validation according to Bharat-OS contract.
 *
 * @param s The string to validate.
 * @param len The length of the string in bytes.
 * @return BH_UTF8_OK if valid, otherwise an error code.
 */
bh_utf8_status_t bh_utf8_validate(const char *s, size_t len);

/**
 * @brief Decodes the next Unicode codepoint from a UTF-8 string.
 *
 * @param s The string to decode.
 * @param len The remaining length of the string.
 * @param off Pointer to the current offset; will be updated on success.
 * @param cp Pointer to store the decoded codepoint.
 * @return BH_UTF8_OK if success, otherwise an error code.
 */
bh_utf8_status_t bh_utf8_next(const char *s, size_t len, size_t *off, uint32_t *cp);

/**
 * @brief Encodes a Unicode codepoint into UTF-8 bytes.
 *
 * @param cp The codepoint to encode.
 * @param out Buffer to store the encoded UTF-8 (must be at least 4 bytes).
 * @return Number of bytes written (1-4), or 0 on error.
 */
size_t bh_utf8_encode(uint32_t cp, char out[4]);

/**
 * @brief Returns the visual cell width of a codepoint.
 *
 * @param cp The Unicode codepoint.
 * @return Width in cells (0 for combining, 1 for most others).
 */
int bh_text_cell_width(uint32_t cp);

/**
 * @brief Sanitizes text for console output.
 *
 * Strips unsafe escape/control sequences and replaces invalid UTF-8.
 *
 * @param in Input string.
 * @param in_len Input length.
 * @param out Output buffer.
 * @param out_cap Output buffer capacity.
 * @return Number of bytes written to output.
 */
size_t bh_text_sanitize_console(const char *in, size_t in_len, char *out, size_t out_cap);

#endif /* BHARAT_LIB_TEXT_BH_UTF8_H */
