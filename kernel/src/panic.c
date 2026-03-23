#include "panic.h"
#include "fault_diag.h"
#include "hal/hal.h"
#include "kernel.h"
#include "sched.h"
#include "console/console_core.h"
#include <stdint.h>
#include <stddef.h>
#include "lib/string.h"

#ifndef PANIC_RECOVERY_MODE
// 0: Halt (Development), 1: Reboot (Production)
#define PANIC_RECOVERY_MODE 0
#endif

// Symbols defined in linker scripts for the PStore region
extern uint8_t _pstore_start;
extern uint8_t _pstore_end;

static void pstore_write(const char *msg) {
  // Only write if we have space. We do a very basic write without complex
  // logging formatting. Ensure we don't overflow the 4KB region. We can write a
  // magic header or just the string.
  uint8_t *pstore = &_pstore_start;
  size_t max_len = (size_t)(&_pstore_end - &_pstore_start);
  size_t i = 0;

  #define PSTORE_CLEAR_BYTES 256

  // Zero out first to reset (in a real system we'd append or manage a ring
  // buffer)
  size_t clear_len = (max_len < PSTORE_CLEAR_BYTES) ? max_len : PSTORE_CLEAR_BYTES;
  if (clear_len > 0) {
    memset(pstore, 0, clear_len);
  }

  const char *prefix = "KERNEL PANIC: ";
  for (size_t pidx = 0; prefix[pidx] != '\0' && i < max_len - 1; pidx++) {
    pstore[i++] = prefix[pidx];
  }

  if (msg) {
    size_t j = 0;
    while (msg[j] != '\0' && i < max_len - 1) {
      pstore[i++] = msg[j++];
    }
  }
  pstore[i] = '\0';
}

static int g_in_panic = 0;

__attribute__((weak)) void panic_flush_logs(void) {
    // Stub for now. If there's an internal logging buffer, it should be flushed here.
}

void kernel_panic_ex(const panic_context_t *ctx) {
  if (g_in_panic) {
    // Recursive panic, just halt
    while (1) {
      hal_cpu_halt();
    }
  }
  g_in_panic = 1;

  hal_cpu_disable_interrupts();

  // Early flush to push any pending print buffers
  panic_flush_logs();

  panic_context_t local_ctx;
  if (ctx) {
      local_ctx = *ctx;
  } else {
      local_ctx.message = "(no message)";
      local_ctx.cause_str = "unknown";
      local_ctx.cause_code = 0;
      local_ctx.fault_addr = 0;
      local_ctx.ip = 0;
      local_ctx.sp = 0;
      local_ctx.core_id = hal_cpu_get_id();
      local_ctx.thread_id = 0;
      local_ctx.process_id = 0;
      local_ctx.aspace_id = 0;
      local_ctx.last_syscall_nr = 0;
      local_ctx.trap_frame = NULL;
      local_ctx.flags = 0;
  }

  // Backfill context
  local_ctx.core_id = hal_cpu_get_id();

  if (console_current_phase() >= CONSOLE_PHASE_RUNTIME) {
      kthread_t *thread = sched_current_thread();
      if (thread) {
          if (local_ctx.thread_id == 0) {
              local_ctx.thread_id = thread->thread_id;
          }
          if (local_ctx.process_id == 0) {
              local_ctx.process_id = thread->process_id;
          }
          if (local_ctx.aspace_id == 0) {
              local_ctx.aspace_id = thread->process_id;
          }
      }
  }

  const fault_breadcrumbs_t* bc = fault_diag_get_breadcrumbs();
  if (bc && local_ctx.last_syscall_nr == 0) {
      local_ctx.last_syscall_nr = bc->last_syscall_nr;
  }

  panic_emit_header(&local_ctx);
  panic_emit_thread_info(&local_ctx);
  panic_emit_fault_info(&local_ctx);

  if (local_ctx.trap_frame) {
      hal_cpu_dump_trap_frame(local_ctx.trap_frame);
  }

  panic_emit_breadcrumbs(&local_ctx);

  // Dump architecture specific CPU state
  hal_cpu_dump_state();

  // Save to persistent storage
  pstore_write(local_ctx.message);

  panic_flush_logs();

  if (PANIC_RECOVERY_MODE == 1) {
    hal_serial_write("Rebooting system...\n");
    hal_cpu_reboot();
  } else {
    hal_serial_write("System halted.\n");
    while (1) {
      hal_cpu_halt();
    }
  }
}

void kernel_panic(const char *message) {
    panic_context_t ctx = {
        .message = message,
        .cause_str = "panic",
        .cause_code = 0,
        .fault_addr = 0,
        .ip = 0,
        .sp = 0,
        .core_id = hal_cpu_get_id(),
        .thread_id = 0,
        .process_id = 0,
        .aspace_id = 0,
        .last_syscall_nr = 0,
        .trap_frame = NULL,
        .flags = 0
    };
    kernel_panic_ex(&ctx);
}
