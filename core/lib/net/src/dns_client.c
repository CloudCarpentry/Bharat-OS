#include "net/dns_client.h"
#include "net/udp.h"
#include <string.h>
#include <stdio.h>

static ipv4_addr_t dns_server;

int dns_client_init(ipv4_addr_t server_ip) {
    dns_server = server_ip;
    printf("[dns_client] Initialized with server %d.%d.%d.%d\n",
        (dns_server.addr >> 24) & 0xff, (dns_server.addr >> 16) & 0xff,
        (dns_server.addr >> 8) & 0xff, dns_server.addr & 0xff);
    return 0;
}

int dns_resolve(const char *name, dns_callback_t cb, void *arg) {
    (void)name;
    (void)cb;
    (void)arg;

    printf("[dns_client] Resolving '%s' (not yet fully implemented)\n", name);

    // In a real implementation:
    // 1. Build DNS Query packet
    // 2. Wrap in UDP
    // 3. Send to DNS Server IP
    // 4. Register callback in pending requests list

    return -1; // Not yet implemented
}

void dns_client_tick(void) {
    // Handle request timeouts and retransmissions
}
