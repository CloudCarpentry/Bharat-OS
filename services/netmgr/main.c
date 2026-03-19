#include <stdint.h>
#include <stddef.h>
#include <bharat/net/net.h>

/*
 * Bharat-OS User-Space Network Manager
 *
 * Orchestration, policy, DHCP, and routing table ownership.
 * Control-plane focused. Does not handle line-rate packets.
 */

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    // TODO: Acquire capability to talk to netstack and drivers.
    // TODO: Start interface monitoring loop.

    while(1) {
        // Poll control plane messages (URPC / IPC)
        // Manage link states, DHCP leases, and route tables
    }

    return 0;
}
