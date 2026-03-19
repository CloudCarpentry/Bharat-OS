#include <stdint.h>
#include <stddef.h>
#include <bharat/net/net.h>
#include <bharat/packet/packet.h>

/*
 * Bharat-OS Minimum Universal IP Stack
 *
 * Provides Ethernet, ARP, IP, UDP, and TCP.
 * Sits below the socket layer and above the NIC drivers.
 */

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    // TODO: Register packet ring capabilities with NICs.
    // TODO: Setup communication channel with netmgr.

    while(1) {
        // Dequeue packets from NIC RX rings
        // Run protocol parsers (L2 -> L3 -> L4)
        // Deliver to socket endpoints
    }

    return 0;
}
