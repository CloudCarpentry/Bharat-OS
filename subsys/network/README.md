# Network Subsystem (subsys/network)

## Purpose

This subsystem defines the **networking and communications contract layer** for Bharat-OS.

It standardizes how:
* applications
* services
* subsystems

interact with networking capabilities.

## Scope

This subsystem currently focuses on:
* defining abstraction boundaries
* aligning existing services (net, netmgr, netstack)
* preparing for future expansion into full communications framework

## Layering Model

Bharat-OS networking is structured into:

1. **Control Plane**
   * services/netmgr
   * interface config, routing, policy

2. **Data Plane**
   * services/netstack
   * packet processing, TCP/IP

3. **Shared Libraries**
   * lib/net
   * lib/packet

4. **Subsystem Contract**
   * subsys/network (this layer)

## Future Expansion

This subsystem will evolve to support:
* multi-bus communication (CAN, LIN, etc.)
* real-time communication (TSN)
* gatewaying between buses and IP
* QoS and traffic classes
* fast dataplane acceleration

## Status

**Status: Stub (Architecture Defined, Implementation Pending)**
