#ifndef NETSTACK_SOCKET_TABLE_H
#define NETSTACK_SOCKET_TABLE_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_SOCKETS 32
#define SOCK_ANY_IP 0x00000000

typedef enum {
    SOCK_STATE_CLOSED = 0,
    SOCK_STATE_BOUND
} sock_state_t;

typedef struct {
    int id;
    sock_state_t state;
    uint32_t local_ip;
    uint16_t local_port;
    uint32_t flags; // Placeholder for loopback/raw/internal use

    // Callback slot for incoming packets
    void (*rx_callback)(int id, uint32_t src_ip, uint16_t src_port, const uint8_t *data, uint16_t len);
} socket_t;

/* Initialize the socket table */
void socket_table_init(void);

/* Allocate a new internal socket */
int socket_create(void);

/* Bind a socket to a local IP and port */
int socket_bind(int id, uint32_t local_ip, uint16_t local_port);

/* Close a socket */
int socket_close(int id);

/* Set the RX callback for a socket */
int socket_set_rx_callback(int id, void (*callback)(int, uint32_t, uint16_t, const uint8_t *, uint16_t));

/* Lookup a bound socket by IP and port */
socket_t *socket_lookup(uint32_t local_ip, uint16_t local_port);

/* Get socket by ID */
socket_t *socket_get(int id);

#endif // NETSTACK_SOCKET_TABLE_H
