#pragma once

#include <stdint.h>

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define htons(x) ((uint16_t)((((x) >> 8) & 0xff) | (((x) & 0xff) << 8)))
#define ntohs(x) htons(x)
#define htonl(x) ((uint32_t)((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >> 8) | (((x) & 0x0000ff00) << 8) | (((x) & 0x000000ff) << 24)))
#define ntohl(x) htonl(x)
#else
#define htons(x) (x)
#define ntohs(x) (x)
#define htonl(x) (x)
#define ntohl(x) (x)
#endif
