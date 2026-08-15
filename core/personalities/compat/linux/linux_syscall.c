#include "linux_personality.h"
#include "linux_errno.h"
#include "linux_syscall_numbers_x86_64.h"

#ifndef LINUX_X86_64_SYS_IOCTL
#define LINUX_X86_64_SYS_IOCTL 16
#endif

#include "trap/syscall_context.h"
#include "kernel/status.h"
#include "sched/sched.h"
#include "syscall/usercopy.h"
#include "trap/syscall_status.h"
#include "time/ktime.h"

#define LINUX_CLOCK_MONOTONIC 1U
#define LINUX_NSEC_PER_SEC UINT64_C(1000000000)

typedef struct {
    int64_t tv_sec;
    int64_t tv_nsec;
} linux_timespec_t;

static bh_operation_result_t linux_sys_getpid(bh_syscall_ctx_t *ctx) {
    if (!ctx || !ctx->process) return bh_op_result_kstatus(K_ERR_INVALID_ARG);
    return bh_op_result_value((long)ctx->process->process_id);
}

static bh_operation_result_t linux_sys_gettid(bh_syscall_ctx_t *ctx) {
    if (!ctx || !ctx->thread) return bh_op_result_kstatus(K_ERR_INVALID_ARG);
    return bh_op_result_value((long)ctx->thread->thread_id);
}

static bh_operation_result_t linux_sys_futex(bh_syscall_ctx_t *ctx) {
    (void)ctx;
    return bh_op_result_kstatus(K_ERR_UNSUPPORTED);
}

static bh_operation_result_t linux_sys_clock_gettime(bh_syscall_ctx_t *ctx) {
    uint64_t now_ns;
    linux_timespec_t value;
    bh_status_t status;

    if ((uint32_t)ctx->regs.arg[0] != LINUX_CLOCK_MONOTONIC) {
        return bh_op_result_kstatus(K_ERR_UNSUPPORTED);
    }
    now_ns = bh_ktime_now();
    value.tv_sec = (int64_t)(now_ns / LINUX_NSEC_PER_SEC);
    value.tv_nsec = (int64_t)(now_ns % LINUX_NSEC_PER_SEC);
    status = bh_copy_to_user((void *)ctx->regs.arg[1], &value, sizeof(value));
    return bh_op_result_kstatus(bh_status_to_kstatus(status));
}


#include "mm/aspace.h"
#include "mm/vm_mapping.h"

#define LINUX_MAP_SHARED    0x01
#define LINUX_MAP_PRIVATE   0x02
#define LINUX_MAP_FIXED     0x10
#define LINUX_MAP_ANONYMOUS 0x20

#define LINUX_PROT_READ     0x1
#define LINUX_PROT_WRITE    0x2
#define LINUX_PROT_EXEC     0x4

static uint32_t linux_prot_to_bh_prot(uint32_t l_prot) {
    uint32_t prot = 0;
    if (l_prot & LINUX_PROT_READ) prot |= VM_PROT_READ;
    if (l_prot & LINUX_PROT_WRITE) prot |= VM_PROT_WRITE;
    if (l_prot & LINUX_PROT_EXEC) prot |= VM_PROT_EXEC;
    prot |= VM_PROT_USER; // All Linux mappings are user mappings
    return prot;
}

static bh_status_t linux_translate_mmap_request(bh_syscall_ctx_t *ctx, vm_map_request_t *req) {
    uintptr_t addr = ctx->regs.arg[0];
    size_t length = ctx->regs.arg[1];
    uint32_t prot = ctx->regs.arg[2];
    uint32_t flags = ctx->regs.arg[3];

    req->hint = addr;
    req->length = length;
    req->prot = linux_prot_to_bh_prot(prot);
    req->flags = 0;

    if (flags & LINUX_MAP_FIXED) {
        req->flags |= VM_MAP_FIXED;
    }

    if (flags & LINUX_MAP_ANONYMOUS) {
        req->type = VM_MAP_TYPE_ANON;
        req->object = NULL;
        req->object_offset = 0;
    } else {
        // file-backed mappings not yet supported in this stub
        return BH_ERR_NOT_SUPPORTED;
    }

    return BH_OK;
}

