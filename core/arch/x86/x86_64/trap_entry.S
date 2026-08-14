#include "trap_offsets.inc"
.code64
.section .text

.extern trap_handle

.set BH_X86_STUB_VECTOR_OFF, BH_X86_TF_SIZE + 8
.set BH_X86_STUB_ERROR_OFF,  BH_X86_TF_SIZE + 16
.set BH_X86_HW_RIP_OFF,      BH_X86_TF_SIZE + 24
.set BH_X86_HW_CS_OFF,       BH_X86_TF_SIZE + 32
.set BH_X86_HW_RFLAGS_OFF,   BH_X86_TF_SIZE + 40
.set BH_X86_HW_RSP_OFF,      BH_X86_TF_SIZE + 48

# Macro for ISR without error code
.macro ISR_NOERRCODE num
.global isr\num
isr\num:
    pushq $0          # Dummy error code
    pushq $\num       # Vector number
    jmp trap_common
.endm

# Macro for ISR with error code
.macro ISR_ERRCODE num
.global isr\num
isr\num:
    pushq $\num       # Vector number
    jmp trap_common
.endm

# Common exception handlers (0-31)
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

# Default Timer Handler (Vector 32)
ISR_NOERRCODE 32

# Syscall (Vector 128 = 0x80)
ISR_NOERRCODE 128

.global trap_common
trap_common:
    # We allocate space for x86_trap_frame_t. Size of generic trap_frame_t is 288 bytes.
    # x86_trap_frame_t adds 16 bytes for error_code and cr2, total 304 bytes allocated on top of vector/error_code.
    # struct is: trap_frame_t base, uint64_t error_code, uint64_t cr2
    # One alignment slot follows the C-defined raw frame before stub state.
    subq $(BH_X86_TF_SIZE + 8), %rsp

    # Save General Purpose Registers (map x86_64 to gpr[0..14])
    movq %rax, BH_TF_GPR0_OFF(%rsp)     # gpr[0]
    movq %rdi, BH_TF_GPR1_OFF(%rsp)     # gpr[1]
    movq %rsi, BH_TF_GPR2_OFF(%rsp)    # gpr[2]
    movq %rdx, BH_TF_GPR3_OFF(%rsp)    # gpr[3]
    movq %r10, BH_TF_GPR4_OFF(%rsp)    # gpr[4]
    movq %r8,  BH_TF_GPR5_OFF(%rsp)    # gpr[5]
    movq %r9,  BH_TF_GPR6_OFF(%rsp)    # gpr[6]
    movq %rcx, BH_TF_GPR7_OFF(%rsp)    # gpr[7]
    movq %r11, BH_TF_GPR8_OFF(%rsp)    # gpr[8]
    movq %rbx, BH_TF_GPR9_OFF(%rsp)    # gpr[9]
    movq %rbp, BH_TF_GPR10_OFF(%rsp)    # gpr[10]
    movq %r12, BH_TF_GPR11_OFF(%rsp)    # gpr[11]
    movq %r13, BH_TF_GPR12_OFF(%rsp)    # gpr[12]
    movq %r14, BH_TF_GPR13_OFF(%rsp)   # gpr[13]
    movq %r15, BH_TF_GPR14_OFF(%rsp)   # gpr[14]

    # Hardware/stub state follows the C-defined raw frame.  CPL3 alone adds
    # the hardware RSP/SS pair; the symbolic offsets above describe it.

    # pc
    movq BH_X86_HW_RIP_OFF(%rsp), %rax
    movq %rax, BH_TF_PC_OFF(%rsp)

    # CPL0 hardware frames do not contain RSP/SS.  Derive the interrupted
    # kernel RSP from the frame boundary; only CPL3 frames may read HW_RSP.
    movq BH_X86_HW_CS_OFF(%rsp), %rax
    testb $3, %al
    jz .L_kernel_sp
    movq BH_X86_HW_RSP_OFF(%rsp), %rax
    jmp .L_sp_ready
.L_kernel_sp:
    leaq BH_X86_HW_RSP_OFF(%rsp), %rax
.L_sp_ready:
    movq %rax, BH_TF_SP_OFF(%rsp)

    # cause (vector number)
    movq BH_X86_STUB_VECTOR_OFF(%rsp), %rax
    movq %rax, BH_TF_CAUSE_OFF(%rsp)

    # status (RFLAGS)
    movq BH_X86_HW_RFLAGS_OFF(%rsp), %rax
    movq %rax, BH_TF_STATUS_OFF(%rsp)

    # Determine type: Vector < 32 or Vector == 128 is SYNC (0), else IRQ (1)
    movq BH_X86_STUB_VECTOR_OFF(%rsp), %rax # Vector
    cmpq $32, %rax
    jb .L_is_sync
    cmpq $128, %rax
    je .L_is_sync
    movl $1, BH_TF_TYPE_OFF(%rsp)   # IRQ (type is uint32_t)
    jmp .L_type_done
.L_is_sync:
    movl $0, BH_TF_TYPE_OFF(%rsp)   # SYNC
.L_type_done:

    # from_user: (CS & 3) == 3
    movq BH_X86_HW_CS_OFF(%rsp), %rax # CS
    andq $3, %rax
    cmpq $3, %rax
    sete %al
    movzx %al, %eax
    movl %eax, BH_TF_FROM_USER_OFF(%rsp) # from_user (uint32_t)

    # The kernel GS base is entered and exited exactly once for CPL3 traps.
    testl %eax, %eax
    jz .L_gs_ready
    swapgs
.L_gs_ready:

    # Copy Error Code to x86_trap_frame_t extension
    movq BH_X86_STUB_ERROR_OFF(%rsp), %rax
    movq %rax, BH_X86_TF_ERROR_OFF(%rsp) # error_code

    # Read CR2 and copy to x86_trap_frame_t extension
    movq %cr2, %rax
    movq %rax, BH_X86_TF_FAULT_ADDR_OFF(%rsp) # cr2

    movq %rsp, %rdi
    call trap_handle

    # Reload pc in case it changed
    movq BH_TF_PC_OFF(%rsp), %rax
    movq %rax, BH_X86_HW_RIP_OFF(%rsp)

    cmpl $0, BH_TF_FROM_USER_OFF(%rsp)
    je .L_gs_restored
    swapgs
.L_gs_restored:

    # Restore registers
    movq BH_TF_GPR0_OFF(%rsp), %rax
    movq BH_TF_GPR1_OFF(%rsp), %rdi
    movq BH_TF_GPR2_OFF(%rsp), %rsi
    movq BH_TF_GPR3_OFF(%rsp), %rdx
    movq BH_TF_GPR4_OFF(%rsp), %r10
    movq BH_TF_GPR5_OFF(%rsp), %r8
    movq BH_TF_GPR6_OFF(%rsp), %r9
    movq BH_TF_GPR7_OFF(%rsp), %rcx
    movq BH_TF_GPR8_OFF(%rsp), %r11
    movq BH_TF_GPR9_OFF(%rsp), %rbx
    movq BH_TF_GPR10_OFF(%rsp), %rbp
    movq BH_TF_GPR11_OFF(%rsp), %r12
    movq BH_TF_GPR12_OFF(%rsp), %r13
    movq BH_TF_GPR13_OFF(%rsp), %r14
    movq BH_TF_GPR14_OFF(%rsp), %r15

    addq $(BH_X86_TF_SIZE + 24), %rsp # Skip raw frame, padding, vector, and error code
    iretq
