#include <stddef.h>
#include "trap.h"
#include "asm_offsets_macros.h"

void asm_offsets(void) {
    DEFINE(BH_TF_GPR0_OFF, offsetof(trap_frame_t, gpr[0]));
    DEFINE(BH_TF_SP_OFF, offsetof(trap_frame_t, sp));
    DEFINE(BH_TF_PC_OFF, offsetof(trap_frame_t, pc));
    DEFINE(BH_TF_CAUSE_OFF, offsetof(trap_frame_t, cause));
    DEFINE(BH_TF_STATUS_OFF, offsetof(trap_frame_t, status));
    DEFINE(BH_TF_TYPE_OFF, offsetof(trap_frame_t, type));
    DEFINE(BH_TF_FROM_USER_OFF, offsetof(trap_frame_t, from_user));
    DEFINE(BH_TF_SIZE, sizeof(trap_frame_t));
}
