# Bharat-OS `subsys/network`

The network and communications subsystem contract layer.

## Architecture & Role
This subsystem exposes native API contracts for network interfaces and profile-driven feature flags.
It acts as the intermediary between the IP core (`services/netstack`), advanced planes (firewalls, routing), and user-space applications.

## Status
* **Current:** Architectural stub and interface definition space.
* **Roadmap:** Integration into full multi-bus communication architecture (TSN, QoS, isolation).
