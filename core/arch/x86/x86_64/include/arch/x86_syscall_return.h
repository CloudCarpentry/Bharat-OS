#pragma once

/* Preprocessor-safe constants shared by x86_64 assembly and C layout checks. */
#define X86_SYSCALL_RET_PC_OFFSET            0
#define X86_SYSCALL_RET_SP_OFFSET            8
#define X86_SYSCALL_RET_STATUS_OFFSET        16
#define X86_SYSCALL_RET_RESULT_OFFSET        24
#define X86_SYSCALL_RET_ORIGIN_OFFSET        32
#define X86_SYSCALL_RET_FLAGS_OFFSET         36
#define X86_SYSCALL_RET_DISPOSITION_OFFSET   40
#define X86_SYSCALL_RET_CONTEXT_SIZE         48
#define X86_SYSCALL_RETURN_USER_VALUE        0
