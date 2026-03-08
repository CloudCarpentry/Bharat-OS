.code64
.section .text

.extern trap_handle

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
    # Build trap_frame_t. Size is 280 bytes.
    # struct is: uint64_t gpr[31], sp, pc, cause, status, from_user(8bit)
    subq $280, %rsp

    # Store General Purpose Registers (map x86_64 to gpr[0..15])
    # syscall_dispatch maps id to arg0, etc:
    # arg0=rdi, arg1=rsi, arg2=rdx, arg3=rcx, arg4=r8, arg5=r9
    # In trap.c:
    #   rc = syscall_dispatch(gpr[0], gpr[1], gpr[2], gpr[3], gpr[4], gpr[5], gpr[6]);
    # To map properly, let's use the SYSV ABI mapping to gpr[]:
    # gpr[0] = id   = rax (in syscall)
    # gpr[1] = arg0 = rdi
    # gpr[2] = arg1 = rsi
    # gpr[3] = arg2 = rdx
    # gpr[4] = arg3 = rcx / r10 (user puts arg3 in r10 typically)
    # gpr[5] = arg4 = r8
    # gpr[6] = arg5 = r9

    movq %rax, 0(%rsp)     # gpr[0]
    movq %rdi, 8(%rsp)     # gpr[1]
    movq %rsi, 16(%rsp)    # gpr[2]
    movq %rdx, 24(%rsp)    # gpr[3]
    movq %r10, 32(%rsp)    # gpr[4]
    movq %r8,  40(%rsp)    # gpr[5]
    movq %r9,  48(%rsp)    # gpr[6]

    # Also save others for restoration
    movq %rcx, 56(%rsp)    # gpr[7]
    movq %r11, 64(%rsp)    # gpr[8]
    movq %rbx, 72(%rsp)    # gpr[9]
    movq %rbp, 80(%rsp)    # gpr[10]
    movq %r12, 88(%rsp)    # gpr[11]
    movq %r13, 96(%rsp)    # gpr[12]
    movq %r14, 104(%rsp)   # gpr[13]
    movq %r15, 112(%rsp)   # gpr[14]

    # Hardware pushed:
    # [rsp+280] = Vector (cause)
    # [rsp+288] = Error Code
    # [rsp+296] = RIP (pc)
    # [rsp+304] = CS
    # [rsp+312] = RFLAGS (status)
    # [rsp+320] = RSP (sp)
    # [rsp+328] = SS

    # Cause
    movq 280(%rsp), %rax
    movq %rax, 264(%rsp)   # trap_frame->cause

    # PC
    movq 296(%rsp), %rax
    movq %rax, 256(%rsp)   # trap_frame->pc

    # SP
    movq 320(%rsp), %rax
    movq %rax, 248(%rsp)   # trap_frame->sp

    # Status (RFLAGS)
    movq 312(%rsp), %rax
    movq %rax, 272(%rsp)   # trap_frame->status

    # from_user
    movq 304(%rsp), %rax   # load CS
    andq $3, %rax          # CS & 3
    cmpq $3, %rax          # Is it user mode?
    sete %al               # set al to 1 if equal, else 0
    movb %al, 280(%rsp)    # trap_frame->from_user (actually it's at offset 280? Wait.)

    # Struct offset check:
    # 31 * 8 = 248 (gpr)
    # 248 + 8 = 256 (sp)
    # 256 + 8 = 264 (pc)
    # 264 + 8 = 272 (cause)
    # 272 + 8 = 280 (status)
    # 280 + 1 = 281 (from_user)
    # So from_user is at 280, cause is at 264, status is at 272, pc at 256, sp at 248. Correct.

    movq %rsp, %rdi        # arg0 for trap_handle
    call trap_handle

    # Reload values
    movq 256(%rsp), %rax
    movq %rax, 296(%rsp)   # Restore RIP from trap_frame->pc

    # Restore registers
    movq 0(%rsp), %rax
    movq 8(%rsp), %rdi
    movq 16(%rsp), %rsi
    movq 24(%rsp), %rdx
    movq 32(%rsp), %r10
    movq 40(%rsp), %r8
    movq 48(%rsp), %r9

    movq 56(%rsp), %rcx
    movq 64(%rsp), %r11
    movq 72(%rsp), %rbx
    movq 80(%rsp), %rbp
    movq 88(%rsp), %r12
    movq 96(%rsp), %r13
    movq 104(%rsp), %r14
    movq 112(%rsp), %r15

    addq $296, %rsp        # Skip trap_frame, vector, and error code (280 + 8 + 8)

    iretq
