#pragma once

#include "trap_types.h"
#include "trap.h"

int trap_dispatch(trap_frame_t *frame, const trap_info_t *info);
int trap_handle_fault(trap_frame_t *frame, const trap_info_t *info);
long trap_dispatch_syscall(trap_frame_t *frame, const trap_info_t *info);
