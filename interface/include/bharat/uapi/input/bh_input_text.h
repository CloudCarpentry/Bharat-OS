#ifndef BHARAT_UAPI_INPUT_TEXT_H
#define BHARAT_UAPI_INPUT_TEXT_H

#include <stdint.h>

/**
 * @brief Committed text input event.
 *
 * Separate from raw keyboard scancodes, this event represents a committed
 * Unicode codepoint, potentially produced by an IME or simple key translation.
 */
typedef struct bh_text_input_event {
    /** Timestamp in nanoseconds since boot. */
    uint64_t timestamp_ns;

    /** The Unicode codepoint (primary semantic value). */
    uint32_t codepoint;

    /** Active modifiers (shift, ctrl, alt, meta) at time of event. */
    uint32_t modifiers;

    /**
     * UTF-8 encoded representation of the codepoint.
     * Included for transport/display convenience.
     * Must decode exactly to 'codepoint'.
     */
    uint8_t  utf8[4];

    /** Length of the UTF-8 sequence (1-4 bytes). 0 if invalid. */
    uint8_t  utf8_len;
} bh_text_input_event_t;

#endif /* BHARAT_UAPI_INPUT_TEXT_H */
