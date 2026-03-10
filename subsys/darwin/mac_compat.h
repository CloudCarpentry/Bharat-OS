#ifndef BHARAT_MAC_COMPAT_H
#define BHARAT_MAC_COMPAT_H

#include "subsys.h"
#include "../../kernel/include/capability.h"

/*
 * Bharat-OS macOS / Darwin (XNU) Compatibility Layer
 * Requirements Mapping:
 * - Emulates Mach-O binary loading.
 * - Maps Mach ports directly to Bharat-OS IPC endpoints and capabilities.
 * - Supports Darwin's process and thread management via native primitives.
 */

/*
 * Maps a Mach Port (mach_port_t) to a native Bharat-OS Capability.
 * Mach ports have inherent send/receive rights which map natively
 * to the Bharat-OS Capability system.
 */
typedef struct {
    uint32_t mach_port;
    uint32_t backing_capability; // Typically an Endpoint (ep_t) with SEND/RECV perms
} mach_port_map_t;

int mac_subsys_init(subsys_instance_t* env);

/*
 * Register a Mach Port mapping to an underlying capability.
 */
int mac_map_port_to_capability(subsys_instance_t* env, uint32_t port, uint32_t cap);

/*
 * Process a Mach message using Bharat-OS fast-path URPC or synchronous IPC.
 */
int mac_handle_mach_msg(subsys_instance_t* env, uint32_t port, void* msg_buffer, int options);

#endif // BHARAT_MAC_COMPAT_H
