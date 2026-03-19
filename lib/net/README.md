# lib/net (Network Common Library)

**Status:** Stub / Baseline Target

This library defines the common network contracts and types for Bharat-OS. It is intended to be shared across `subsys/network`, `services/netstack`, `services/netmgr`, and user-space network drivers.

## Scope
* Interface identifiers (`if_id_t`).
* Address family enumerations (IPv4, IPv6).
* Link state enums (UP, DOWN, UNKNOWN).
* Route and endpoint placeholder structures.

This library does *not* contain protocol implementations, socket logic, or buffer management (see `lib/packet`).
