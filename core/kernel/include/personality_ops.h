#pragma once
#include "trap_types.h"

struct bh_thread;
typedef struct bh_thread bh_thread_t;


// Forward declaration of trap_frame_t without typedef redefinition issues.
struct trap_frame;
typedef struct trap_frame trap_frame_t;

typedef struct personality_ops {
    long (*handle_syscall)(struct bh_thread *thread,
                           trap_frame_t *frame,
                           const trap_info_t *info);

    int (*handle_user_fault)(struct bh_thread *thread,
                             trap_frame_t *frame,
                             const trap_info_t *info);

    int (*map_fault_to_signal)(const trap_info_t *info);

    long (*normalize_syscall_return)(long result);
} personality_ops_t;
