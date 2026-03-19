# subsys/network

**Status:** Stub / Baseline Target

This is the network personality/subsystem contract layer. It defines how applications and user-space domains view networking operations in Bharat-OS.

## Scope
* Native socket/session model.
* Interface objects and capability surfaces.
* Route and policy abstractions.
* Service discovery hooks and profile feature flags (Embedded, Automotive, Cloud, Edge).

This directory does **not** contain the TCP/IP implementation (which lives in `services/netstack`) or interface lifecycle orchestration (which lives in `services/netmgr`). Instead, it acts as the stable contract API and RPC boundary.