static bh_operation_result_t linux_sys_mmap(bh_syscall_ctx_t *ctx) {
    vm_map_request_t req = {0};

    bh_status_t st = linux_translate_mmap_request(ctx, &req);
    if (st != BH_OK) return bh_op_result_value(-linux_errno_from_bh_status((kstatus_t)st));

    uintptr_t result;
    if (!ctx->process || !ctx->process->addr_space) {
        return bh_op_result_value(-LINUX_EINVAL);
    }

    kstatus_t kst = vm_map_region(ctx->process->addr_space, &req, &result);

    if (kst != K_OK) return bh_op_result_value(-linux_errno_from_bh_status((kstatus_t)kst));

    return bh_op_result_value((long)result);
}

static bh_operation_result_t linux_sys_munmap(bh_syscall_ctx_t *ctx) {
    uintptr_t addr = ctx->regs.arg[0];
    size_t length = ctx->regs.arg[1];

    if (!ctx->process || !ctx->process->addr_space) {
        return bh_op_result_value(-LINUX_EINVAL);
    }

    kstatus_t kst = vm_unmap_region(ctx->process->addr_space, addr, length);
    if (kst != K_OK) return bh_op_result_value(-linux_errno_from_bh_status((kstatus_t)kst));

    return bh_op_result_value(0);
}

static bh_operation_result_t linux_sys_mprotect(bh_syscall_ctx_t *ctx) {
    uintptr_t addr = ctx->regs.arg[0];
    size_t length = ctx->regs.arg[1];
    uint32_t prot = ctx->regs.arg[2];

    if (!ctx->process || !ctx->process->addr_space) {
        return bh_op_result_value(-LINUX_EINVAL);
    }

    kstatus_t kst = vm_protect_region(ctx->process->addr_space, addr, length, linux_prot_to_bh_prot(prot));
    if (kst != K_OK) return bh_op_result_value(-linux_errno_from_bh_status((kstatus_t)kst));

    return bh_op_result_value(0);
}

static bh_operation_result_t linux_sys_brk(bh_syscall_ctx_t *ctx) {
    // TODO: Full implementation requires tracking brk boundary in the process struct.
    // Stub returning -ENOMEM to fail gracefully for now.
    (void)ctx;
    return bh_op_result_value(-LINUX_ENOMEM);
}


static bh_operation_result_t linux_sys_openat(bh_syscall_ctx_t *ctx) {
    // int dirfd = (int)ctx->regs.arg[0];
    const char *pathname = (const char *)ctx->regs.arg[1];
    // int flags = (int)ctx->regs.arg[2];
    // int mode = (int)ctx->regs.arg[3];

    if (!pathname) {
        return bh_op_result_value(-LINUX_EFAULT);
    }

    char path_buf[256];
    // Copy path from user space (safely)
    bh_status_t st = bh_copy_from_user(path_buf, pathname, sizeof(path_buf) - 1);
    if (st != BH_OK) {
        return bh_op_result_value(-linux_errno_from_bh_status((kstatus_t)st));
    }
    path_buf[sizeof(path_buf) - 1] = '\0';

    // In a full implementation, we'd map this to fsd_openat_file and map the
    // resulting capability to a Linux FD in linux_compat's fd_table.
    // For the Phase 1 stub, we return -ENOENT for anything other than specific stubs
    // like /dev/tty, or just defer to a not-supported stub if VFS is not fully integrated.

    return bh_op_result_value(-LINUX_ENOENT);
}

static bh_operation_result_t linux_sys_close(bh_syscall_ctx_t *ctx) {
    int fd = (int)ctx->regs.arg[0];

    // Stub: we don't have direct access to fd_table here unless we expose it
    // or call a function in linux_compat.c. For now, if fd < 0, error.
    if (fd < 0 || fd >= 256) {
        return bh_op_result_value(-LINUX_EBADF);
    }

    return bh_op_result_value(0); // Success stub
}

static bh_operation_result_t linux_sys_ioctl(bh_syscall_ctx_t *ctx) {
    int fd = (int)ctx->regs.arg[0];

    if (fd < 0 || fd >= 256) {
        return bh_op_result_value(-LINUX_EBADF);
    }

    return bh_op_result_value(-LINUX_ENOTTY); // Stub response
}

