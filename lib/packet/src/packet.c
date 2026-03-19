#include <bharat/packet/packet.h>
#include <stdbool.h>

#define PACKET_POOL_SIZE 256
#define PACKET_BUFFER_SIZE 2048
#define DEFAULT_HEADROOM 256

static packet_buf_t pkt_pool[PACKET_POOL_SIZE];
static uint8_t pkt_data_pool[PACKET_POOL_SIZE][PACKET_BUFFER_SIZE];
static bool pkt_allocated[PACKET_POOL_SIZE];

void libpacket_init(void) {
    for (int i = 0; i < PACKET_POOL_SIZE; i++) {
        pkt_allocated[i] = false;
        pkt_pool[i].total_len = PACKET_BUFFER_SIZE;
    }
}

packet_buf_t *packet_alloc(void) {
    for (int i = 0; i < PACKET_POOL_SIZE; i++) {
        if (!pkt_allocated[i]) {
            pkt_allocated[i] = true;
            packet_buf_t *pkt = &pkt_pool[i];
            pkt->data = pkt_data_pool[i] + DEFAULT_HEADROOM;
            pkt->head_len = DEFAULT_HEADROOM;
            pkt->tail_len = PACKET_BUFFER_SIZE - DEFAULT_HEADROOM;
            pkt->data_len = 0;
            pkt->flags = 0;
            pkt->refcount = 1;
            pkt->next = NULL;
            pkt->owner_ctx = NULL;
            return pkt;
        }
    }
    return NULL; // Pool exhausted
}

void packet_free(packet_buf_t *pkt) {
    if (!pkt) return;

    // Calculate index to free
    int index = pkt - pkt_pool;
    if (index >= 0 && index < PACKET_POOL_SIZE) {
        pkt_allocated[index] = false;
    }
}

void packet_ref(packet_buf_t *pkt) {
    if (pkt) {
        pkt->refcount++;
    }
}

void packet_unref(packet_buf_t *pkt) {
    if (pkt) {
        if (pkt->refcount > 0) {
            pkt->refcount--;
            if (pkt->refcount == 0) {
                packet_free(pkt);
            }
        }
    }
}
