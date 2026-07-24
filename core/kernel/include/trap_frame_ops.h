#pragma once

#include "trap.h"

static inline uintptr_t trap_frame_get_ip(const trap_frame_t *frame) {
    return frame->pc;
}

static inline uintptr_t trap_frame_get_sp(const trap_frame_t *frame) {
    return frame->sp;
}

static inline uintptr_t trap_frame_get_syscall_no(const trap_frame_t *frame) {
    return frame->gpr[0];
}

static inline uintptr_t trap_frame_get_arg0(const trap_frame_t *frame) {
    return frame->gpr[1];
}

static inline uintptr_t trap_frame_get_arg1(const trap_frame_t *frame) {
    return frame->gpr[2];
}

static inline uintptr_t trap_frame_get_arg2(const trap_frame_t *frame) {
    return frame->gpr[3];
}

static inline uintptr_t trap_frame_get_arg3(const trap_frame_t *frame) {
    return frame->gpr[4];
}

static inline uintptr_t trap_frame_get_arg4(const trap_frame_t *frame) {
    return frame->gpr[5];
}

static inline uintptr_t trap_frame_get_arg5(const trap_frame_t *frame) {
    return frame->gpr[6];
}

static inline void trap_frame_set_retval(trap_frame_t *frame, uintptr_t value) {
    frame->gpr[0] = value;
}
