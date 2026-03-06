#ifndef BHARAT_SANDBOX_H
#define BHARAT_SANDBOX_H

#include <stdint.h>
#include "sched.h"

/*
 * Bharat-OS Native Sandboxing & Capability Framework
 * Equivalent to Linux Namespaces, Cgroups, and Seccomp.
 */

// Resource Limits (Cgroups)
typedef struct {
    uint64_t max_memory_bytes;
    uint32_t cpu_quota_percent;
    uint32_t max_processes;
    uint64_t network_bw_bps;
} resource_limits_t;

// Isolation Namespaces
typedef struct {
    int isolate_pid;
    int isolate_network;
    int isolate_mount;
    int isolate_ipc;
} namespace_config_t;

// Capability Security Context
typedef struct {
    uint64_t allow_syscalls_mask;
    uint64_t allow_port_io;
    int allow_raw_sockets;
    uint32_t sandbox_id;
    
    namespace_config_t namespaces;
    resource_limits_t limits;
} sandbox_ctx_t;

// Create a new strict security capability context
sandbox_ctx_t* sandbox_create_context(void);

// Apply a sandbox context to a process (Containers)
int sandbox_apply(kprocess_t* process, sandbox_ctx_t* ctx);

// Verify if a process has a specific capability 
int sandbox_check_capability(kprocess_t* process, uint64_t requested_capability);

#endif // BHARAT_SANDBOX_H
