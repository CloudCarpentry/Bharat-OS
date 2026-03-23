#pragma once

struct kthread;
typedef struct kthread kthread_t;

#include "trap_types.h"

// Forward declaration of trap_frame_t without typedef redefinition issues.
struct trap_frame;
typedef struct trap_frame trap_frame_t;

typedef struct personality_ops {
    long (*handle_syscall)(kthread_t *thread,
                           trap_frame_t *frame,
                           const trap_info_t *info);

    int (*handle_user_fault)(kthread_t *thread,
                             trap_frame_t *frame,
                             const trap_info_t *info);

    int (*map_fault_to_signal)(const trap_info_t *info);
} personality_ops_t;
