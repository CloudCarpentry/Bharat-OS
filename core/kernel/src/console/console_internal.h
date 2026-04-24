#ifndef BHARAT_CONSOLE_INTERNAL_H
#define BHARAT_CONSOLE_INTERNAL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* UTF-8 Decoder State Machine */
#define UTF8_ACCEPT 0
#define UTF8_REJECT 1

typedef struct {
    uint32_t state;
    uint32_t codepoint;
} utf8_decoder_t;

void utf8_decoder_init(utf8_decoder_t* decoder);
uint32_t utf8_decode(utf8_decoder_t* decoder, uint8_t byte);

#endif /* BHARAT_CONSOLE_INTERNAL_H */
