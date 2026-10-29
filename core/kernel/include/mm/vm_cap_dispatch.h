#ifndef BHARAT_VM_CAP_DISPATCH_H
#define BHARAT_VM_CAP_DISPATCH_H
#include <stdint.h>
#include "kernel/status.h"

kstatus_t vm_space_cap_invoke(uintptr_t cap_id, uintptr_t opcode,
                              uintptr_t request, uintptr_t reserved);
#endif
