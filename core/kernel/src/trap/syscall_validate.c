#include "trap/syscall_context.h"
#include "kernel/status.h"
#include "hal/hal.h"
#include <stddef.h>

kstatus_t bh_syscall_table_validate(const bh_personality_syscall_table_t *table) {
    if (!table || !table->table) return K_ERR_INVALID_ARG;

    for (uint32_t i = 0; i <= table->max_syscall_nr; i++) {
        const bh_syscall_meta_t *desc = &table->table[i];

        // Skip empty slots if any (though usually they should be stubs)
        if (desc->handler == NULL) {
            continue;
        }

        // 1. desc.nr == index
        if (desc->nr != i) {
            return K_ERR_BAD_STATE;
        }

        // 2. desc.arg_count <= 6
        if (desc->arg_count > 6) {
            return K_ERR_BAD_STATE;
        }

        // 3. desc.class is valid
        if (desc->class_id == BH_SYS_CLASS_NONE || desc->class_id > BH_SYS_CLASS_SYSTEM) {
            return K_ERR_BAD_STATE;
        }

        // 4. FAST syscall must not be BLOCKING
        if ((desc->flags & BH_SYSCALL_F_FAST) && (desc->flags & BH_SYSCALL_F_BLOCKING)) {
            return K_ERR_BAD_STATE;
        }

        // 5. FAST syscall must not use usercopy
        if ((desc->flags & BH_SYSCALL_F_FAST) &&
            (desc->flags & (BH_SYSCALL_F_USER_READ | BH_SYSCALL_F_USER_WRITE))) {
            return K_ERR_BAD_STATE;
        }

        // 6. CAP_REQUIRED syscall must define capability metadata
        if (desc->flags & BH_SYSCALL_F_CAP_REQUIRED) {
            if (desc->cap_arg_index == BH_SYS_CAP_INDEX_NONE) {
                return K_ERR_BAD_STATE;
            }
            if (desc->cap_arg_index >= desc->arg_count) {
                return K_ERR_BAD_STATE;
            }
        }
    }

    return K_OK;
}
