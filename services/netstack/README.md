# Bharat-OS `services/netstack`

## Role

Data plane for networking:
- packet processing
- TCP/IP stack
- transport protocols

The minimum universal IP stack for Bharat-OS.
Handles basic IPv4/IPv6, UDP, TCP, and ARP/NDP.

## Status
* **Current:** Partial implementation.
  - Core protocol modules exist for IPv4, ARP, ICMP, UDP, TCP, socket table, loopback, and Ethernet framing helpers.
  - TCP stack is currently in its early foundational phase (basic segment receive and validation).
  - Checksum and packet-buffer helper utilities are implemented.
  - Main daemon loop is still scaffolded; timer/driver poll integration and full runtime orchestration are in progress.
* **Roadmap:** Full capability-safe, multi-queue IP stack with broader transport/runtime coverage, including a complete TCP state machine and connection tracking.
