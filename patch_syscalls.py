import re

with open("core/personalities/compat/linux/linux_syscall.c", "r") as f:
    content = f.read()

openat_close_funcs = """
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
    path_buf[sizeof(path_buf) - 1] = '\\0';

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

"""

# Insert functions before bh_sys_read extern declaration
content = content.replace("extern bh_operation_result_t bh_sys_read(bh_syscall_ctx_t *ctx);", openat_close_funcs + "extern bh_operation_result_t bh_sys_read(bh_syscall_ctx_t *ctx);")

# Add IOCTL define at the top
ioctl_nr = """
#ifndef LINUX_X86_64_SYS_IOCTL
#define LINUX_X86_64_SYS_IOCTL 16
#endif
"""

content = content.replace('#include "linux_syscall_numbers_x86_64.h"', '#include "linux_syscall_numbers_x86_64.h"\n' + ioctl_nr)


# Add to table
table_insert = """    [LINUX_X86_64_SYS_OPEN]       = { .nr = LINUX_X86_64_SYS_OPEN, .name = "open", .class_id = BH_SYS_CLASS_IO, .arg_count = 3, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ, .handler = linux_sys_openat },
    [LINUX_X86_64_SYS_OPENAT]     = { .nr = LINUX_X86_64_SYS_OPENAT, .name = "openat", .class_id = BH_SYS_CLASS_IO, .arg_count = 4, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ, .handler = linux_sys_openat },
    [LINUX_X86_64_SYS_CLOSE]      = { .nr = LINUX_X86_64_SYS_CLOSE, .name = "close", .class_id = BH_SYS_CLASS_IO, .arg_count = 1, .flags = BH_SYSCALL_F_BLOCKING, .handler = linux_sys_close },
    [LINUX_X86_64_SYS_IOCTL]      = { .nr = LINUX_X86_64_SYS_IOCTL, .name = "ioctl", .class_id = BH_SYS_CLASS_IO, .arg_count = 3, .flags = BH_SYSCALL_F_BLOCKING, .handler = linux_sys_ioctl },
"""
content = content.replace("    [LINUX_X86_64_SYS_READ]       = { .nr = LINUX_X86_64_SYS_READ", table_insert + "    [LINUX_X86_64_SYS_READ]       = { .nr = LINUX_X86_64_SYS_READ")

with open("core/personalities/compat/linux/linux_syscall.c", "w") as f:
    f.write(content)
