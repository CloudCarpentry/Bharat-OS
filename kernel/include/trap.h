#ifndef BHARAT_TRAP_H
#define BHARAT_TRAP_H

#include <stdint.h>

#include <bharat/uapi/syscall_nr.h>

typedef enum {
  TRAP_TYPE_SYNC = 0,
  TRAP_TYPE_IRQ = 1,
  TRAP_TYPE_FIQ = 2,
  TRAP_TYPE_SERROR = 3,
} trap_type_t;

typedef struct trap_frame {
  uint64_t gpr[31];
  uint64_t sp;
  uint64_t pc;
  uint64_t cause;
  uint64_t status;
  uint32_t type;      // Sync, IRQ, etc.
  uint32_t from_user; // Changed to uint32_t for alignment
} trap_frame_t;

int trap_init(void);
long syscall_dispatch(syscall_id_t id, uint64_t arg0, uint64_t arg1,
                      uint64_t arg2, uint64_t arg3, uint64_t arg4,
                      uint64_t arg5);
long trap_handle(trap_frame_t *frame);

#endif // BHARAT_TRAP_H
