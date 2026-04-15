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
    # We allocate space for x86_trap_frame_t. Size of generic trap_frame_t is 288 bytes.
    # x86_trap_frame_t adds 16 bytes for error_code and cr2, total 304 bytes allocated on top of vector/error_code.
    # struct is: trap_frame_t base, uint64_t error_code, uint64_t cr2
    # The CPU/stubs pushed vector and error_code. We allocate 312 bytes to keep %rsp 16-byte aligned.
    subq $312, %rsp

    # Save General Purpose Registers (map x86_64 to gpr[0..14])
    movq %rax, 0(%rsp)     # gpr[0]
    movq %rdi, 8(%rsp)     # gpr[1]
    movq %rsi, 16(%rsp)    # gpr[2]
    movq %rdx, 24(%rsp)    # gpr[3]
    movq %r10, 32(%rsp)    # gpr[4]
    movq %r8,  40(%rsp)    # gpr[5]
    movq %r9,  48(%rsp)    # gpr[6]
    movq %rcx, 56(%rsp)    # gpr[7]
    movq %r11, 64(%rsp)    # gpr[8]
    movq %rbx, 72(%rsp)    # gpr[9]
    movq %rbp, 80(%rsp)    # gpr[10]
    movq %r12, 88(%rsp)    # gpr[11]
    movq %r13, 96(%rsp)    # gpr[12]
    movq %r14, 104(%rsp)   # gpr[13]
    movq %r15, 112(%rsp)   # gpr[14]

    # Hardware/Stub pushed state (offsets relative to current rsp due to subq 312):
    # [rsp+312] = Vector (cause)
    # [rsp+320] = Error Code
    # [rsp+328] = RIP (pc)
    # [rsp+336] = CS
    # [rsp+344] = RFLAGS (status)
    # [rsp+352] = RSP (sp)
    # [rsp+360] = SS

    # pc
    movq 328(%rsp), %rax
    movq %rax, 256(%rsp)

    # sp
    movq 352(%rsp), %rax
    movq %rax, 248(%rsp)

    # cause (vector number)
    movq 312(%rsp), %rax
    movq %rax, 264(%rsp)

    # status (RFLAGS)
    movq 344(%rsp), %rax
    movq %rax, 272(%rsp)

    # Determine type: Vector < 32 or Vector == 128 is SYNC (0), else IRQ (1)
    movq 312(%rsp), %rax # Vector
    cmpq $32, %rax
    jb .L_is_sync
    cmpq $128, %rax
    je .L_is_sync
    movl $1, 280(%rsp)   # IRQ (type is uint32_t)
    jmp .L_type_done
.L_is_sync:
    movl $0, 280(%rsp)   # SYNC
.L_type_done:

    # from_user: (CS & 3) == 3
    movq 336(%rsp), %rax # CS
    andq $3, %rax
    cmpq $3, %rax
    sete %al
    movzx %al, %eax
    movl %eax, 284(%rsp) # from_user (uint32_t)

    # Copy Error Code to x86_trap_frame_t extension
    movq 320(%rsp), %rax
    movq %rax, 288(%rsp) # error_code

    # Read CR2 and copy to x86_trap_frame_t extension
    movq %cr2, %rax
    movq %rax, 296(%rsp) # cr2

    movq %rsp, %rdi
    call trap_handle

    # Reload pc in case it changed
    movq 256(%rsp), %rax
    movq %rax, 328(%rsp)

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

    addq $328, %rsp # Skip x86_trap_frame(304) + padding(8) + vector(8) + error_code(8) = 328
    iretq
