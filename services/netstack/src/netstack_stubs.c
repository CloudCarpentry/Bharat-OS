#include <stddef.h>

void *memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    while(n--) *p++ = (unsigned char)c;
    return s;
}

void *memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    while(n--) *d++ = *s++;
    return dest;
}

// Stubs for driver_virtio_adapter
void virtio_net_init() {}
int virtio_net_probe() { return 0; }
int virtio_net_bind() { return 0; }
int virtio_net_start() { return 0; }
int virtio_net_tx() { return 0; }
