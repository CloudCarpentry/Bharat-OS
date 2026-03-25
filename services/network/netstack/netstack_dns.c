#include "net/dns_client.h"
#include <stdio.h>

void netstack_dns_init(ipv4_addr_t server) {
    dns_client_init(server);
    printf("[netstack_dns] Initialized DNS client\n");
}

void netstack_dns_tick(void) {
    dns_client_tick();
}
