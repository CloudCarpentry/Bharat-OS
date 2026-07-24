#pragma once

#include <stdint.h>
#include <stddef.h>

static inline uint16_t net_checksum(const void *buf, size_t len) {
    const uint16_t *data = (const uint16_t *)buf;
    uint32_t sum = 0;
    while (len > 1) {
        sum += *data++;
        len -= 2;
    }
    if (len > 0) {
        sum += *(const uint8_t *)data;
    }
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    return (uint16_t)(~sum);
}
