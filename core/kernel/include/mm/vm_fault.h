#ifndef BHARAT_VM_FAULT_H
#define BHARAT_VM_FAULT_H

#include <stdint.h>
#include <stdbool.h>

typedef struct vm_fault_info {
    uint64_t space_id;
    uintptr_t addr;

    bool is_user;
    bool is_write;
    bool is_exec;
    bool not_present;
    bool permission_fault;

    uint64_t arch_code;
} vm_fault_info_t;

#endif // BHARAT_VM_FAULT_H