extern bh_operation_result_t bh_sys_read(bh_syscall_ctx_t *ctx);
extern bh_operation_result_t bh_sys_write(bh_syscall_ctx_t *ctx);
extern bh_operation_result_t bh_sys_thread_exit(bh_syscall_ctx_t *ctx);

static const bh_syscall_meta_t linux_syscall_table_x86_64[] = {
    [LINUX_X86_64_SYS_OPEN]       = { .nr = LINUX_X86_64_SYS_OPEN, .name = "open", .class_id = BH_SYS_CLASS_IO, .arg_count = 3, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ, .handler = linux_sys_openat },
    [LINUX_X86_64_SYS_OPENAT]     = { .nr = LINUX_X86_64_SYS_OPENAT, .name = "openat", .class_id = BH_SYS_CLASS_IO, .arg_count = 4, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ, .handler = linux_sys_openat },
    [LINUX_X86_64_SYS_CLOSE]      = { .nr = LINUX_X86_64_SYS_CLOSE, .name = "close", .class_id = BH_SYS_CLASS_IO, .arg_count = 1, .flags = BH_SYSCALL_F_BLOCKING, .handler = linux_sys_close },
    [LINUX_X86_64_SYS_IOCTL]      = { .nr = LINUX_X86_64_SYS_IOCTL, .name = "ioctl", .class_id = BH_SYS_CLASS_IO, .arg_count = 3, .flags = BH_SYSCALL_F_BLOCKING, .handler = linux_sys_ioctl },
    [LINUX_X86_64_SYS_READ]       = { .nr = LINUX_X86_64_SYS_READ, .name = "read", .class_id = BH_SYS_CLASS_IO, .arg_count = 3, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_WRITE, .handler = bh_sys_read },
    [LINUX_X86_64_SYS_WRITE]      = { .nr = LINUX_X86_64_SYS_WRITE, .name = "write", .class_id = BH_SYS_CLASS_IO, .arg_count = 3, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ, .handler = bh_sys_write },

    [LINUX_X86_64_SYS_MMAP]       = { .nr = LINUX_X86_64_SYS_MMAP, .name = "mmap", .class_id = BH_SYS_CLASS_MEMORY, .arg_count = 6, .handler = linux_sys_mmap },
    [LINUX_X86_64_SYS_MUNMAP]     = { .nr = LINUX_X86_64_SYS_MUNMAP, .name = "munmap", .class_id = BH_SYS_CLASS_MEMORY, .arg_count = 2, .handler = linux_sys_munmap },
    [LINUX_X86_64_SYS_MPROTECT]   = { .nr = LINUX_X86_64_SYS_MPROTECT, .name = "mprotect", .class_id = BH_SYS_CLASS_MEMORY, .arg_count = 3, .handler = linux_sys_mprotect },
    [LINUX_X86_64_SYS_BRK]        = { .nr = LINUX_X86_64_SYS_BRK, .name = "brk", .class_id = BH_SYS_CLASS_MEMORY, .arg_count = 1, .handler = linux_sys_brk },
    [LINUX_X86_64_SYS_GETPID]     = { .nr = LINUX_X86_64_SYS_GETPID, .name = "getpid", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 0, .flags = BH_SYSCALL_F_FAST, .handler = linux_sys_getpid },
    [LINUX_X86_64_SYS_EXIT]       = { .nr = LINUX_X86_64_SYS_EXIT, .name = "exit", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 1, .handler = bh_sys_thread_exit },
    [LINUX_X86_64_SYS_EXIT_GROUP] = { .nr = LINUX_X86_64_SYS_EXIT_GROUP, .name = "exit_group", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 1, .handler = bh_sys_thread_exit },
    [LINUX_X86_64_SYS_CLOCK_GETTIME] = { .nr = LINUX_X86_64_SYS_CLOCK_GETTIME, .name = "clock_gettime", .class_id = BH_SYS_CLASS_SYSTEM, .arg_count = 2, .flags = BH_SYSCALL_F_USER_WRITE, .handler = linux_sys_clock_gettime },
};

const bh_personality_syscall_table_t bh_linux_syscall_table = {
    .name = "linux",
    .abi_version = 1,
    .entry_count = 232,
    .table = linux_syscall_table_x86_64
};
