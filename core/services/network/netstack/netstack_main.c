#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "net/netif.h"
#include "net/route.h"

// Forward declarations
void netstack_rx_init(void);
void netstack_tx_init(void);
void netstack_port_init(void);

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    printf("[netstack] Starting minimal network dataplane service...\n");

    // Initialize components
    route_init();
    netstack_rx_init();
    netstack_tx_init();
    netstack_port_init();

    // Main loop
    while (1) {
        // Here we would typically block on uRPC, IRQs, or select/poll
        // For now, simple delay to avoid high CPU
        sleep(1);
    }

    return 0;
}
