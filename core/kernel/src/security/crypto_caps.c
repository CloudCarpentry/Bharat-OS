#include <stdint.h>
#include <bharat/uapi/sys_errno.h>
#include <security/crypto_caps.h>

/**
 * @file crypto_caps.c
 * @brief Capability check stubs for the Cryptographic Provider Registry.
 *
 * In a real Bharat-OS implementation, these functions would interface
 * with the formal kernel capability object system (e.g., capability.c).
 * For this isolated implementation, we use a simple thread-local or
 * global mask to simulate granting and checking capabilities.
 */

/* Simulate a per-process capability mask */
static uint32_t current_process_caps = 0;

int crypto_cap_check(uint32_t required_cap)
{
    if ((current_process_caps & required_cap) == required_cap) {
        return 0; // Success
    }
    return -SYS_EPERM; // Permission denied
}

void crypto_cap_grant(uint32_t cap)
{
    current_process_caps |= cap;
}

void crypto_cap_revoke(uint32_t cap)
{
    current_process_caps &= ~cap;
}
