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


// Filesystem/storage classes managed by sandbox policy
typedef enum {
    SANDBOX_STORAGE_FS = 0,
    SANDBOX_STORAGE_BLOCK,
    SANDBOX_STORAGE_BLOB,
    SANDBOX_STORAGE_TMPFS,
    SANDBOX_STORAGE_NETWORK_FS
} sandbox_storage_class_t;

// Path capability mask bits
#define SANDBOX_PATH_READ      (1U << 0)
#define SANDBOX_PATH_WRITE     (1U << 1)
#define SANDBOX_PATH_EXEC      (1U << 2)
#define SANDBOX_PATH_CREATE    (1U << 3)
#define SANDBOX_PATH_DELETE    (1U << 4)
#define SANDBOX_PATH_METADATA  (1U << 5)

#define SANDBOX_MAX_PATH_RULES 16

// Per-prefix filesystem rule
typedef struct {
    char prefix[128];
    uint32_t allowed_ops_mask;
} sandbox_path_rule_t;

// Filesystem policy for mount/path/storage restrictions
typedef struct {
    uint32_t allowed_storage_classes_mask;
    int enforce_noexec_on_writable_mounts;
    int enforce_nodev_on_writable_mounts;
    int enforce_nosuid_on_writable_mounts;
    uint32_t path_rule_count;
    sandbox_path_rule_t path_rules[SANDBOX_MAX_PATH_RULES];
} sandbox_fs_policy_t;

// Capability Security Context
typedef struct {
    uint64_t allow_syscalls_mask;
    uint64_t allow_port_io;
    int allow_raw_sockets;
    uint32_t sandbox_id;
    
    namespace_config_t namespaces;
    resource_limits_t limits;
    sandbox_fs_policy_t fs_policy;
} sandbox_ctx_t;

// Create a new strict security capability context
sandbox_ctx_t* sandbox_create_context(void);

// Apply a sandbox context to a process (Containers)
int sandbox_apply(kprocess_t* process, sandbox_ctx_t* ctx);

// Verify if a process has a specific capability 
int sandbox_check_capability(kprocess_t* process, uint64_t requested_capability);

// Add or replace a path capability rule in a sandbox context.
int sandbox_allow_path(sandbox_ctx_t* ctx, const char* prefix, uint32_t allowed_ops_mask);

// Allow a storage class (filesystem/block/blob/tmpfs/network fs) in the sandbox.
int sandbox_allow_storage_class(sandbox_ctx_t* ctx, sandbox_storage_class_t storage_class);

#endif // BHARAT_SANDBOX_H
