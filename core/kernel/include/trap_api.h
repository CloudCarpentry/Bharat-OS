#pragma once

#include "kernel/status.h"
#include "trap.h"
#include "trap_types.h"

kstatus_t bh_trap_decode(const trap_frame_t *frame, bh_trap_context_t *out);

int trap_dispatch(trap_frame_t *frame, const trap_info_t *info);
int trap_handle_fault(trap_frame_t *frame, const trap_info_t *info);
long trap_dispatch_syscall(trap_frame_t *frame, const trap_info_t *info);
