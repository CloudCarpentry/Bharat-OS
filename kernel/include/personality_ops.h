#pragma once

struct kthread;
struct trap_frame;
struct trap_info;

typedef struct personality_ops {
    long (*handle_syscall)(struct kthread *thread,
                           struct trap_frame *frame,
                           const struct trap_info *info);

    int (*handle_user_fault)(struct kthread *thread,
                             struct trap_frame *frame,
                             const struct trap_info *info);

    int (*map_fault_to_signal)(const struct trap_info *info);
} personality_ops_t;
