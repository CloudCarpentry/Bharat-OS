#include "socket_table.h"
#include <stddef.h>

static socket_t sockets[MAX_SOCKETS];

void socket_table_init(void) {
    for (int i = 0; i < MAX_SOCKETS; i++) {
        sockets[i].id = -1;
        sockets[i].state = SOCK_STATE_CLOSED;
        sockets[i].rx_callback = NULL;
    }
}

int socket_create(void) {
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i].state == SOCK_STATE_CLOSED) {
            sockets[i].id = i;
            sockets[i].state = SOCK_STATE_BOUND; // Simplified for Phase 2: Create = Bind
            sockets[i].local_ip = SOCK_ANY_IP;
            sockets[i].local_port = 0;
            return i;
        }
    }
    return -1; // No free sockets
}

int socket_bind(int id, uint32_t local_ip, uint16_t local_port) {
    if (id < 0 || id >= MAX_SOCKETS || sockets[id].state == SOCK_STATE_CLOSED) {
        return -1; // Invalid socket
    }

    // Check for conflicts
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (i != id && sockets[i].state == SOCK_STATE_BOUND &&
            sockets[i].local_port == local_port &&
            (sockets[i].local_ip == SOCK_ANY_IP || local_ip == SOCK_ANY_IP || sockets[i].local_ip == local_ip)) {
            return -1; // Port in use
        }
    }

    sockets[id].local_ip = local_ip;
    sockets[id].local_port = local_port;
    return 0;
}

int socket_close(int id) {
    if (id < 0 || id >= MAX_SOCKETS) return -1;
    sockets[id].state = SOCK_STATE_CLOSED;
    sockets[id].id = -1;
    sockets[id].rx_callback = NULL;
    return 0;
}

int socket_set_rx_callback(int id, void (*callback)(int, uint32_t, uint16_t, const uint8_t *, uint16_t)) {
    if (id < 0 || id >= MAX_SOCKETS || sockets[id].state == SOCK_STATE_CLOSED) return -1;
    sockets[id].rx_callback = callback;
    return 0;
}

socket_t *socket_lookup(uint32_t local_ip, uint16_t local_port) {
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i].state == SOCK_STATE_BOUND && sockets[i].local_port == local_port) {
            if (sockets[i].local_ip == SOCK_ANY_IP || sockets[i].local_ip == local_ip) {
                return &sockets[i];
            }
        }
    }
    return NULL;
}

socket_t *socket_get(int id) {
    if (id < 0 || id >= MAX_SOCKETS || sockets[id].state == SOCK_STATE_CLOSED) return NULL;
    return &sockets[id];
}
