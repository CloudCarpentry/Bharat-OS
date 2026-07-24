#include "netbuf.h"

void netbuf_init(netbuf_t *nb) {
    if (!nb) return;
    nb->size = NETBUF_MAX_SIZE;
    nb->head = NETBUF_DEFAULT_HEADROOM;
    nb->tail = NETBUF_DEFAULT_HEADROOM;
    nb->flags = 0;
    memset(nb->buffer, 0, nb->size);
}

uint8_t *netbuf_data(netbuf_t *nb) {
    if (!nb) return NULL;
    return &nb->buffer[nb->head];
}

uint16_t netbuf_len(netbuf_t *nb) {
    if (!nb) return 0;
    return nb->tail - nb->head;
}

uint8_t *netbuf_push(netbuf_t *nb, uint16_t len) {
    if (!nb || len > nb->head) return NULL;
    nb->head -= len;
    return netbuf_data(nb);
}

uint8_t *netbuf_pull(netbuf_t *nb, uint16_t len) {
    if (!nb || len > netbuf_len(nb)) return NULL;
    uint8_t *old_data = netbuf_data(nb);
    nb->head += len;
    return old_data;
}

uint8_t *netbuf_put(netbuf_t *nb, uint16_t len) {
    if (!nb || nb->tail + len > nb->size) return NULL;
    uint8_t *old_tail = &nb->buffer[nb->tail];
    nb->tail += len;
    return old_tail;
}
