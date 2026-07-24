#ifndef BHARAT_NETWORK_CONTRACTS_H
#define BHARAT_NETWORK_CONTRACTS_H

#include "version.h"

/*
 * Bharat-OS Subsystem Network Contracts
 *
 * Top-level stable subsystem contract definitions:
 * - ABI Version limits and compatibility.
 * - Service names for discovery.
 * - Architectural boundaries and ownership models.
 */

#define BNET_MGR_SERVICE_NAME   "bharat.netmgr"
#define BNET_STACK_SERVICE_NAME "bharat.netstack"
#define BNET_FAST_SERVICE_NAME  "bharat.netfast"

/*
 * Object Model Notes:
 * - netmgr: Owns the control plane. Manages interface lifecycles, assigns configurations,
 *           maintains routing and policy models.
 * - netstack: Owns the baseline L2/L3/L4 data plane processing and socket layer.
 * - netfast: Optional high-performance fast path. Takes queue registrations directly
 *            and bypasses slow paths for appliance profiles.
 * - drivers: Hardware abstractions (e.g. virtio_net). Handled by IO subsystem and mapped
 *            to netstack or netfast via memory-mapped queues.
 * - clients: Request sockets, establish streams. Bounded by capabilities and namespace routing.
 */

#endif // BHARAT_NETWORK_CONTRACTS_H
