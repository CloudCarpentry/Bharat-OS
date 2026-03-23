#ifndef BHARAT_WIN_COMPAT_H
#define BHARAT_WIN_COMPAT_H

#include "subsys.h"
#include "../../kernel/include/capability.h"

/*
 * Bharat-OS Windows/NT Compatibility Layer
 * Requirements Mapping:
 * - Emulates PE format loading and imports.
 * - Simulates the NT kernel syscalls or acts as a Hardware Virtual Machine environment.
 * - NT handles are natively mapped to Bharat-OS Capability tokens.
 * - NT LPC mechanisms are mapped to Bharat-OS Synchronous IPC endpoints.
 */

/*
 * Maps a Windows NT Handle (HANDLE) to a native Bharat-OS Capability.
 * This guarantees strict zero-trust isolation even for Windows binaries.
 */
typedef struct {
    void* nt_handle; // typically a void* or DWORD
    uint32_t backing_capability;
} nt_handle_map_t;

int winnt_subsys_init(subsys_instance_t* env);

/*
 * Register an NT Handle mapping to route Windows operations to a specific capability.
 */
int winnt_map_handle_to_capability(subsys_instance_t* env, void* handle, uint32_t cap);

/*
 * Handle NT LPC (Local Procedure Call) mapped through Bharat-OS Endpoint IPC.
 */
int winnt_handle_lpc(subsys_instance_t* env, void* port_handle, void* message_buffer);

#endif // BHARAT_WIN_COMPAT_H
