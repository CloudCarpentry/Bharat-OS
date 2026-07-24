#include "console_internal.h"

// DFA-based UTF-8 decoder based on Bjoern Hoehrmann's algorithm
static const uint8_t utf8d[] = {
  // The first part of the table maps bytes to character classes that
  // to reduce the size of the transition table and create bitmasks.
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,

  // The second part is a transition table that maps a combination
  // of a state of the automaton and a character class to a state.
  0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
  12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
  12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
  12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
  12,36,12,12,12,12,12,12,12,12,12,12,
};

void utf8_decoder_init(utf8_decoder_t* decoder) {
    if (decoder) {
        decoder->state = UTF8_ACCEPT;
        decoder->codepoint = 0;
    }
}

uint32_t utf8_decode(utf8_decoder_t* decoder, uint8_t byte) {
    uint32_t type = utf8d[byte];

    decoder->codepoint = (decoder->state != UTF8_ACCEPT) ?
        (byte & 0x3fu) | (decoder->codepoint << 6) :
        (0xff >> type) & (byte);

    decoder->state = utf8d[256 + decoder->state + type];

    /* Additional hardening for kernel console to prevent overlong, surrogates, and out-of-range */
    if (decoder->state == UTF8_ACCEPT) {
        uint32_t cp = decoder->codepoint;
        bool invalid = false;

        if (cp >= 0xD800 && cp <= 0xDFFF) invalid = true;
        if (cp > 0x10FFFF) invalid = true;

        /* Check for overlong encodings using the Hoehrmann decoder's property.
         * The DFA already handles many overlongs via states, but we can be explicit. */
        if (cp < 0x80 && type != 0) invalid = true;
        if (cp >= 0x80 && cp < 0x800 && type != 2 && type != 10) invalid = true; // simplified

        if (invalid) {
            decoder->state = UTF8_REJECT;
        }
    }

    return decoder->state;
}
