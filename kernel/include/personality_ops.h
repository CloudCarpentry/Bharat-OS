#pragma once

struct bh_thread;
#include <sched/sched.h>

// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
#include "trap_types.h"

// Forward declaration of trap_frame_t without typedef redefinition issues.
struct trap_frame;
typedef struct trap_frame trap_frame_t;

typedef struct personality_ops {
    long (*handle_syscall)(bh_thread_t *thread,
                           trap_frame_t *frame,
                           const trap_info_t *info);

    int (*handle_user_fault)(bh_thread_t *thread,
                             trap_frame_t *frame,
                             const trap_info_t *info);

    int (*map_fault_to_signal)(const trap_info_t *info);
} personality_ops_t;
