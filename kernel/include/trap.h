#ifndef BHARAT_TRAP_H
#define BHARAT_TRAP_H

#include <stdint.h>
#include <stddef.h>
#include <bharat/uapi/syscall_nr.h>

typedef enum {
  TRAP_TYPE_SYNC = 0,
  TRAP_TYPE_IRQ = 1,
  TRAP_TYPE_FIQ = 2,
  TRAP_TYPE_SERROR = 3,
} trap_type_t;

typedef struct trap_frame {
  uintptr_t gpr[31];
  uintptr_t sp;
  uintptr_t pc;
  uintptr_t cause;
  uintptr_t status;
  uint32_t type;      // Sync, IRQ, etc.
  uint32_t from_user; // Changed to uint32_t for alignment
} trap_frame_t;

int trap_init(void);
long syscall_dispatch(syscall_id_t id, uintptr_t arg0, uintptr_t arg1,
                      uintptr_t arg2, uintptr_t arg3, uintptr_t arg4,
                      uintptr_t arg5);
long trap_handle(trap_frame_t *frame);

#endif // BHARAT_TRAP_H
