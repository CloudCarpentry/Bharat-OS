#ifndef BHARAT_MM_FAULT_H
#define BHARAT_MM_FAULT_H

#include <stdint.h>

#include "aspace.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VM_FAULT_READ = 1u << 0,
    VM_FAULT_WRITE = 1u << 1,
    VM_FAULT_EXEC = 1u << 2,
    VM_FAULT_USER = 1u << 3,
} vm_fault_access_t;

typedef enum {
    VM_FAULT_RESOLVED = 0,
    VM_FAULT_RETRY = 1,
    VM_FAULT_KILL = 2,
    VM_FAULT_PANIC = 3,
} vm_fault_result_t;

typedef struct {
    vm_address_space_t *aspace;
    uint64_t fault_addr;
    uint32_t access;
    uint32_t arch_code;
} vm_fault_event_t;

vm_fault_result_t vm_handle_fault(const vm_fault_event_t *event);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_MM_FAULT_H
