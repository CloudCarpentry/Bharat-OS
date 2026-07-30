---
title: Gateway Ecus
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - use-cases
see_also:
  - README.md
---
### Gateway ECUs

Bharat-OS can target gateway ECUs because its architecture naturally separates network-facing, vehicle-bus-facing, and diagnostic domains.

Potential value:

- **Capability-mediated networking:** each network interface, bus, or diagnostic endpoint can expose only the rights required by that service.
- **Gateway service isolation:** routing, filtering, diagnostics, OTA, and telemetry can be separate services rather than one privileged monolith.
- **Restartable communication stacks:** CAN, Ethernet, SOME/IP-like services, diagnostics, and update channels can restart independently.
- **Policy outside kernel:** routing, firewall, diagnostic access, and update policy stay in services, keeping the kernel smaller.

Example mapping:

| Gateway ECU function | Bharat-OS mapping |
| --- | --- |
| CAN/CAN-FD bus access | Vehicle stack + bus driver capability |
| Automotive Ethernet | Network stack + netmgr control plane |
| Diagnostics | Isolated diagnostic service |
| Firewall/routing | User-space gateway policy service |
| OTA/update | Secure update service roadmap |
| Fault reporting | Telemetry/diagnostic event service |
