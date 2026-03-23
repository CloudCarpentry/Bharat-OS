#ifndef BHARAT_KERNEL_ERROR_INFO_H
#define BHARAT_KERNEL_ERROR_INFO_H

#include "kernel/status.h"
#include <stdint.h>

/*
 * Bharat-OS Diagnostic Error Record
 * For deep debugging, crash dumps, and tracing.
 * This structure should not be leaked in normal UAPI but is used
 * heavily by the kernel's fault domains, telemetry, and panic handlers.
 */

typedef struct {
    kstatus_t code;
    uint16_t  domain;
    uint16_t  module;
    uint32_t  flags;
    uintptr_t arg0;
    uintptr_t arg1;
    uintptr_t arg2;
} kerror_info_t;

#endif // BHARAT_KERNEL_ERROR_INFO_H
