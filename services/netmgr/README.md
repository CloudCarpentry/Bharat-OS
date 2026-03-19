# services/netmgr (Network Orchestration & Policy)

**Status:** Stub / Baseline Target

This service owns the orchestration and policy plane for the Bharat-OS network stack.

## Scope
* Interface lifecycle (bringing links up/down).
* DHCP and static IP configuration coordination.
* Routing table ownership and policy distribution.
* Firewall rule installation (to be enforced by the fast path or stack).
* Profile activation and telemetry aggregation.

This daemon stays message-heavy and capability-rich. It does **not** process packets in the data plane (see `services/netstack`).
