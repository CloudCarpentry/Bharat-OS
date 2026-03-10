#ifndef BHARAT_ANDROID_SECURITY_H
#define BHARAT_ANDROID_SECURITY_H

#include "android_personality.h"

/*
 * Phase 4: SELinux-style Distributed Mediation Hooks
 *
 * Maps Android MAC policies (SELinux) to Bharat-OS capability checks.
 * In a multikernel, capability checks happen locally via cached policies,
 * with misses escalated to a central or authoritative core.
 */

/**
 * @brief Android Security Context Label (SID).
 */
typedef uint32_t android_sec_sid_t;

/**
 * @brief Represents an SELinux security context string and its SID.
 */
typedef struct {
    android_sec_sid_t sid;
    char context[256]; // e.g., "u:r:system_server:s0"
} android_sec_context_t;

/**
 * @brief Distributed Mediation Hook: Binder Transaction Check
 *
 * Checks if the caller SID has permission to send a transaction to the target SID.
 * Uses a local cache first, resolving remotely if necessary.
 *
 * @param caller_sid The SID of the client thread.
 * @param target_sid The SID of the target service process.
 * @param action The SELinux action/permission requested (e.g., 'call', 'transfer').
 * @return 0 on success (allowed), < 0 on failure (denied).
 */
int android_sec_check_binder_access(android_sec_sid_t caller_sid, android_sec_sid_t target_sid, uint32_t action);

/**
 * @brief Distributed Mediation Hook: Shared Memory Access Check
 *
 * Ensures the process has the capability to map an ashmem/FMQ region.
 *
 * @param process_sid The SID of the requesting process.
 * @param obj The memory object identity.
 * @param prot The requested memory protections (PROT_READ, PROT_WRITE).
 * @return 0 on success, < 0 on error.
 */
int android_sec_check_memory_access(android_sec_sid_t process_sid, android_logical_obj_t* obj, int prot);

/**
 * @brief Transitions a process into a new security domain.
 *
 * Used extensively by Android `init` when spawning daemons.
 *
 * @param current_sid The current security context.
 * @param new_sid The target security context.
 * @return 0 on success, < 0 on error.
 */
int android_sec_transition_domain(android_sec_sid_t current_sid, android_sec_sid_t new_sid);

#endif // BHARAT_ANDROID_SECURITY_H
