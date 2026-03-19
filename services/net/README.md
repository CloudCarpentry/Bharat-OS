# services/net (Legacy Network Monolith)

**Status:** Legacy / Transitional

This is the original monolithic placeholder for the Bharat-OS network service.
It attempted to combine routing policy, interface lifecycle, and data-plane packet processing into a single user-space domain.

This design does not scale to high-speed profiles (Edge Gateways/Cloud Nodes) and is a single-point-of-failure for the whole network stack.

## Deprecation Notice

This directory is kept temporarily to ensure existing build paths and experiments remain functional during the transition.
It will eventually be fully decomposed into:
* `services/netmgr` (Policy, orchestration, routing table ownership).
* `services/netstack` (Universal IP stack, L3/L4 baseline).
* `services/netfast` (Roadmap fast path / appliance dataplane).
