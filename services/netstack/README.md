# services/netstack (Minimum Universal IP Stack)

**Status:** Stub / Baseline Target

This service provides the baseline IP stack for Bharat-OS. It serves as the standard path for devices that do not require high-speed bypass (like an appliance dataplane).

## Scope
* Ethernet, ARP, NDP.
* IPv4, IPv6, ICMP.
* UDP, TCP.
* Fragmentation and reassembly.
* Base forwarding path and socket endpoints.

This service is distinct from the control plane (`services/netmgr`) and the optional zero-copy dataplane (`services/netfast`).
