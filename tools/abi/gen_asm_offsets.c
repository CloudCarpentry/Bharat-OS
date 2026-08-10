#include <stddef.h>
#include "trap.h"
#include "sched/cpu_context.h"
#include "asm_offsets_macros.h"

void asm_offsets(void) {
    DEFINE(BH_TF_GPR0_OFF, offsetof(trap_frame_t, gpr[0]));
    DEFINE(BH_TF_GPR1_OFF, offsetof(trap_frame_t, gpr[1]));
    DEFINE(BH_TF_GPR2_OFF, offsetof(trap_frame_t, gpr[2]));
    DEFINE(BH_TF_GPR3_OFF, offsetof(trap_frame_t, gpr[3]));
    DEFINE(BH_TF_GPR4_OFF, offsetof(trap_frame_t, gpr[4]));
    DEFINE(BH_TF_GPR5_OFF, offsetof(trap_frame_t, gpr[5]));
    DEFINE(BH_TF_GPR6_OFF, offsetof(trap_frame_t, gpr[6]));
    DEFINE(BH_TF_GPR7_OFF, offsetof(trap_frame_t, gpr[7]));
    DEFINE(BH_TF_GPR8_OFF, offsetof(trap_frame_t, gpr[8]));
    DEFINE(BH_TF_GPR9_OFF, offsetof(trap_frame_t, gpr[9]));
    DEFINE(BH_TF_GPR10_OFF, offsetof(trap_frame_t, gpr[10]));
    DEFINE(BH_TF_GPR11_OFF, offsetof(trap_frame_t, gpr[11]));
    DEFINE(BH_TF_GPR12_OFF, offsetof(trap_frame_t, gpr[12]));
    DEFINE(BH_TF_GPR13_OFF, offsetof(trap_frame_t, gpr[13]));
    DEFINE(BH_TF_GPR14_OFF, offsetof(trap_frame_t, gpr[14]));
    DEFINE(BH_TF_GPR15_OFF, offsetof(trap_frame_t, gpr[15]));
    DEFINE(BH_TF_GPR16_OFF, offsetof(trap_frame_t, gpr[16]));
    DEFINE(BH_TF_GPR17_OFF, offsetof(trap_frame_t, gpr[17]));
    DEFINE(BH_TF_GPR18_OFF, offsetof(trap_frame_t, gpr[18]));
    DEFINE(BH_TF_GPR19_OFF, offsetof(trap_frame_t, gpr[19]));
    DEFINE(BH_TF_GPR20_OFF, offsetof(trap_frame_t, gpr[20]));
    DEFINE(BH_TF_GPR21_OFF, offsetof(trap_frame_t, gpr[21]));
    DEFINE(BH_TF_GPR22_OFF, offsetof(trap_frame_t, gpr[22]));
    DEFINE(BH_TF_GPR23_OFF, offsetof(trap_frame_t, gpr[23]));
    DEFINE(BH_TF_GPR24_OFF, offsetof(trap_frame_t, gpr[24]));
    DEFINE(BH_TF_GPR25_OFF, offsetof(trap_frame_t, gpr[25]));
    DEFINE(BH_TF_GPR26_OFF, offsetof(trap_frame_t, gpr[26]));
    DEFINE(BH_TF_GPR27_OFF, offsetof(trap_frame_t, gpr[27]));
    DEFINE(BH_TF_GPR28_OFF, offsetof(trap_frame_t, gpr[28]));
    DEFINE(BH_TF_GPR29_OFF, offsetof(trap_frame_t, gpr[29]));
    DEFINE(BH_TF_GPR30_OFF, offsetof(trap_frame_t, gpr[30]));
    DEFINE(BH_TF_SP_OFF, offsetof(trap_frame_t, sp));
    DEFINE(BH_TF_PC_OFF, offsetof(trap_frame_t, pc));
    DEFINE(BH_TF_CAUSE_OFF, offsetof(trap_frame_t, cause));
    DEFINE(BH_TF_STATUS_OFF, offsetof(trap_frame_t, status));
    DEFINE(BH_TF_TYPE_OFF, offsetof(trap_frame_t, type));
    DEFINE(BH_TF_FROM_USER_OFF, offsetof(trap_frame_t, from_user));
    DEFINE(BH_TF_SIZE, sizeof(trap_frame_t));
    DEFINE(BH_TF_STACK_SIZE, (sizeof(trap_frame_t) + 15U) & ~15U);
    DEFINE(BH_X86_TF_ERROR_OFF, offsetof(bh_x86_64_raw_trap_frame_t, error_code));
    DEFINE(BH_X86_TF_FAULT_ADDR_OFF, offsetof(bh_x86_64_raw_trap_frame_t, fault_addr));
    DEFINE(BH_X86_TF_SIZE, sizeof(bh_x86_64_raw_trap_frame_t));
    DEFINE(BH_ARM64_TF_FAULT_ADDR_OFF, offsetof(bh_arm64_raw_trap_frame_t, fault_addr));
    DEFINE(BH_ARM64_TF_SIZE, sizeof(bh_arm64_raw_trap_frame_t));
    DEFINE(BH_ARM64_TF_STACK_SIZE, (sizeof(bh_arm64_raw_trap_frame_t) + 15U) & ~15U);
    DEFINE(BH_RISCV_TF_FAULT_ADDR_OFF, offsetof(bh_riscv_raw_trap_frame_t, fault_addr));
    DEFINE(BH_RISCV_TF_SIZE, sizeof(bh_riscv_raw_trap_frame_t));
    DEFINE(BH_RISCV_TF_STACK_SIZE, (sizeof(bh_riscv_raw_trap_frame_t) + 15U) & ~15U);
    DEFINE(BH_CTX_REG0_OFF, offsetof(cpu_context_t, regs[0]));
    DEFINE(BH_CTX_REG1_OFF, offsetof(cpu_context_t, regs[1]));
    DEFINE(BH_CTX_REG2_OFF, offsetof(cpu_context_t, regs[2]));
    DEFINE(BH_CTX_REG3_OFF, offsetof(cpu_context_t, regs[3]));
    DEFINE(BH_CTX_REG4_OFF, offsetof(cpu_context_t, regs[4]));
    DEFINE(BH_CTX_REG5_OFF, offsetof(cpu_context_t, regs[5]));
    DEFINE(BH_CTX_REG6_OFF, offsetof(cpu_context_t, regs[6]));
    DEFINE(BH_CTX_REG7_OFF, offsetof(cpu_context_t, regs[7]));
    DEFINE(BH_CTX_REG8_OFF, offsetof(cpu_context_t, regs[8]));
    DEFINE(BH_CTX_REG9_OFF, offsetof(cpu_context_t, regs[9]));
    DEFINE(BH_CTX_REG10_OFF, offsetof(cpu_context_t, regs[10]));
    DEFINE(BH_CTX_REG11_OFF, offsetof(cpu_context_t, regs[11]));
    DEFINE(BH_CTX_REG12_OFF, offsetof(cpu_context_t, regs[12]));
    DEFINE(BH_CTX_PC_OFF, offsetof(cpu_context_t, pc));
    DEFINE(BH_CTX_SP_OFF, offsetof(cpu_context_t, sp));
    DEFINE(BH_CTX_EXT_OFF, offsetof(cpu_context_t, ext));
    DEFINE(BH_CTX_SIZE, sizeof(cpu_context_t));
}
