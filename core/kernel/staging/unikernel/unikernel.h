#ifndef BHARAT_UNIKERNEL_H
#define BHARAT_UNIKERNEL_H

#include "../hal/hal.h"
#include <stdint.h>


/*
 * Bharat-OS Unikernel (Library OS) Definitions
 * [EXPERIMENTAL] — Not part of the v1 kernel core.
 * Optional single-address-space/library-OS deployment mode for statically
 * linked trusted workloads (Cloud-Native microservices).
 */

// In Unikernel mode, we run a single trusted execution context
#ifdef BHARAT_UNIKERNEL_BUILD

// The user application must define this entry point for the microkernel to jump
// to
extern int unikernel_main(int argc, char **argv);

// Bypass VFS/System Calls and access Network/Storage HAL directly
static inline int uk_direct_net_send(void *buffer, uint32_t len) {
  // Jump straight into the VirtIO or DPDK Network driver physically
  return 0; // Stub
}

#endif // BHARAT_UNIKERNEL_BUILD

#endif // BHARAT_UNIKERNEL_H
