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

/* x86 appends fault registers to the normalized save area.  Keeping this
 * definition in C makes the generated assembler offsets authoritative. */
typedef struct bh_x86_64_raw_trap_frame {
  trap_frame_t base;
  uint64_t error_code;
  uint64_t fault_addr;
} bh_x86_64_raw_trap_frame_t;

typedef struct bh_arm64_raw_trap_frame {
  trap_frame_t base;
  uintptr_t fault_addr;
} bh_arm64_raw_trap_frame_t;

typedef struct bh_riscv_raw_trap_frame {
  trap_frame_t base;
  uintptr_t fault_addr;
} bh_riscv_raw_trap_frame_t;

_Static_assert(__builtin_offsetof(trap_frame_t, gpr) == 0, "gpr offset mismatch");
_Static_assert(__builtin_offsetof(trap_frame_t, sp) == 31 * sizeof(uintptr_t), "sp offset mismatch");
_Static_assert(__builtin_offsetof(trap_frame_t, pc) == 32 * sizeof(uintptr_t), "pc offset mismatch");
_Static_assert(__builtin_offsetof(trap_frame_t, cause) == 33 * sizeof(uintptr_t), "cause offset mismatch");
_Static_assert(__builtin_offsetof(trap_frame_t, status) == 34 * sizeof(uintptr_t), "status offset mismatch");
_Static_assert(__builtin_offsetof(trap_frame_t, type) == 35 * sizeof(uintptr_t), "type offset mismatch");
_Static_assert(__builtin_offsetof(trap_frame_t, from_user) == 35 * sizeof(uintptr_t) + 4, "from_user offset mismatch");
_Static_assert(__builtin_offsetof(bh_x86_64_raw_trap_frame_t, error_code) >= sizeof(trap_frame_t),
               "x86 error-code overlaps common frame");

int trap_init(void);
long syscall_dispatch(syscall_id_t id, uintptr_t arg0, uintptr_t arg1,
                      uintptr_t arg2, uintptr_t arg3, uintptr_t arg4,
                      uintptr_t arg5);
long trap_handle(trap_frame_t *frame);

#endif // BHARAT_TRAP_H
