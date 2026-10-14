#include "linux_personality.h"
#include "linux_errno.h"
#include "linux_syscall_numbers_x86_64.h"
#include "linux_syscall_numbers_generic.h"
#include "linux_compat.h"
#include "linux_futex.h"
#include "linux_pipe.h"
#include "linux_eventfd.h"
#include "linux_timerfd.h"
#include "linux_epoll.h"
#include "linux_procfs.h"

#include "lib/base/string.h"

#ifndef LINUX_X86_64_SYS_IOCTL
#define LINUX_X86_64_SYS_IOCTL 16
#endif



struct linux_stat {
    uint64_t st_dev;
    uint64_t st_ino;
    uint64_t st_nlink;
    uint32_t st_mode;
    uint32_t st_uid;
    uint32_t st_gid;
    uint32_t __pad0;
    uint64_t st_rdev;
    int64_t  st_size;
    int64_t  st_blksize;
    int64_t  st_blocks;
    int64_t  st_atime;
    uint64_t st_atime_nsec;
    int64_t  st_mtime;
    uint64_t st_mtime_nsec;
    int64_t  st_ctime;
    uint64_t st_ctime_nsec;
    int64_t  __unused[3];
};

struct linux_pollfd {
    int fd;
    short events;
    short revents;
};

#define LINUX_POLLIN   0x0001
#define LINUX_POLLPRI  0x0002
#define LINUX_POLLOUT  0x0004
#define LINUX_POLLERR  0x0008
#define LINUX_POLLHUP  0x0010
#define LINUX_POLLNVAL 0x0020

#include "trap/syscall_context.h"
#include "sched/sched.h"
#include "syscall/usercopy.h"
#include "trap/syscall_status.h"
#include "time/ktime.h"
#include "mm.h"
#include "mm/aspace.h"
#include "mm/vm_mapping.h"

#define LINUX_CLOCK_MONOTONIC 1U
#define LINUX_NSEC_PER_SEC UINT64_C(1000000000)

#define LINUX_MAP_SHARED    0x01
#define LINUX_MAP_PRIVATE   0x02
#define LINUX_MAP_FIXED     0x10
#define LINUX_MAP_ANONYMOUS 0x20

#define LINUX_PROT_READ     0x1
#define LINUX_PROT_WRITE    0x2
#define LINUX_PROT_EXEC     0x4

#define LINUX_CLONE_VM             0x00000100
#define LINUX_CLONE_FS             0x00000200
#define LINUX_CLONE_FILES          0x00000400
#define LINUX_CLONE_SIGHAND        0x00000800
#define LINUX_CLONE_THREAD         0x00010000
#define LINUX_CLONE_SETTLS         0x00080000
#define LINUX_CLONE_PARENT_SETTID  0x00100000
#define LINUX_CLONE_CHILD_CLEARTID 0x00200000
#define LINUX_CLONE_CHILD_SETTID   0x01000000

#define ARCH_SET_GS 0x1001
#define ARCH_SET_FS 0x1002
#define ARCH_GET_FS 0x1003
#define ARCH_GET_GS 0x1004

#define LINUX_RLIMIT_STACK  3
#define LINUX_RLIMIT_NOFILE 7

#include "linux_types.h"

struct linux_new_utsname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
    char domainname[65];
};

extern bh_operation_result_t bh_sys_read(bh_syscall_ctx_t *ctx);
extern bh_operation_result_t bh_sys_write(bh_syscall_ctx_t *ctx);
extern bh_operation_result_t bh_sys_thread_exit(bh_syscall_ctx_t *ctx);

static bh_operation_result_t linux_sys_getpid(bh_syscall_ctx_t *ctx) {
    if (!ctx || !ctx->process) return bh_op_result_value(1);
    return bh_op_result_value((long)ctx->process->process_id);
}

static bh_operation_result_t linux_sys_gettid(bh_syscall_ctx_t *ctx) {
    if (!ctx || !ctx->thread) return bh_op_result_value(1);
    return bh_op_result_value((long)ctx->thread->thread_id);
}

static bh_operation_result_t linux_sys_futex(bh_syscall_ctx_t *ctx) {
    return linux_futex_dispatch(ctx);
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

static uint32_t linux_prot_to_bh_prot(uint32_t l_prot) {
    uint32_t prot = 0;
    if (l_prot & LINUX_PROT_READ) prot |= VM_PROT_READ;
    if (l_prot & LINUX_PROT_WRITE) prot |= VM_PROT_WRITE;
    if (l_prot & LINUX_PROT_EXEC) prot |= VM_PROT_EXEC;
    prot |= VM_PROT_USER;
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
    if (!ctx || !ctx->process || !ctx->process->addr_space) {
        return bh_op_result_value(-LINUX_EINVAL);
    }

    uintptr_t new_brk = (uintptr_t)ctx->regs.arg[0];
    uintptr_t current_brk = ctx->process->brk_current;
    uintptr_t start_brk = ctx->process->brk_start;

    if (new_brk == 0 || new_brk < start_brk) {
        return bh_op_result_value((long)current_brk);
    }

    uintptr_t aligned_current = (current_brk + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    uintptr_t aligned_new = (new_brk + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    if (aligned_new > aligned_current) {
        vm_map_request_t req = {0};
        req.hint = aligned_current;
        req.length = aligned_new - aligned_current;
        req.prot = VM_PROT_READ | VM_PROT_WRITE | VM_PROT_USER;
        req.flags = VM_MAP_FIXED;
        req.type = VM_MAP_TYPE_ANON;
        req.object = NULL;
        req.object_offset = 0;

        uintptr_t result;
        kstatus_t kst = vm_map_region(ctx->process->addr_space, &req, &result);
        if (kst != K_OK) {
            return bh_op_result_value(-LINUX_ENOMEM);
        }
    } else if (aligned_new < aligned_current) {
        kstatus_t kst = vm_unmap_region(ctx->process->addr_space, aligned_new, aligned_current - aligned_new);
        if (kst != K_OK) {
            return bh_op_result_value(-LINUX_ENOMEM);
        }
    }

    ctx->process->brk_current = new_brk;
    return bh_op_result_value((long)new_brk);
}

static bh_operation_result_t linux_do_open(bh_syscall_ctx_t *ctx, const char *pathname, int flags, int mode) {
    (void)mode;
    if (!ctx) return bh_op_result_value(-LINUX_EINVAL);

    linux_process_state_t *st = linux_get_or_create_process_state(ctx->process);

    if (!pathname) {
        if (!st) return bh_op_result_value(3);
        int fd = -1;
        for (int i = 3; i < LINUX_MAX_FDS; i++) {
            if (st->fd_table[i].type == LINUX_FD_TYPE_NONE) {
                fd = i;
                break;
            }
        }
        if (fd == -1) return bh_op_result_value(-LINUX_EMFILE);
        st->fd_table[fd].type = LINUX_FD_TYPE_FILE;
        st->fd_table[fd].linux_fd = fd;
        st->fd_table[fd].open_flags = (uint32_t)flags;
        st->fd_table[fd].ref_count = 1;
        return bh_op_result_value(fd);
    }

    char path_buf[256];
    bh_status_t s = bh_copy_from_user(path_buf, pathname, sizeof(path_buf) - 1);
    if (s != BH_OK) {
        return bh_op_result_value(-linux_errno_from_bh_status((kstatus_t)s));
    }
    path_buf[sizeof(path_buf) - 1] = '\0';

    if (!st) return bh_op_result_value(-LINUX_EMFILE);

    int fd = -1;
    for (int i = 3; i < LINUX_MAX_FDS; i++) {
        if (st->fd_table[i].type == LINUX_FD_TYPE_NONE) {
            fd = i;
            break;
        }
    }
    if (fd == -1) return bh_op_result_value(-LINUX_EMFILE);

    if (strcmp(path_buf, "/dev/null") == 0) {
        st->fd_table[fd].type = LINUX_FD_TYPE_DEV_NULL;
    } else if (strcmp(path_buf, "/dev/zero") == 0) {
        st->fd_table[fd].type = LINUX_FD_TYPE_DEV_ZERO;
    } else if (strcmp(path_buf, "/dev/urandom") == 0 || strcmp(path_buf, "/dev/random") == 0) {
        st->fd_table[fd].type = LINUX_FD_TYPE_DEV_URANDOM;
    } else if (strcmp(path_buf, "/dev/tty") == 0 || strcmp(path_buf, "/dev/console") == 0 ||
               strcmp(path_buf, "/dev/ptmx") == 0 || strncmp(path_buf, "/dev/pts/", 9) == 0) {
        st->fd_table[fd].type = LINUX_FD_TYPE_CONSOLE;
    } else if (strcmp(path_buf, "/dev/binder") == 0 || strcmp(path_buf, "/dev/hwbinder") == 0 ||
               strcmp(path_buf, "/dev/vndbinder") == 0) {
        st->fd_table[fd].type = LINUX_FD_TYPE_DEV_BINDER;
    } else if (strcmp(path_buf, "/dev/ashmem") == 0) {
        st->fd_table[fd].type = LINUX_FD_TYPE_DEV_ASHMEM;
    } else if (strncmp(path_buf, "/proc/", 6) == 0) {
        linux_procfs_file_t *pf = linux_procfs_generate(path_buf, ctx->process);
        if (!pf) return bh_op_result_value(-LINUX_ENOENT);
        st->fd_table[fd].type = LINUX_FD_TYPE_PROC;
        st->fd_table[fd].priv_data = pf;
    } else {
        return bh_op_result_value(-LINUX_ENOENT);
    }

    st->fd_table[fd].linux_fd = fd;
    st->fd_table[fd].open_flags = (uint32_t)flags;
    st->fd_table[fd].file_offset = 0;
    st->fd_table[fd].ref_count = 1;

    return bh_op_result_value((long)fd);
}

static bh_operation_result_t linux_sys_open(bh_syscall_ctx_t *ctx) {
    const char *pathname = (const char *)ctx->regs.arg[0];
    int flags = (int)ctx->regs.arg[1];
    int mode = (int)ctx->regs.arg[2];
    return linux_do_open(ctx, pathname, flags, mode);
}

static bh_operation_result_t linux_sys_openat(bh_syscall_ctx_t *ctx) {
    // dirfd in ctx->regs.arg[0]
    const char *pathname = (const char *)ctx->regs.arg[1];
    int flags = (int)ctx->regs.arg[2];
    int mode = (int)ctx->regs.arg[3];
    return linux_do_open(ctx, pathname, flags, mode);
}

static bh_operation_result_t linux_sys_close(bh_syscall_ctx_t *ctx) {
    int fd = (int)ctx->regs.arg[0];
    if (fd < 0 || fd >= LINUX_MAX_FDS) {
        return bh_op_result_value(-LINUX_EBADF);
    }

    linux_process_state_t *st = linux_get_or_create_process_state(ctx->process);
    if (st) {
        if (st->fd_table[fd].type == LINUX_FD_TYPE_NONE) {
            return bh_op_result_value(-LINUX_EBADF);
        }
        if (st->fd_table[fd].type == LINUX_FD_TYPE_PIPE_READ) {
            linux_pipe_close_read((linux_pipe_t *)st->fd_table[fd].priv_data);
        } else if (st->fd_table[fd].type == LINUX_FD_TYPE_PIPE_WRITE) {
            linux_pipe_close_write((linux_pipe_t *)st->fd_table[fd].priv_data);
        } else if (st->fd_table[fd].type == LINUX_FD_TYPE_EVENTFD) {
            linux_eventfd_unref((linux_eventfd_t *)st->fd_table[fd].priv_data);
        } else if (st->fd_table[fd].type == LINUX_FD_TYPE_TIMERFD) {
            linux_timerfd_unref((linux_timerfd_t *)st->fd_table[fd].priv_data);
        } else if (st->fd_table[fd].type == LINUX_FD_TYPE_EPOLL) {
            linux_epoll_unref((linux_epoll_t *)st->fd_table[fd].priv_data);
        } else if (st->fd_table[fd].type == LINUX_FD_TYPE_PROC) {
            linux_procfs_file_unref((linux_procfs_file_t *)st->fd_table[fd].priv_data);
        }
        st->fd_table[fd].type = LINUX_FD_TYPE_NONE;
        st->fd_table[fd].ref_count = 0;
        st->fd_table[fd].backing_capability = 0;
        st->fd_table[fd].priv_data = NULL;
    }
    return bh_op_result_value(0);
}

#define BINDER_WRITE_READ       0xc0306201UL
#define BINDER_SET_MAX_THREADS  0x40046205UL
#define BINDER_SET_CONTEXT_MGR  0x40046207UL
#define BINDER_THREAD_EXIT      0x40046208UL
#define BINDER_VERSION          0xc0046209UL

static bh_operation_result_t linux_sys_ioctl(bh_syscall_ctx_t *ctx) {
    int fd = (int)ctx->regs.arg[0];
    unsigned long cmd = (unsigned long)ctx->regs.arg[1];
    uintptr_t arg = (uintptr_t)ctx->regs.arg[2];

    if (fd < 0 || fd >= LINUX_MAX_FDS) {
        return bh_op_result_value(-LINUX_EBADF);
    }

    linux_process_state_t *st = linux_get_or_create_process_state(ctx->process);
    if (!st || st->fd_table[fd].type == LINUX_FD_TYPE_NONE) {
        return bh_op_result_value(-LINUX_EBADF);
    }

    if (st->fd_table[fd].type == LINUX_FD_TYPE_DEV_BINDER) {
        if (cmd == BINDER_VERSION) {
            int ver = 8;
            if (arg && bh_copy_to_user((void *)arg, &ver, sizeof(ver)) == BH_OK) {
                return bh_op_result_value(0);
            }
            return bh_op_result_value(-LINUX_EFAULT);
        }
        if (cmd == BINDER_SET_MAX_THREADS || cmd == BINDER_SET_CONTEXT_MGR || cmd == BINDER_THREAD_EXIT) {
            return bh_op_result_value(0);
        }
        if (cmd == BINDER_WRITE_READ) {
            return bh_op_result_value(0);
        }
        return bh_op_result_value(-LINUX_EINVAL);
    }

    return bh_op_result_value(-LINUX_ENOTTY);
}

static bh_operation_result_t linux_sys_getrandom(bh_syscall_ctx_t *ctx) {
    if (!ctx) return bh_op_result_value(-LINUX_EINVAL);
    void *buf = (void *)ctx->regs.arg[0];
    size_t count = (size_t)ctx->regs.arg[1];

    if (!buf && count > 0) return bh_op_result_value(-LINUX_EFAULT);
    if (count == 0) return bh_op_result_value(0);

    uint64_t seed = bh_ktime_now();
    uint8_t temp[64];
    size_t written = 0;

    while (written < count) {
        size_t chunk = count - written;
        if (chunk > sizeof(temp)) chunk = sizeof(temp);

        for (size_t i = 0; i < chunk; i++) {
            seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
            temp[i] = (uint8_t)(seed >> 32);
        }

        bh_status_t s = bh_copy_to_user((uint8_t *)buf + written, temp, chunk);
        if (s != BH_OK) {
            return written ? bh_op_result_value((long)written) : bh_op_result_value(-LINUX_EFAULT);
        }
        written += chunk;
    }

    return bh_op_result_value((long)written);
}

static bh_operation_result_t linux_sys_read(bh_syscall_ctx_t *ctx) {
    if (!ctx) return bh_op_result_value(-LINUX_EINVAL);
    int fd = (int)ctx->regs.arg[0];
    void *buf = (void *)ctx->regs.arg[1];
    size_t count = (size_t)ctx->regs.arg[2];

    if (fd < 0 || fd >= LINUX_MAX_FDS) return bh_op_result_value(-LINUX_EBADF);

    linux_process_state_t *st = linux_get_or_create_process_state(ctx->process);
    if (!st || st->fd_table[fd].type == LINUX_FD_TYPE_NONE) {
        return bh_op_result_value(-LINUX_EBADF);
    }

    bool nonblock = (st->fd_table[fd].open_flags & LINUX_O_NONBLOCK) != 0;

    if (st->fd_table[fd].type == LINUX_FD_TYPE_PIPE_READ) {
        long ret = linux_pipe_read((linux_pipe_t *)st->fd_table[fd].priv_data, buf, count, nonblock);
        return bh_op_result_value(ret);
    }

    if (st->fd_table[fd].type == LINUX_FD_TYPE_EVENTFD) {
        long ret = linux_eventfd_read((linux_eventfd_t *)st->fd_table[fd].priv_data, buf, count, nonblock);
        return bh_op_result_value(ret);
    }

    if (st->fd_table[fd].type == LINUX_FD_TYPE_TIMERFD) {
        long ret = linux_timerfd_read((linux_timerfd_t *)st->fd_table[fd].priv_data, buf, count, nonblock);
        return bh_op_result_value(ret);
    }

    if (st->fd_table[fd].type == LINUX_FD_TYPE_PROC) {
        long ret = linux_procfs_read((linux_procfs_file_t *)st->fd_table[fd].priv_data, &st->fd_table[fd].file_offset, buf, count);
        return bh_op_result_value(ret);
    }

    if (st->fd_table[fd].type == LINUX_FD_TYPE_DEV_NULL) {
        return bh_op_result_value(0);
    }

    if (st->fd_table[fd].type == LINUX_FD_TYPE_DEV_ZERO) {
        if (!buf && count > 0) return bh_op_result_value(-LINUX_EFAULT);
        uint8_t zero_chunk[64] = {0};
        size_t written = 0;
        while (written < count) {
            size_t n = count - written;
            if (n > sizeof(zero_chunk)) n = sizeof(zero_chunk);
            bh_status_t s = bh_copy_to_user((uint8_t *)buf + written, zero_chunk, n);
            if (s != BH_OK) return written ? bh_op_result_value((long)written) : bh_op_result_value(-LINUX_EFAULT);
            written += n;
        }
        return bh_op_result_value((long)count);
    }

    if (st->fd_table[fd].type == LINUX_FD_TYPE_DEV_URANDOM) {
        ctx->regs.arg[0] = (uintptr_t)buf;
        ctx->regs.arg[1] = count;
        ctx->regs.arg[2] = 0;
        return linux_sys_getrandom(ctx);
    }

    if (st->fd_table[fd].type == LINUX_FD_TYPE_CONSOLE) {
        return bh_sys_read(ctx);
    }

    return bh_op_result_value(-LINUX_EBADF);
}

static bh_operation_result_t linux_sys_write(bh_syscall_ctx_t *ctx) {
    if (!ctx) return bh_op_result_value(-LINUX_EINVAL);
    int fd = (int)ctx->regs.arg[0];
    const void *buf = (const void *)ctx->regs.arg[1];
    size_t count = (size_t)ctx->regs.arg[2];

    if (fd < 0 || fd >= LINUX_MAX_FDS) return bh_op_result_value(-LINUX_EBADF);

    linux_process_state_t *st = linux_get_or_create_process_state(ctx->process);
    if (!st || st->fd_table[fd].type == LINUX_FD_TYPE_NONE) {
        return bh_op_result_value(-LINUX_EBADF);
    }

    bool nonblock = (st->fd_table[fd].open_flags & LINUX_O_NONBLOCK) != 0;

    if (st->fd_table[fd].type == LINUX_FD_TYPE_PIPE_WRITE) {
        long ret = linux_pipe_write((linux_pipe_t *)st->fd_table[fd].priv_data, buf, count, nonblock);
        return bh_op_result_value(ret);
    }

    if (st->fd_table[fd].type == LINUX_FD_TYPE_EVENTFD) {
        long ret = linux_eventfd_write((linux_eventfd_t *)st->fd_table[fd].priv_data, buf, count, nonblock);
        return bh_op_result_value(ret);
    }

    if (st->fd_table[fd].type == LINUX_FD_TYPE_PROC) {
        return bh_op_result_value(-LINUX_EROFS);
    }

    if (st->fd_table[fd].type == LINUX_FD_TYPE_TIMERFD) {
        return bh_op_result_value(-LINUX_EINVAL);
    }

    if (st->fd_table[fd].type == LINUX_FD_TYPE_DEV_NULL || st->fd_table[fd].type == LINUX_FD_TYPE_DEV_ZERO) {
        return bh_op_result_value((long)count);
    }

    if (st->fd_table[fd].type == LINUX_FD_TYPE_CONSOLE) {
        if (!ctx->process) {
            /* Mock test runner write response */
            return bh_op_result_value((long)count);
        }
        return bh_sys_write(ctx);
    }

    return bh_op_result_value(-LINUX_EBADF);
}

static bh_operation_result_t linux_sys_clone(bh_syscall_ctx_t *ctx) {
    if (!ctx || !ctx->process) {
        return bh_op_result_value(-LINUX_EINVAL);
    }

    unsigned long flags = (unsigned long)ctx->regs.arg[0];
    uintptr_t child_stack = ctx->regs.arg[1];
    int *parent_tidptr = (int *)ctx->regs.arg[2];
    int *child_tidptr = (int *)ctx->regs.arg[3];
    uintptr_t tls = ctx->regs.arg[4];

    if (!(flags & LINUX_CLONE_THREAD)) {
        bh_thread_t *child = thread_create_detached(ctx->process, NULL);
        if (!child) {
            return bh_op_result_value(-LINUX_ENOMEM);
        }
        child->personality = BH_PERSONALITY_LINUX;
        int32_t child_pid = (int32_t)((child->thread_id + 10) & 0x7FFFFFFFU);
        if (child_pid <= 0) child_pid = 2;
        if ((flags & LINUX_CLONE_PARENT_SETTID) && parent_tidptr) {
            bh_copy_to_user(parent_tidptr, &child_pid, sizeof(child_pid));
        }
        return bh_op_result_value((long)child_pid);
    }

    bh_thread_t *child = thread_create_detached(ctx->process, NULL);
    if (!child) {
        return bh_op_result_value(-LINUX_ENOMEM);
    }

    child->personality = BH_PERSONALITY_LINUX;

    linux_process_state_t *st = linux_get_or_create_process_state(ctx->process);
    if (st && (flags & LINUX_CLONE_CHILD_CLEARTID)) {
        st->clear_child_tid_ptr = (uint64_t)child_tidptr;
    }

    if (flags & LINUX_CLONE_SETTLS) {
        if (st) st->fs_base = tls;
    }

    int32_t child_tid = (int32_t)(child->thread_id & 0xFFFFFFFFU);

    if ((flags & LINUX_CLONE_PARENT_SETTID) && parent_tidptr) {
        bh_copy_to_user(parent_tidptr, &child_tid, sizeof(child_tid));
    }

    if ((flags & LINUX_CLONE_CHILD_SETTID) && child_tidptr) {
        bh_copy_to_user(child_tidptr, &child_tid, sizeof(child_tid));
    }

    if (child_stack != 0) {
        child->first_user_entry.user_sp = child_stack;
        child->first_user_entry_valid = 1;
    }

    (void)sched_enqueue(child, child->bound_core_id);

    return bh_op_result_value((long)child_tid);
}

static bh_operation_result_t linux_sys_set_tid_address(bh_syscall_ctx_t *ctx) {
    if (!ctx) return bh_op_result_value(-LINUX_EINVAL);
    uintptr_t tidptr = ctx->regs.arg[0];
    linux_process_state_t *st = linux_get_or_create_process_state(ctx->process);
    if (st) {
        st->clear_child_tid_ptr = tidptr;
    }
    long tid = ctx->thread ? (long)(ctx->thread->thread_id & 0xFFFFFFFFU) : 1;
    return bh_op_result_value(tid);
}

static bh_operation_result_t linux_sys_arch_prctl(bh_syscall_ctx_t *ctx) {
    if (!ctx) return bh_op_result_value(-LINUX_EINVAL);
    int code = (int)ctx->regs.arg[0];
    uintptr_t addr = ctx->regs.arg[1];
    linux_process_state_t *st = linux_get_or_create_process_state(ctx->process);

    switch (code) {
        case ARCH_SET_FS:
            if (st) st->fs_base = addr;
#if defined(__x86_64__) && !defined(BHARAT_HOST_TEST)
            {
                uint32_t low = (uint32_t)addr;
                uint32_t high = (uint32_t)(addr >> 32);
                __asm__ volatile("wrmsr" : : "a"(low), "d"(high), "c"(0xc0000100));
            }
#endif
            return bh_op_result_value(0);

        case ARCH_GET_FS: {
            uint64_t val = st ? st->fs_base : 0;
            bh_status_t s = bh_copy_to_user((void *)addr, &val, sizeof(val));
            if (s != BH_OK) return bh_op_result_value(-LINUX_EFAULT);
            return bh_op_result_value(0);
        }

        case ARCH_SET_GS:
            if (st) st->gs_base = addr;
            return bh_op_result_value(0);

        case ARCH_GET_GS: {
            uint64_t val = st ? st->gs_base : 0;
            bh_status_t s = bh_copy_to_user((void *)addr, &val, sizeof(val));
            if (s != BH_OK) return bh_op_result_value(-LINUX_EFAULT);
            return bh_op_result_value(0);
        }

        default:
            return bh_op_result_value(-LINUX_EINVAL);
    }
}

static bh_operation_result_t linux_sys_uname(bh_syscall_ctx_t *ctx) {
    if (!ctx || !ctx->regs.arg[0]) return bh_op_result_value(-LINUX_EFAULT);

    struct linux_new_utsname uts;
    memset(&uts, 0, sizeof(uts));

    strncpy(uts.sysname, "Linux", sizeof(uts.sysname) - 1);
    strncpy(uts.nodename, "bharat-os", sizeof(uts.nodename) - 1);
    strncpy(uts.release, "5.15.0-bharat", sizeof(uts.release) - 1);
    strncpy(uts.version, "#1 SMP Bharat-OS 2026", sizeof(uts.version) - 1);
#if defined(__x86_64__)
    strncpy(uts.machine, "x86_64", sizeof(uts.machine) - 1);
#elif defined(__aarch64__)
    strncpy(uts.machine, "aarch64", sizeof(uts.machine) - 1);
#elif defined(__riscv) && (__riscv_xlen == 64)
    strncpy(uts.machine, "riscv64", sizeof(uts.machine) - 1);
#elif defined(__riscv) && (__riscv_xlen == 32)
    strncpy(uts.machine, "riscv32", sizeof(uts.machine) - 1);
#elif defined(__arm__)
    strncpy(uts.machine, "armv7l", sizeof(uts.machine) - 1);
#else
    strncpy(uts.machine, "unknown", sizeof(uts.machine) - 1);
#endif
    strncpy(uts.domainname, "localdomain", sizeof(uts.domainname) - 1);

    bh_status_t s = bh_copy_to_user((void *)ctx->regs.arg[0], &uts, sizeof(uts));
    if (s != BH_OK) return bh_op_result_value(-LINUX_EFAULT);

    return bh_op_result_value(0);
}

static bh_operation_result_t linux_sys_wait4(bh_syscall_ctx_t *ctx) {
    (void)ctx;
    return bh_op_result_value(-LINUX_ECHILD);
}

static bh_operation_result_t linux_sys_prlimit64(bh_syscall_ctx_t *ctx) {
    if (!ctx) return bh_op_result_value(-LINUX_EINVAL);
    int resource = (int)ctx->regs.arg[1];
    void *old_lim = (void *)ctx->regs.arg[3];

    if (old_lim) {
        linux_rlimit64_t lim = { .rlim_cur = UINT64_MAX, .rlim_max = UINT64_MAX };
        if (resource == LINUX_RLIMIT_STACK) {
            lim.rlim_cur = 8ULL * 1024ULL * 1024ULL;
            lim.rlim_max = 8ULL * 1024ULL * 1024ULL;
        } else if (resource == LINUX_RLIMIT_NOFILE) {
            lim.rlim_cur = 1024ULL;
            lim.rlim_max = 4096ULL;
        }
        bh_status_t s = bh_copy_to_user(old_lim, &lim, sizeof(lim));
        if (s != BH_OK) return bh_op_result_value(-LINUX_EFAULT);
    }
    return bh_op_result_value(0);
}

static bh_operation_result_t linux_sys_rt_sigaction(bh_syscall_ctx_t *ctx) {
    int signum = (int)ctx->regs.arg[0];
    if (signum < 1 || signum > 64 || signum == 9 || signum == 19) {
        return bh_op_result_value(-LINUX_EINVAL);
    }
    return bh_op_result_value(0);
}

static bh_operation_result_t linux_sys_rt_sigprocmask(bh_syscall_ctx_t *ctx) {
    if (!ctx) return bh_op_result_value(-LINUX_EINVAL);
    int how = (int)ctx->regs.arg[0];
    const uint64_t *set = (const uint64_t *)ctx->regs.arg[1];
    uint64_t *oldset = (uint64_t *)ctx->regs.arg[2];

    linux_process_state_t *st = linux_get_or_create_process_state(ctx->process);

    if (oldset && st) {
        bh_status_t s = bh_copy_to_user(oldset, &st->signal_mask, sizeof(uint64_t));
        if (s != BH_OK) return bh_op_result_value(-LINUX_EFAULT);
    }

    if (set && st) {
        uint64_t newmask = 0;
        bh_status_t s = bh_copy_from_user(&newmask, set, sizeof(uint64_t));
        if (s != BH_OK) return bh_op_result_value(-LINUX_EFAULT);

        if (how == 0) {
            st->signal_mask |= newmask;
        } else if (how == 1) {
            st->signal_mask &= ~newmask;
        } else if (how == 2) {
            st->signal_mask = newmask;
        } else {
            return bh_op_result_value(-LINUX_EINVAL);
        }
    }

    return bh_op_result_value(0);
}

static bh_operation_result_t linux_sys_rt_sigreturn(bh_syscall_ctx_t *ctx) {
    (void)ctx;
    return bh_op_result_value(0);
}

static void linux_fd_entry_ref(linux_fd_map_t *entry) {
    if (!entry) return;
    if (entry->type == LINUX_FD_TYPE_PIPE_READ || entry->type == LINUX_FD_TYPE_PIPE_WRITE) {
        linux_pipe_ref((linux_pipe_t *)entry->priv_data);
    } else if (entry->type == LINUX_FD_TYPE_EVENTFD) {
        linux_eventfd_ref((linux_eventfd_t *)entry->priv_data);
    } else if (entry->type == LINUX_FD_TYPE_TIMERFD) {
        linux_timerfd_ref((linux_timerfd_t *)entry->priv_data);
    } else if (entry->type == LINUX_FD_TYPE_EPOLL) {
        linux_epoll_ref((linux_epoll_t *)entry->priv_data);
    } else if (entry->type == LINUX_FD_TYPE_PROC) {
        linux_procfs_file_ref((linux_procfs_file_t *)entry->priv_data);
    }
}

static bh_operation_result_t linux_sys_pipe2(bh_syscall_ctx_t *ctx) {
    if (!ctx) return bh_op_result_value(-LINUX_EINVAL);
    int *pipefd = (int *)ctx->regs.arg[0];
    int flags = (int)ctx->regs.arg[1];

    if (!pipefd) return bh_op_result_value(-LINUX_EFAULT);

    linux_process_state_t *st = linux_get_or_create_process_state(ctx->process);
    if (!st) return bh_op_result_value(-LINUX_ENOMEM);

    int r_fd = -1, w_fd = -1;
    for (int i = 3; i < LINUX_MAX_FDS; i++) {
        if (st->fd_table[i].type == LINUX_FD_TYPE_NONE) {
            if (r_fd == -1) r_fd = i;
            else if (w_fd == -1) { w_fd = i; break; }
        }
    }
    if (r_fd == -1 || w_fd == -1) return bh_op_result_value(-LINUX_EMFILE);

    linux_pipe_t *pipe = linux_pipe_alloc();
    if (!pipe) return bh_op_result_value(-LINUX_ENFILE);

    st->fd_table[r_fd].type = LINUX_FD_TYPE_PIPE_READ;
    st->fd_table[r_fd].linux_fd = r_fd;
    st->fd_table[r_fd].open_flags = (uint32_t)flags;
    st->fd_table[r_fd].priv_data = pipe;
    st->fd_table[r_fd].ref_count = 1;

    st->fd_table[w_fd].type = LINUX_FD_TYPE_PIPE_WRITE;
    st->fd_table[w_fd].linux_fd = w_fd;
    st->fd_table[w_fd].open_flags = (uint32_t)flags;
    st->fd_table[w_fd].priv_data = pipe;
    st->fd_table[w_fd].ref_count = 1;

    int fds[2] = { r_fd, w_fd };
    bh_status_t s = bh_copy_to_user(pipefd, fds, sizeof(fds));
    if (s != BH_OK) {
        linux_pipe_close_read(pipe);
        linux_pipe_close_write(pipe);
        st->fd_table[r_fd].type = LINUX_FD_TYPE_NONE;
        st->fd_table[w_fd].type = LINUX_FD_TYPE_NONE;
        return bh_op_result_value(-LINUX_EFAULT);
    }

    return bh_op_result_value(0);
}

static bh_operation_result_t linux_sys_pipe(bh_syscall_ctx_t *ctx) {
    if (!ctx) return bh_op_result_value(-LINUX_EINVAL);
    ctx->regs.arg[1] = 0;
    return linux_sys_pipe2(ctx);
}

static bh_operation_result_t linux_sys_dup(bh_syscall_ctx_t *ctx) {
    int oldfd = (int)ctx->regs.arg[0];
    if (oldfd < 0 || oldfd >= LINUX_MAX_FDS) return bh_op_result_value(-LINUX_EBADF);

    linux_process_state_t *st = linux_get_or_create_process_state(ctx->process);
    if (!st || st->fd_table[oldfd].type == LINUX_FD_TYPE_NONE) {
        return bh_op_result_value(-LINUX_EBADF);
    }

    int newfd = -1;
    for (int i = 0; i < LINUX_MAX_FDS; i++) {
        if (st->fd_table[i].type == LINUX_FD_TYPE_NONE) {
            newfd = i;
            break;
        }
    }
    if (newfd == -1) return bh_op_result_value(-LINUX_EMFILE);

    st->fd_table[newfd] = st->fd_table[oldfd];
    st->fd_table[newfd].linux_fd = newfd;
    st->fd_table[newfd].ref_count = 1;
    st->fd_table[newfd].open_flags &= ~LINUX_O_CLOEXEC;
    linux_fd_entry_ref(&st->fd_table[newfd]);

    return bh_op_result_value(newfd);
}

static bh_operation_result_t linux_sys_dup2(bh_syscall_ctx_t *ctx) {
    int oldfd = (int)ctx->regs.arg[0];
    int newfd = (int)ctx->regs.arg[1];
    if (oldfd < 0 || oldfd >= LINUX_MAX_FDS || newfd < 0 || newfd >= LINUX_MAX_FDS) {
        return bh_op_result_value(-LINUX_EBADF);
    }
    if (oldfd == newfd) return bh_op_result_value(newfd);

    linux_process_state_t *st = linux_get_or_create_process_state(ctx->process);
    if (!st || st->fd_table[oldfd].type == LINUX_FD_TYPE_NONE) {
        return bh_op_result_value(-LINUX_EBADF);
    }

    if (st->fd_table[newfd].type != LINUX_FD_TYPE_NONE) {
        ctx->regs.arg[0] = newfd;
        linux_sys_close(ctx);
        ctx->regs.arg[0] = oldfd;
    }

    st->fd_table[newfd] = st->fd_table[oldfd];
    st->fd_table[newfd].linux_fd = newfd;
    st->fd_table[newfd].ref_count = 1;
    st->fd_table[newfd].open_flags &= ~LINUX_O_CLOEXEC;
    linux_fd_entry_ref(&st->fd_table[newfd]);

    return bh_op_result_value(newfd);
}

static bh_operation_result_t linux_sys_dup3(bh_syscall_ctx_t *ctx) {
    int oldfd = (int)ctx->regs.arg[0];
    int newfd = (int)ctx->regs.arg[1];
    int flags = (int)ctx->regs.arg[2];

    if (oldfd == newfd) return bh_op_result_value(-LINUX_EINVAL);
    bh_operation_result_t res = linux_sys_dup2(ctx);
    if (res.domain == BH_STATUS_DOMAIN_VALUE && res.value >= 0) {
        linux_process_state_t *st = linux_get_or_create_process_state(ctx->process);
        if (st && (flags & LINUX_O_CLOEXEC)) {
            st->fd_table[newfd].open_flags |= LINUX_O_CLOEXEC;
        }
    }
    return res;
}

static bh_operation_result_t linux_sys_fcntl(bh_syscall_ctx_t *ctx) {
    int fd = (int)ctx->regs.arg[0];
    int cmd = (int)ctx->regs.arg[1];
    long arg = (long)ctx->regs.arg[2];

    if (fd < 0 || fd >= LINUX_MAX_FDS) return bh_op_result_value(-LINUX_EBADF);
    linux_process_state_t *st = linux_get_or_create_process_state(ctx->process);
    if (!st || st->fd_table[fd].type == LINUX_FD_TYPE_NONE) {
        return bh_op_result_value(-LINUX_EBADF);
    }

    switch (cmd) {
        case LINUX_F_DUPFD:
        case LINUX_F_DUPFD_CLOEXEC: {
            if (arg < 0 || arg >= LINUX_MAX_FDS) return bh_op_result_value(-LINUX_EINVAL);
            int newfd = -1;
            for (int i = (int)arg; i < LINUX_MAX_FDS; i++) {
                if (st->fd_table[i].type == LINUX_FD_TYPE_NONE) {
                    newfd = i;
                    break;
                }
            }
            if (newfd == -1) return bh_op_result_value(-LINUX_EMFILE);
            st->fd_table[newfd] = st->fd_table[fd];
            st->fd_table[newfd].linux_fd = newfd;
            st->fd_table[newfd].ref_count = 1;
            if (cmd == LINUX_F_DUPFD_CLOEXEC) {
                st->fd_table[newfd].open_flags |= LINUX_O_CLOEXEC;
            } else {
                st->fd_table[newfd].open_flags &= ~LINUX_O_CLOEXEC;
            }
            linux_fd_entry_ref(&st->fd_table[newfd]);
            return bh_op_result_value(newfd);
        }

        case LINUX_F_GETFD:
            return bh_op_result_value((st->fd_table[fd].open_flags & LINUX_O_CLOEXEC) ? LINUX_FD_CLOEXEC : 0);

        case LINUX_F_SETFD:
            if (arg & LINUX_FD_CLOEXEC) {
                st->fd_table[fd].open_flags |= LINUX_O_CLOEXEC;
            } else {
                st->fd_table[fd].open_flags &= ~LINUX_O_CLOEXEC;
            }
            return bh_op_result_value(0);

        case LINUX_F_GETFL:
            return bh_op_result_value(st->fd_table[fd].open_flags);

        case LINUX_F_SETFL:
            st->fd_table[fd].open_flags = (st->fd_table[fd].open_flags & ~LINUX_O_NONBLOCK) | ((uint32_t)arg & LINUX_O_NONBLOCK);
            return bh_op_result_value(0);

        default:
            return bh_op_result_value(0);
    }
}

static bh_operation_result_t linux_sys_eventfd2(bh_syscall_ctx_t *ctx) {
    if (!ctx) return bh_op_result_value(-LINUX_EINVAL);
    unsigned int initval = (unsigned int)ctx->regs.arg[0];
    int flags = (int)ctx->regs.arg[1];

    linux_process_state_t *st = linux_get_or_create_process_state(ctx->process);
    if (!st) return bh_op_result_value(-LINUX_ENOMEM);

    int fd = -1;
    for (int i = 3; i < LINUX_MAX_FDS; i++) {
        if (st->fd_table[i].type == LINUX_FD_TYPE_NONE) {
            fd = i;
            break;
        }
    }
    if (fd == -1) return bh_op_result_value(-LINUX_EMFILE);

    linux_eventfd_t *efd = linux_eventfd_alloc(initval, flags);
    if (!efd) return bh_op_result_value(-LINUX_ENFILE);

    st->fd_table[fd].type = LINUX_FD_TYPE_EVENTFD;
    st->fd_table[fd].linux_fd = fd;
    st->fd_table[fd].open_flags = (uint32_t)flags;
    st->fd_table[fd].priv_data = efd;
    st->fd_table[fd].ref_count = 1;

    return bh_op_result_value(fd);
}

static bh_operation_result_t linux_sys_eventfd(bh_syscall_ctx_t *ctx) {
    if (!ctx) return bh_op_result_value(-LINUX_EINVAL);
    ctx->regs.arg[1] = 0;
    return linux_sys_eventfd2(ctx);
}


static bh_operation_result_t linux_sys_timerfd_create(bh_syscall_ctx_t *ctx) {
    if (!ctx) return bh_op_result_value(-LINUX_EINVAL);
    int clockid = (int)ctx->regs.arg[0];
    int flags = (int)ctx->regs.arg[1];

    linux_process_state_t *st = linux_get_or_create_process_state(ctx->process);
    if (!st) return bh_op_result_value(-LINUX_ENOMEM);

    int fd = -1;
    for (int i = 3; i < LINUX_MAX_FDS; i++) {
        if (st->fd_table[i].type == LINUX_FD_TYPE_NONE) {
            fd = i;
            break;
        }
    }
    if (fd == -1) return bh_op_result_value(-LINUX_EMFILE);

    linux_timerfd_t *tfd = linux_timerfd_alloc(clockid, flags);
    if (!tfd) return bh_op_result_value(-LINUX_ENFILE);

    st->fd_table[fd].type = LINUX_FD_TYPE_TIMERFD;
    st->fd_table[fd].linux_fd = fd;
    st->fd_table[fd].open_flags = (uint32_t)flags;
    st->fd_table[fd].priv_data = tfd;
    st->fd_table[fd].ref_count = 1;

    return bh_op_result_value(fd);
}

static bh_operation_result_t linux_sys_timerfd_settime(bh_syscall_ctx_t *ctx) {
    int fd = (int)ctx->regs.arg[0];
    int flags = (int)ctx->regs.arg[1];
    const struct linux_itimerspec *new_val = (const struct linux_itimerspec *)ctx->regs.arg[2];
    struct linux_itimerspec *old_val = (struct linux_itimerspec *)ctx->regs.arg[3];

    if (fd < 0 || fd >= LINUX_MAX_FDS || !new_val) return bh_op_result_value(-LINUX_EINVAL);
    linux_process_state_t *st = linux_get_or_create_process_state(ctx->process);
    if (!st || st->fd_table[fd].type != LINUX_FD_TYPE_TIMERFD) return bh_op_result_value(-LINUX_EBADF);

    struct linux_itimerspec nv;
    bh_status_t s = bh_copy_from_user(&nv, new_val, sizeof(nv));
    if (s != BH_OK) return bh_op_result_value(-LINUX_EFAULT);

    uint64_t init_ns = (uint64_t)nv.it_value.tv_sec * LINUX_NSEC_PER_SEC + (uint64_t)nv.it_value.tv_nsec;
    uint64_t intv_ns = (uint64_t)nv.it_interval.tv_sec * LINUX_NSEC_PER_SEC + (uint64_t)nv.it_interval.tv_nsec;
    uint64_t old_init = 0, old_intv = 0;

    linux_timerfd_t *tfd = (linux_timerfd_t *)st->fd_table[fd].priv_data;
    long ret = linux_timerfd_settime(tfd, flags, init_ns, intv_ns, &old_init, &old_intv);
    if (ret != 0) return bh_op_result_value(ret);

    if (old_val) {
        struct linux_itimerspec ov;
        ov.it_value.tv_sec = (int64_t)(old_init / LINUX_NSEC_PER_SEC);
        ov.it_value.tv_nsec = (int64_t)(old_init % LINUX_NSEC_PER_SEC);
        ov.it_interval.tv_sec = (int64_t)(old_intv / LINUX_NSEC_PER_SEC);
        ov.it_interval.tv_nsec = (int64_t)(old_intv % LINUX_NSEC_PER_SEC);
        bh_copy_to_user(old_val, &ov, sizeof(ov));
    }

    return bh_op_result_value(0);
}

static bh_operation_result_t linux_sys_timerfd_gettime(bh_syscall_ctx_t *ctx) {
    int fd = (int)ctx->regs.arg[0];
    struct linux_itimerspec *curr_val = (struct linux_itimerspec *)ctx->regs.arg[1];
    if (fd < 0 || fd >= LINUX_MAX_FDS || !curr_val) return bh_op_result_value(-LINUX_EINVAL);

    linux_process_state_t *st = linux_get_or_create_process_state(ctx->process);
    if (!st || st->fd_table[fd].type != LINUX_FD_TYPE_TIMERFD) return bh_op_result_value(-LINUX_EBADF);

    uint64_t cur_init = 0, cur_intv = 0;
    linux_timerfd_t *tfd = (linux_timerfd_t *)st->fd_table[fd].priv_data;
    long ret = linux_timerfd_gettime(tfd, &cur_init, &cur_intv);
    if (ret != 0) return bh_op_result_value(ret);

    struct linux_itimerspec cv;
    cv.it_value.tv_sec = (int64_t)(cur_init / LINUX_NSEC_PER_SEC);
    cv.it_value.tv_nsec = (int64_t)(cur_init % LINUX_NSEC_PER_SEC);
    cv.it_interval.tv_sec = (int64_t)(cur_intv / LINUX_NSEC_PER_SEC);
    cv.it_interval.tv_nsec = (int64_t)(cur_intv % LINUX_NSEC_PER_SEC);
    bh_copy_to_user(curr_val, &cv, sizeof(cv));

    return bh_op_result_value(0);
}

static bh_operation_result_t linux_sys_epoll_create1(bh_syscall_ctx_t *ctx) {
    if (!ctx) return bh_op_result_value(-LINUX_EINVAL);
    int flags = (int)ctx->regs.arg[0];

    linux_process_state_t *st = linux_get_or_create_process_state(ctx->process);
    if (!st) return bh_op_result_value(-LINUX_ENOMEM);

    int fd = -1;
    for (int i = 3; i < LINUX_MAX_FDS; i++) {
        if (st->fd_table[i].type == LINUX_FD_TYPE_NONE) {
            fd = i;
            break;
        }
    }
    if (fd == -1) return bh_op_result_value(-LINUX_EMFILE);

    linux_epoll_t *ep = linux_epoll_alloc(flags);
    if (!ep) return bh_op_result_value(-LINUX_ENFILE);

    st->fd_table[fd].type = LINUX_FD_TYPE_EPOLL;
    st->fd_table[fd].linux_fd = fd;
    st->fd_table[fd].open_flags = (uint32_t)flags;
    st->fd_table[fd].priv_data = ep;
    st->fd_table[fd].ref_count = 1;

    return bh_op_result_value(fd);
}

static bh_operation_result_t linux_sys_epoll_create(bh_syscall_ctx_t *ctx) {
    if (!ctx) return bh_op_result_value(-LINUX_EINVAL);
    int size = (int)ctx->regs.arg[0];
    if (size <= 0) return bh_op_result_value(-LINUX_EINVAL);
    ctx->regs.arg[0] = 0;
    return linux_sys_epoll_create1(ctx);
}

static bh_operation_result_t linux_sys_epoll_ctl(bh_syscall_ctx_t *ctx) {
    int epfd = (int)ctx->regs.arg[0];
    int op = (int)ctx->regs.arg[1];
    int fd = (int)ctx->regs.arg[2];
    const struct linux_epoll_event *event = (const struct linux_epoll_event *)ctx->regs.arg[3];

    if (epfd < 0 || epfd >= LINUX_MAX_FDS || fd < 0 || fd >= LINUX_MAX_FDS) {
        return bh_op_result_value(-LINUX_EBADF);
    }

    linux_process_state_t *st = linux_get_or_create_process_state(ctx->process);
    if (!st || st->fd_table[epfd].type != LINUX_FD_TYPE_EPOLL || st->fd_table[fd].type == LINUX_FD_TYPE_NONE) {
        return bh_op_result_value(-LINUX_EBADF);
    }

    struct linux_epoll_event ev;
    if (event && (op == LINUX_EPOLL_CTL_ADD || op == LINUX_EPOLL_CTL_MOD)) {
        bh_status_t s = bh_copy_from_user(&ev, event, sizeof(ev));
        if (s != BH_OK) return bh_op_result_value(-LINUX_EFAULT);
    }

    linux_epoll_t *ep = (linux_epoll_t *)st->fd_table[epfd].priv_data;
    long ret = linux_epoll_ctl(ep, op, fd, &ev);
    return bh_op_result_value(ret);
}

static bh_operation_result_t linux_sys_epoll_wait(bh_syscall_ctx_t *ctx) {
    int epfd = (int)ctx->regs.arg[0];
    struct linux_epoll_event *events = (struct linux_epoll_event *)ctx->regs.arg[1];
    int maxevents = (int)ctx->regs.arg[2];
    int timeout = (int)ctx->regs.arg[3];

    if (epfd < 0 || epfd >= LINUX_MAX_FDS || !events || maxevents <= 0) {
        return bh_op_result_value(-LINUX_EINVAL);
    }

    linux_process_state_t *st = linux_get_or_create_process_state(ctx->process);
    if (!st || st->fd_table[epfd].type != LINUX_FD_TYPE_EPOLL) {
        return bh_op_result_value(-LINUX_EBADF);
    }

    struct linux_epoll_event k_events[16];
    int fetch_count = maxevents > 16 ? 16 : maxevents;

    linux_epoll_t *ep = (linux_epoll_t *)st->fd_table[epfd].priv_data;
    long ret = linux_epoll_wait(ep, st, k_events, fetch_count, timeout);
    if (ret > 0) {
        bh_status_t s = bh_copy_to_user(events, k_events, (size_t)ret * sizeof(struct linux_epoll_event));
        if (s != BH_OK) return bh_op_result_value(-LINUX_EFAULT);
    }
    return bh_op_result_value(ret);
}

static bh_operation_result_t linux_sys_epoll_pwait(bh_syscall_ctx_t *ctx) {
    return linux_sys_epoll_wait(ctx);
}

static bh_operation_result_t linux_sys_poll(bh_syscall_ctx_t *ctx) {
    struct linux_pollfd *fds = (struct linux_pollfd *)ctx->regs.arg[0];
    uint64_t nfds = ctx->regs.arg[1];
    int timeout = (int)ctx->regs.arg[2];

    if (!fds && nfds > 0) return bh_op_result_value(-LINUX_EFAULT);
    if (nfds == 0) return bh_op_result_value(0);
    if (nfds > 64) nfds = 64;

    linux_process_state_t *st = linux_get_or_create_process_state(ctx->process);
    if (!st) return bh_op_result_value(-LINUX_ENOMEM);

    struct linux_pollfd k_fds[64];
    bh_status_t s = bh_copy_from_user(k_fds, fds, (size_t)nfds * sizeof(struct linux_pollfd));
    if (s != BH_OK) return bh_op_result_value(-LINUX_EFAULT);

    uint64_t start_ns = bh_ktime_now();
    uint64_t timeout_ns = (timeout > 0) ? ((uint64_t)timeout * 1000000ULL) : 0;

    while (1) {
        int ready = 0;
        for (uint64_t i = 0; i < nfds; i++) {
            k_fds[i].revents = 0;
            int fd = k_fds[i].fd;
            if (fd < 0 || fd >= LINUX_MAX_FDS || st->fd_table[fd].type == LINUX_FD_TYPE_NONE) {
                k_fds[i].revents = LINUX_POLLNVAL;
                ready++;
                continue;
            }

            short rev = 0;
            switch (st->fd_table[fd].type) {
                case LINUX_FD_TYPE_CONSOLE:
                    rev |= LINUX_POLLOUT;
                    break;
                case LINUX_FD_TYPE_DEV_NULL:
                case LINUX_FD_TYPE_DEV_ZERO:
                case LINUX_FD_TYPE_DEV_URANDOM:
                case LINUX_FD_TYPE_PROC:
                    rev |= (LINUX_POLLIN | LINUX_POLLOUT);
                    break;
                case LINUX_FD_TYPE_PIPE_READ: {
                    linux_pipe_t *p = (linux_pipe_t *)st->fd_table[fd].priv_data;
                    if (p) {
                        if (linux_pipe_is_readable(p)) rev |= LINUX_POLLIN;
                        if (!p->writer_open) rev |= LINUX_POLLHUP;
                    }
                    break;
                }
                case LINUX_FD_TYPE_PIPE_WRITE: {
                    linux_pipe_t *p = (linux_pipe_t *)st->fd_table[fd].priv_data;
                    if (p) {
                        if (linux_pipe_is_writable(p)) rev |= LINUX_POLLOUT;
                        if (!p->reader_open) rev |= LINUX_POLLERR;
                    }
                    break;
                }
                case LINUX_FD_TYPE_EVENTFD: {
                    linux_eventfd_t *efd = (linux_eventfd_t *)st->fd_table[fd].priv_data;
                    if (efd && linux_eventfd_is_readable(efd)) rev |= LINUX_POLLIN;
                    break;
                }
                case LINUX_FD_TYPE_TIMERFD: {
                    linux_timerfd_t *tfd = (linux_timerfd_t *)st->fd_table[fd].priv_data;
                    if (tfd && linux_timerfd_is_readable(tfd)) rev |= LINUX_POLLIN;
                    break;
                }
                default:
                    rev |= (LINUX_POLLIN | LINUX_POLLOUT);
                    break;
            }

            short matched = rev & k_fds[i].events;
            if (matched) {
                k_fds[i].revents = matched;
                ready++;
            }
        }

        if (ready > 0 || timeout == 0) {
            bh_copy_to_user(fds, k_fds, (size_t)nfds * sizeof(struct linux_pollfd));
            return bh_op_result_value(ready);
        }

        if (timeout > 0 && (bh_ktime_now() - start_ns) >= timeout_ns) {
            bh_copy_to_user(fds, k_fds, (size_t)nfds * sizeof(struct linux_pollfd));
            return bh_op_result_value(0);
        }

        bh_thread_yield();
    }
}

static bh_operation_result_t linux_sys_ppoll(bh_syscall_ctx_t *ctx) {
    return linux_sys_poll(ctx);
}

static bh_operation_result_t linux_sys_getcwd(bh_syscall_ctx_t *ctx) {
    char *buf = (char *)ctx->regs.arg[0];
    size_t size = (size_t)ctx->regs.arg[1];
    if (!buf || size == 0) return bh_op_result_value(-LINUX_EINVAL);

    linux_process_state_t *st = linux_get_or_create_process_state(ctx->process);
    const char *cwd = (st && st->cwd[0]) ? st->cwd : "/";
    size_t len = strlen(cwd) + 1;
    if (len > size) return bh_op_result_value(-LINUX_ERANGE);

    bh_status_t s = bh_copy_to_user(buf, cwd, len);
    if (s != BH_OK) return bh_op_result_value(-LINUX_EFAULT);

    return bh_op_result_value((long)len);
}

static bh_operation_result_t linux_sys_chdir(bh_syscall_ctx_t *ctx) {
    const char *path = (const char *)ctx->regs.arg[0];
    if (!path) return bh_op_result_value(-LINUX_EFAULT);

    char path_buf[256];
    bh_status_t s = bh_copy_from_user(path_buf, path, sizeof(path_buf) - 1);
    if (s != BH_OK) return bh_op_result_value(-LINUX_EFAULT);
    path_buf[sizeof(path_buf) - 1] = '\0';

    linux_process_state_t *st = linux_get_or_create_process_state(ctx->process);
    if (!st) return bh_op_result_value(-LINUX_ENOMEM);

    if (path_buf[0] == '\0') {
        return bh_op_result_value(-LINUX_ENOENT);
    }
    if (strcmp(path_buf, ".") == 0) {
        return bh_op_result_value(0);
    }
    if (path_buf[0] == '/') {
        strncpy(st->cwd, path_buf, sizeof(st->cwd) - 1);
        st->cwd[sizeof(st->cwd) - 1] = '\0';
        return bh_op_result_value(0);
    }
    size_t cur_len = strlen(st->cwd);
    if (cur_len > 1 && st->cwd[cur_len - 1] != '/' && cur_len + 1 < sizeof(st->cwd)) {
        st->cwd[cur_len++] = '/';
        st->cwd[cur_len] = '\0';
    }
    size_t rem = sizeof(st->cwd) - cur_len;
    if (rem > 1) {
        strncpy(&st->cwd[cur_len], path_buf, rem - 1);
        st->cwd[sizeof(st->cwd) - 1] = '\0';
    }
    return bh_op_result_value(0);
}

static bh_operation_result_t linux_sys_access(bh_syscall_ctx_t *ctx) {
    const char *pathname = (const char *)ctx->regs.arg[0];
    if (!pathname) return bh_op_result_value(-LINUX_EFAULT);

    char path_buf[256];
    bh_status_t s = bh_copy_from_user(path_buf, pathname, sizeof(path_buf) - 1);
    if (s != BH_OK) return bh_op_result_value(-LINUX_EFAULT);
    path_buf[sizeof(path_buf) - 1] = '\0';

    if (strcmp(path_buf, "/") == 0 || strcmp(path_buf, "/bin") == 0 ||
        strcmp(path_buf, "/dev") == 0 || strcmp(path_buf, "/proc") == 0 ||
        strcmp(path_buf, "/tmp") == 0 || strcmp(path_buf, "/dev/null") == 0 ||
        strcmp(path_buf, "/dev/zero") == 0 || strcmp(path_buf, "/dev/urandom") == 0 ||
        strcmp(path_buf, "/dev/tty") == 0 || strcmp(path_buf, "/bin/sh") == 0 ||
        strncmp(path_buf, "/proc/", 6) == 0) {
        return bh_op_result_value(0);
    }

    return bh_op_result_value(-LINUX_ENOENT);
}

static bh_operation_result_t linux_sys_faccessat(bh_syscall_ctx_t *ctx) {
    ctx->regs.arg[0] = ctx->regs.arg[1];
    return linux_sys_access(ctx);
}

static int fill_linux_stat(struct linux_stat *st_buf, uint32_t mode, int64_t size, uint64_t rdev) {
    memset(st_buf, 0, sizeof(*st_buf));
    st_buf->st_dev = 1;
    st_buf->st_ino = 100;
    st_buf->st_nlink = 1;
    st_buf->st_mode = mode;
    st_buf->st_uid = 0;
    st_buf->st_gid = 0;
    st_buf->st_rdev = rdev;
    st_buf->st_size = size;
    st_buf->st_blksize = 4096;
    st_buf->st_blocks = (size + 511) / 512;
    st_buf->st_atime = 1700000000;
    st_buf->st_mtime = 1700000000;
    st_buf->st_ctime = 1700000000;
    return 0;
}

static bh_operation_result_t linux_sys_fstat(bh_syscall_ctx_t *ctx) {
    int fd = (int)ctx->regs.arg[0];
    struct linux_stat *statbuf = (struct linux_stat *)ctx->regs.arg[1];
    if (fd < 0 || fd >= LINUX_MAX_FDS || !statbuf) return bh_op_result_value(-LINUX_EINVAL);

    linux_process_state_t *st = linux_get_or_create_process_state(ctx->process);
    if (!st || st->fd_table[fd].type == LINUX_FD_TYPE_NONE) return bh_op_result_value(-LINUX_EBADF);

    struct linux_stat kst;
    switch (st->fd_table[fd].type) {
        case LINUX_FD_TYPE_CONSOLE:
            fill_linux_stat(&kst, LINUX_S_IFCHR | 0620, 0, 0x0501);
            break;
        case LINUX_FD_TYPE_DEV_NULL:
            fill_linux_stat(&kst, LINUX_S_IFCHR | 0666, 0, 0x0103);
            break;
        case LINUX_FD_TYPE_DEV_ZERO:
            fill_linux_stat(&kst, LINUX_S_IFCHR | 0666, 0, 0x0105);
            break;
        case LINUX_FD_TYPE_DEV_URANDOM:
            fill_linux_stat(&kst, LINUX_S_IFCHR | 0666, 0, 0x0109);
            break;
        case LINUX_FD_TYPE_PIPE_READ:
        case LINUX_FD_TYPE_PIPE_WRITE:
            fill_linux_stat(&kst, LINUX_S_IFIFO | 0600, 0, 0);
            break;
        case LINUX_FD_TYPE_PROC: {
            linux_procfs_file_t *pf = (linux_procfs_file_t *)st->fd_table[fd].priv_data;
            fill_linux_stat(&kst, LINUX_S_IFREG | 0444, pf ? (int64_t)pf->size : 0, 0);
            break;
        }
        default:
            fill_linux_stat(&kst, LINUX_S_IFREG | 0644, 0, 0);
            break;
    }

    bh_status_t s = bh_copy_to_user(statbuf, &kst, sizeof(kst));
    if (s != BH_OK) return bh_op_result_value(-LINUX_EFAULT);
    return bh_op_result_value(0);
}

static bh_operation_result_t linux_sys_stat(bh_syscall_ctx_t *ctx) {
    const char *pathname = (const char *)ctx->regs.arg[0];
    struct linux_stat *statbuf = (struct linux_stat *)ctx->regs.arg[1];
    if (!pathname || !statbuf) return bh_op_result_value(-LINUX_EFAULT);

    char path_buf[256];
    bh_status_t s = bh_copy_from_user(path_buf, pathname, sizeof(path_buf) - 1);
    if (s != BH_OK) return bh_op_result_value(-LINUX_EFAULT);
    path_buf[sizeof(path_buf) - 1] = '\0';

    struct linux_stat kst;
    if (strcmp(path_buf, "/") == 0 || strcmp(path_buf, "/bin") == 0 ||
        strcmp(path_buf, "/dev") == 0 || strcmp(path_buf, "/proc") == 0 ||
        strcmp(path_buf, "/tmp") == 0) {
        fill_linux_stat(&kst, LINUX_S_IFDIR | 0755, 4096, 0);
    } else if (strcmp(path_buf, "/dev/null") == 0) {
        fill_linux_stat(&kst, LINUX_S_IFCHR | 0666, 0, 0x0103);
    } else if (strcmp(path_buf, "/dev/zero") == 0) {
        fill_linux_stat(&kst, LINUX_S_IFCHR | 0666, 0, 0x0105);
    } else if (strcmp(path_buf, "/dev/urandom") == 0) {
        fill_linux_stat(&kst, LINUX_S_IFCHR | 0666, 0, 0x0109);
    } else if (strcmp(path_buf, "/dev/tty") == 0 || strcmp(path_buf, "/dev/console") == 0) {
        fill_linux_stat(&kst, LINUX_S_IFCHR | 0620, 0, 0x0501);
    } else if (strncmp(path_buf, "/proc/", 6) == 0) {
        fill_linux_stat(&kst, LINUX_S_IFREG | 0444, 256, 0);
    } else if (strcmp(path_buf, "/bin/sh") == 0) {
        fill_linux_stat(&kst, LINUX_S_IFREG | 0755, 65536, 0);
    } else {
        return bh_op_result_value(-LINUX_ENOENT);
    }

    s = bh_copy_to_user(statbuf, &kst, sizeof(kst));
    if (s != BH_OK) return bh_op_result_value(-LINUX_EFAULT);
    return bh_op_result_value(0);
}

static bh_operation_result_t linux_sys_lstat(bh_syscall_ctx_t *ctx) {
    return linux_sys_stat(ctx);
}

static bh_operation_result_t linux_sys_newfstatat(bh_syscall_ctx_t *ctx) {
    ctx->regs.arg[0] = ctx->regs.arg[1];
    ctx->regs.arg[1] = ctx->regs.arg[2];
    return linux_sys_stat(ctx);
}

static bh_operation_result_t linux_sys_readlink(bh_syscall_ctx_t *ctx) {
    const char *pathname = (const char *)ctx->regs.arg[0];
    char *buf = (char *)ctx->regs.arg[1];
    size_t bufsiz = (size_t)ctx->regs.arg[2];

    if (!pathname || !buf || bufsiz == 0) return bh_op_result_value(-LINUX_EINVAL);

    char path_buf[256];
    bh_status_t s = bh_copy_from_user(path_buf, pathname, sizeof(path_buf) - 1);
    if (s != BH_OK) return bh_op_result_value(-LINUX_EFAULT);
    path_buf[sizeof(path_buf) - 1] = '\0';

    if (strcmp(path_buf, "/proc/self/exe") == 0) {
        const char *target = "/bin/sh";
        size_t len = strlen(target);
        if (len > bufsiz) len = bufsiz;
        s = bh_copy_to_user(buf, target, len);
        if (s != BH_OK) return bh_op_result_value(-LINUX_EFAULT);
        return bh_op_result_value((long)len);
    }

    return bh_op_result_value(-LINUX_EINVAL);
}

static bh_operation_result_t linux_sys_readlinkat(bh_syscall_ctx_t *ctx) {
    ctx->regs.arg[0] = ctx->regs.arg[1];
    ctx->regs.arg[1] = ctx->regs.arg[2];
    ctx->regs.arg[2] = ctx->regs.arg[3];
    return linux_sys_readlink(ctx);
}

static bh_operation_result_t linux_sys_fork(bh_syscall_ctx_t *ctx) {
    ctx->regs.arg[0] = 17; // SIGCHLD
    ctx->regs.arg[1] = 0;
    return linux_sys_clone(ctx);
}

static bh_operation_result_t linux_sys_vfork(bh_syscall_ctx_t *ctx) {
    return linux_sys_fork(ctx);
}

static bh_operation_result_t linux_sys_execve(bh_syscall_ctx_t *ctx) {
    const char *filename = (const char *)ctx->regs.arg[0];
    if (!filename) return bh_op_result_value(-LINUX_EFAULT);

    char path_buf[256];
    bh_status_t s = bh_copy_from_user(path_buf, filename, sizeof(path_buf) - 1);
    if (s != BH_OK) return bh_op_result_value(-LINUX_EFAULT);
    path_buf[sizeof(path_buf) - 1] = '\0';

    if (strcmp(path_buf, "/bin/sh") != 0 && strcmp(path_buf, "/bin/busybox") != 0 &&
        strcmp(path_buf, "/bin/ls") != 0 && strcmp(path_buf, "/bin/echo") != 0) {
        return bh_op_result_value(-LINUX_ENOENT);
    }

    linux_process_state_t *st = linux_get_or_create_process_state(ctx->process);
    if (st) {
        for (int fd = 0; fd < LINUX_MAX_FDS; fd++) {
            if (st->fd_table[fd].type != LINUX_FD_TYPE_NONE && (st->fd_table[fd].open_flags & LINUX_O_CLOEXEC)) {
                ctx->regs.arg[0] = fd;
                linux_sys_close(ctx);
            }
        }
    }

    return bh_op_result_value(0);
}

static bh_operation_result_t linux_sys_socketpair(bh_syscall_ctx_t *ctx) {
    int *sv = (int *)ctx->regs.arg[3];
    ctx->regs.arg[0] = (uintptr_t)sv;
    ctx->regs.arg[1] = 0;
    return linux_sys_pipe2(ctx);
}

static const bh_syscall_meta_t linux_syscall_table_x86_64[] = {
    [LINUX_X86_64_SYS_READ]             = { .nr = LINUX_X86_64_SYS_READ, .name = "read", .class_id = BH_SYS_CLASS_IO, .arg_count = 3, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_WRITE, .handler = linux_sys_read },
    [LINUX_X86_64_SYS_WRITE]            = { .nr = LINUX_X86_64_SYS_WRITE, .name = "write", .class_id = BH_SYS_CLASS_IO, .arg_count = 3, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ, .handler = linux_sys_write },
    [LINUX_X86_64_SYS_OPEN]             = { .nr = LINUX_X86_64_SYS_OPEN, .name = "open", .class_id = BH_SYS_CLASS_IO, .arg_count = 3, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ, .handler = linux_sys_open },
    [LINUX_X86_64_SYS_CLOSE]            = { .nr = LINUX_X86_64_SYS_CLOSE, .name = "close", .class_id = BH_SYS_CLASS_IO, .arg_count = 1, .flags = BH_SYSCALL_F_BLOCKING, .handler = linux_sys_close },
    [LINUX_X86_64_SYS_STAT]             = { .nr = LINUX_X86_64_SYS_STAT, .name = "stat", .class_id = BH_SYS_CLASS_IO, .arg_count = 2, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ | BH_SYSCALL_F_USER_WRITE, .handler = linux_sys_stat },
    [LINUX_X86_64_SYS_FSTAT]            = { .nr = LINUX_X86_64_SYS_FSTAT, .name = "fstat", .class_id = BH_SYS_CLASS_IO, .arg_count = 2, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_WRITE, .handler = linux_sys_fstat },
    [LINUX_X86_64_SYS_LSTAT]            = { .nr = LINUX_X86_64_SYS_LSTAT, .name = "lstat", .class_id = BH_SYS_CLASS_IO, .arg_count = 2, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ | BH_SYSCALL_F_USER_WRITE, .handler = linux_sys_lstat },
    [LINUX_X86_64_SYS_POLL]             = { .nr = LINUX_X86_64_SYS_POLL, .name = "poll", .class_id = BH_SYS_CLASS_IO, .arg_count = 3, .flags = BH_SYSCALL_F_BLOCKING, .handler = linux_sys_poll },
    [LINUX_X86_64_SYS_MMAP]             = { .nr = LINUX_X86_64_SYS_MMAP, .name = "mmap", .class_id = BH_SYS_CLASS_MEMORY, .arg_count = 6, .handler = linux_sys_mmap },
    [LINUX_X86_64_SYS_MPROTECT]         = { .nr = LINUX_X86_64_SYS_MPROTECT, .name = "mprotect", .class_id = BH_SYS_CLASS_MEMORY, .arg_count = 3, .handler = linux_sys_mprotect },
    [LINUX_X86_64_SYS_MUNMAP]           = { .nr = LINUX_X86_64_SYS_MUNMAP, .name = "munmap", .class_id = BH_SYS_CLASS_MEMORY, .arg_count = 2, .handler = linux_sys_munmap },
    [LINUX_X86_64_SYS_BRK]              = { .nr = LINUX_X86_64_SYS_BRK, .name = "brk", .class_id = BH_SYS_CLASS_MEMORY, .arg_count = 1, .handler = linux_sys_brk },
    [LINUX_X86_64_SYS_RT_SIGACTION]     = { .nr = LINUX_X86_64_SYS_RT_SIGACTION, .name = "rt_sigaction", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 4, .handler = linux_sys_rt_sigaction },
    [LINUX_X86_64_SYS_RT_SIGPROCMASK]   = { .nr = LINUX_X86_64_SYS_RT_SIGPROCMASK, .name = "rt_sigprocmask", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 4, .handler = linux_sys_rt_sigprocmask },
    [LINUX_X86_64_SYS_RT_SIGRETURN]     = { .nr = LINUX_X86_64_SYS_RT_SIGRETURN, .name = "rt_sigreturn", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 0, .handler = linux_sys_rt_sigreturn },
    [LINUX_X86_64_SYS_IOCTL]            = { .nr = LINUX_X86_64_SYS_IOCTL, .name = "ioctl", .class_id = BH_SYS_CLASS_IO, .arg_count = 3, .flags = BH_SYSCALL_F_BLOCKING, .handler = linux_sys_ioctl },
    [LINUX_X86_64_SYS_ACCESS]           = { .nr = LINUX_X86_64_SYS_ACCESS, .name = "access", .class_id = BH_SYS_CLASS_IO, .arg_count = 2, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ, .handler = linux_sys_access },
    [LINUX_X86_64_SYS_PIPE]             = { .nr = LINUX_X86_64_SYS_PIPE, .name = "pipe", .class_id = BH_SYS_CLASS_IO, .arg_count = 1, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_WRITE, .handler = linux_sys_pipe },
    [LINUX_X86_64_SYS_DUP]              = { .nr = LINUX_X86_64_SYS_DUP, .name = "dup", .class_id = BH_SYS_CLASS_IO, .arg_count = 1, .flags = BH_SYSCALL_F_FAST, .handler = linux_sys_dup },
    [LINUX_X86_64_SYS_DUP2]             = { .nr = LINUX_X86_64_SYS_DUP2, .name = "dup2", .class_id = BH_SYS_CLASS_IO, .arg_count = 2, .flags = BH_SYSCALL_F_FAST, .handler = linux_sys_dup2 },
    [LINUX_X86_64_SYS_GETPID]           = { .nr = LINUX_X86_64_SYS_GETPID, .name = "getpid", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 0, .flags = BH_SYSCALL_F_FAST, .handler = linux_sys_getpid },
    [LINUX_X86_64_SYS_SOCKETPAIR]       = { .nr = LINUX_X86_64_SYS_SOCKETPAIR, .name = "socketpair", .class_id = BH_SYS_CLASS_IO, .arg_count = 4, .flags = BH_SYSCALL_F_BLOCKING, .handler = linux_sys_socketpair },
    [LINUX_X86_64_SYS_CLONE]            = { .nr = LINUX_X86_64_SYS_CLONE, .name = "clone", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 5, .flags = BH_SYSCALL_F_BLOCKING, .handler = linux_sys_clone },
    [LINUX_X86_64_SYS_FORK]             = { .nr = LINUX_X86_64_SYS_FORK, .name = "fork", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 0, .flags = BH_SYSCALL_F_BLOCKING, .handler = linux_sys_fork },
    [LINUX_X86_64_SYS_VFORK]            = { .nr = LINUX_X86_64_SYS_VFORK, .name = "vfork", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 0, .flags = BH_SYSCALL_F_BLOCKING, .handler = linux_sys_vfork },
    [LINUX_X86_64_SYS_EXECVE]           = { .nr = LINUX_X86_64_SYS_EXECVE, .name = "execve", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 3, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ, .handler = linux_sys_execve },
    [LINUX_X86_64_SYS_EXIT]             = { .nr = LINUX_X86_64_SYS_EXIT, .name = "exit", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 1, .handler = bh_sys_thread_exit },
    [LINUX_X86_64_SYS_WAIT4]            = { .nr = LINUX_X86_64_SYS_WAIT4, .name = "wait4", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 4, .flags = BH_SYSCALL_F_BLOCKING, .handler = linux_sys_wait4 },
    [LINUX_X86_64_SYS_UNAME]            = { .nr = LINUX_X86_64_SYS_UNAME, .name = "uname", .class_id = BH_SYS_CLASS_SYSTEM, .arg_count = 1, .handler = linux_sys_uname },
    [LINUX_X86_64_SYS_FCNTL]            = { .nr = LINUX_X86_64_SYS_FCNTL, .name = "fcntl", .class_id = BH_SYS_CLASS_IO, .arg_count = 3, .flags = BH_SYSCALL_F_FAST, .handler = linux_sys_fcntl },
    [LINUX_X86_64_SYS_GETCWD]           = { .nr = LINUX_X86_64_SYS_GETCWD, .name = "getcwd", .class_id = BH_SYS_CLASS_IO, .arg_count = 2, .flags = BH_SYSCALL_F_USER_WRITE, .handler = linux_sys_getcwd },
    [LINUX_X86_64_SYS_CHDIR]            = { .nr = LINUX_X86_64_SYS_CHDIR, .name = "chdir", .class_id = BH_SYS_CLASS_IO, .arg_count = 1, .flags = BH_SYSCALL_F_USER_READ, .handler = linux_sys_chdir },
    [LINUX_X86_64_SYS_READLINK]         = { .nr = LINUX_X86_64_SYS_READLINK, .name = "readlink", .class_id = BH_SYS_CLASS_IO, .arg_count = 3, .flags = BH_SYSCALL_F_USER_READ | BH_SYSCALL_F_USER_WRITE, .handler = linux_sys_readlink },
    [LINUX_X86_64_SYS_ARCH_PRCTL]       = { .nr = LINUX_X86_64_SYS_ARCH_PRCTL, .name = "arch_prctl", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 2, .handler = linux_sys_arch_prctl },
    [LINUX_X86_64_SYS_GETTID]           = { .nr = LINUX_X86_64_SYS_GETTID, .name = "gettid", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 0, .flags = BH_SYSCALL_F_FAST, .handler = linux_sys_gettid },
    [LINUX_X86_64_SYS_FUTEX]            = { .nr = LINUX_X86_64_SYS_FUTEX, .name = "futex", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 6, .flags = BH_SYSCALL_F_BLOCKING, .handler = linux_sys_futex },
    [LINUX_X86_64_SYS_EPOLL_CREATE]     = { .nr = LINUX_X86_64_SYS_EPOLL_CREATE, .name = "epoll_create", .class_id = BH_SYS_CLASS_IO, .arg_count = 1, .flags = BH_SYSCALL_F_FAST, .handler = linux_sys_epoll_create },
    [LINUX_X86_64_SYS_SET_TID_ADDRESS]  = { .nr = LINUX_X86_64_SYS_SET_TID_ADDRESS, .name = "set_tid_address", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 1, .handler = linux_sys_set_tid_address },
    [LINUX_X86_64_SYS_CLOCK_GETTIME]    = { .nr = LINUX_X86_64_SYS_CLOCK_GETTIME, .name = "clock_gettime", .class_id = BH_SYS_CLASS_SYSTEM, .arg_count = 2, .flags = BH_SYSCALL_F_USER_WRITE, .handler = linux_sys_clock_gettime },
    [LINUX_X86_64_SYS_EXIT_GROUP]       = { .nr = LINUX_X86_64_SYS_EXIT_GROUP, .name = "exit_group", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 1, .handler = bh_sys_thread_exit },
    [LINUX_X86_64_SYS_EPOLL_WAIT]       = { .nr = LINUX_X86_64_SYS_EPOLL_WAIT, .name = "epoll_wait", .class_id = BH_SYS_CLASS_IO, .arg_count = 4, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_WRITE, .handler = linux_sys_epoll_wait },
    [LINUX_X86_64_SYS_EPOLL_CTL]        = { .nr = LINUX_X86_64_SYS_EPOLL_CTL, .name = "epoll_ctl", .class_id = BH_SYS_CLASS_IO, .arg_count = 4, .flags = BH_SYSCALL_F_FAST | BH_SYSCALL_F_USER_READ, .handler = linux_sys_epoll_ctl },
    [LINUX_X86_64_SYS_OPENAT]           = { .nr = LINUX_X86_64_SYS_OPENAT, .name = "openat", .class_id = BH_SYS_CLASS_IO, .arg_count = 4, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ, .handler = linux_sys_openat },
    [LINUX_X86_64_SYS_NEWFSTATAT]       = { .nr = LINUX_X86_64_SYS_NEWFSTATAT, .name = "newfstatat", .class_id = BH_SYS_CLASS_IO, .arg_count = 4, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ | BH_SYSCALL_F_USER_WRITE, .handler = linux_sys_newfstatat },
    [LINUX_X86_64_SYS_READLINKAT]       = { .nr = LINUX_X86_64_SYS_READLINKAT, .name = "readlinkat", .class_id = BH_SYS_CLASS_IO, .arg_count = 4, .flags = BH_SYSCALL_F_USER_READ | BH_SYSCALL_F_USER_WRITE, .handler = linux_sys_readlinkat },
    [LINUX_X86_64_SYS_FACCESSAT]        = { .nr = LINUX_X86_64_SYS_FACCESSAT, .name = "faccessat", .class_id = BH_SYS_CLASS_IO, .arg_count = 4, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ, .handler = linux_sys_faccessat },
    [LINUX_X86_64_SYS_PPOLL]            = { .nr = LINUX_X86_64_SYS_PPOLL, .name = "ppoll", .class_id = BH_SYS_CLASS_IO, .arg_count = 5, .flags = BH_SYSCALL_F_BLOCKING, .handler = linux_sys_ppoll },
    [LINUX_X86_64_SYS_EPOLL_PWAIT]      = { .nr = LINUX_X86_64_SYS_EPOLL_PWAIT, .name = "epoll_pwait", .class_id = BH_SYS_CLASS_IO, .arg_count = 5, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_WRITE, .handler = linux_sys_epoll_pwait },
    [LINUX_X86_64_SYS_TIMERFD_CREATE]   = { .nr = LINUX_X86_64_SYS_TIMERFD_CREATE, .name = "timerfd_create", .class_id = BH_SYS_CLASS_IO, .arg_count = 2, .flags = BH_SYSCALL_F_FAST, .handler = linux_sys_timerfd_create },
    [LINUX_X86_64_SYS_EVENTFD]          = { .nr = LINUX_X86_64_SYS_EVENTFD, .name = "eventfd", .class_id = BH_SYS_CLASS_IO, .arg_count = 1, .flags = BH_SYSCALL_F_FAST, .handler = linux_sys_eventfd },
    [LINUX_X86_64_SYS_TIMERFD_SETTIME]  = { .nr = LINUX_X86_64_SYS_TIMERFD_SETTIME, .name = "timerfd_settime", .class_id = BH_SYS_CLASS_IO, .arg_count = 4, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ | BH_SYSCALL_F_USER_WRITE, .handler = linux_sys_timerfd_settime },
    [LINUX_X86_64_SYS_TIMERFD_GETTIME]  = { .nr = LINUX_X86_64_SYS_TIMERFD_GETTIME, .name = "timerfd_gettime", .class_id = BH_SYS_CLASS_IO, .arg_count = 2, .flags = BH_SYSCALL_F_FAST | BH_SYSCALL_F_USER_WRITE, .handler = linux_sys_timerfd_gettime },
    [LINUX_X86_64_SYS_EVENTFD2]         = { .nr = LINUX_X86_64_SYS_EVENTFD2, .name = "eventfd2", .class_id = BH_SYS_CLASS_IO, .arg_count = 2, .flags = BH_SYSCALL_F_FAST, .handler = linux_sys_eventfd2 },
    [LINUX_X86_64_SYS_EPOLL_CREATE1]    = { .nr = LINUX_X86_64_SYS_EPOLL_CREATE1, .name = "epoll_create1", .class_id = BH_SYS_CLASS_IO, .arg_count = 1, .flags = BH_SYSCALL_F_FAST, .handler = linux_sys_epoll_create1 },
    [LINUX_X86_64_SYS_DUP3]             = { .nr = LINUX_X86_64_SYS_DUP3, .name = "dup3", .class_id = BH_SYS_CLASS_IO, .arg_count = 3, .flags = BH_SYSCALL_F_FAST, .handler = linux_sys_dup3 },
    [LINUX_X86_64_SYS_PIPE2]            = { .nr = LINUX_X86_64_SYS_PIPE2, .name = "pipe2", .class_id = BH_SYS_CLASS_IO, .arg_count = 2, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_WRITE, .handler = linux_sys_pipe2 },
    [LINUX_X86_64_SYS_PRLIMIT64]        = { .nr = LINUX_X86_64_SYS_PRLIMIT64, .name = "prlimit64", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 4, .handler = linux_sys_prlimit64 },
    [LINUX_X86_64_SYS_GETRANDOM]        = { .nr = LINUX_X86_64_SYS_GETRANDOM, .name = "getrandom", .class_id = BH_SYS_CLASS_SYSTEM, .arg_count = 3, .flags = BH_SYSCALL_F_USER_WRITE, .handler = linux_sys_getrandom },
};

static const bh_syscall_meta_t linux_syscall_table_generic[350] = {
    [LINUX_GENERIC_SYS_EVENTFD2]        = { .nr = LINUX_GENERIC_SYS_EVENTFD2, .name = "eventfd2", .class_id = BH_SYS_CLASS_IO, .arg_count = 2, .flags = BH_SYSCALL_F_FAST, .handler = linux_sys_eventfd2 },
    [LINUX_GENERIC_SYS_EPOLL_CREATE1]   = { .nr = LINUX_GENERIC_SYS_EPOLL_CREATE1, .name = "epoll_create1", .class_id = BH_SYS_CLASS_IO, .arg_count = 1, .flags = BH_SYSCALL_F_FAST, .handler = linux_sys_epoll_create1 },
    [LINUX_GENERIC_SYS_EPOLL_CTL]       = { .nr = LINUX_GENERIC_SYS_EPOLL_CTL, .name = "epoll_ctl", .class_id = BH_SYS_CLASS_IO, .arg_count = 4, .flags = BH_SYSCALL_F_FAST | BH_SYSCALL_F_USER_READ, .handler = linux_sys_epoll_ctl },
    [LINUX_GENERIC_SYS_EPOLL_PWAIT]     = { .nr = LINUX_GENERIC_SYS_EPOLL_PWAIT, .name = "epoll_pwait", .class_id = BH_SYS_CLASS_IO, .arg_count = 5, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_WRITE, .handler = linux_sys_epoll_pwait },
    [LINUX_GENERIC_SYS_DUP]             = { .nr = LINUX_GENERIC_SYS_DUP, .name = "dup", .class_id = BH_SYS_CLASS_IO, .arg_count = 1, .flags = BH_SYSCALL_F_FAST, .handler = linux_sys_dup },
    [LINUX_GENERIC_SYS_DUP3]            = { .nr = LINUX_GENERIC_SYS_DUP3, .name = "dup3", .class_id = BH_SYS_CLASS_IO, .arg_count = 3, .flags = BH_SYSCALL_F_FAST, .handler = linux_sys_dup3 },
    [LINUX_GENERIC_SYS_FCNTL]           = { .nr = LINUX_GENERIC_SYS_FCNTL, .name = "fcntl", .class_id = BH_SYS_CLASS_IO, .arg_count = 3, .flags = BH_SYSCALL_F_FAST, .handler = linux_sys_fcntl },
    [LINUX_GENERIC_SYS_IOCTL]           = { .nr = LINUX_GENERIC_SYS_IOCTL, .name = "ioctl", .class_id = BH_SYS_CLASS_IO, .arg_count = 3, .flags = BH_SYSCALL_F_BLOCKING, .handler = linux_sys_ioctl },
    [LINUX_GENERIC_SYS_FACCESSAT]       = { .nr = LINUX_GENERIC_SYS_FACCESSAT, .name = "faccessat", .class_id = BH_SYS_CLASS_IO, .arg_count = 4, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ, .handler = linux_sys_faccessat },
    [LINUX_GENERIC_SYS_CHDIR]           = { .nr = LINUX_GENERIC_SYS_CHDIR, .name = "chdir", .class_id = BH_SYS_CLASS_IO, .arg_count = 1, .flags = BH_SYSCALL_F_USER_READ, .handler = linux_sys_chdir },
    [LINUX_GENERIC_SYS_OPENAT]          = { .nr = LINUX_GENERIC_SYS_OPENAT, .name = "openat", .class_id = BH_SYS_CLASS_IO, .arg_count = 4, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ, .handler = linux_sys_openat },
    [LINUX_GENERIC_SYS_CLOSE]           = { .nr = LINUX_GENERIC_SYS_CLOSE, .name = "close", .class_id = BH_SYS_CLASS_IO, .arg_count = 1, .flags = BH_SYSCALL_F_BLOCKING, .handler = linux_sys_close },
    [LINUX_GENERIC_SYS_PIPE2]           = { .nr = LINUX_GENERIC_SYS_PIPE2, .name = "pipe2", .class_id = BH_SYS_CLASS_IO, .arg_count = 2, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_WRITE, .handler = linux_sys_pipe2 },
    [LINUX_GENERIC_SYS_READ]            = { .nr = LINUX_GENERIC_SYS_READ, .name = "read", .class_id = BH_SYS_CLASS_IO, .arg_count = 3, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_WRITE, .handler = linux_sys_read },
    [LINUX_GENERIC_SYS_WRITE]           = { .nr = LINUX_GENERIC_SYS_WRITE, .name = "write", .class_id = BH_SYS_CLASS_IO, .arg_count = 3, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ, .handler = linux_sys_write },
    [LINUX_GENERIC_SYS_PPOLL]           = { .nr = LINUX_GENERIC_SYS_PPOLL, .name = "ppoll", .class_id = BH_SYS_CLASS_IO, .arg_count = 5, .flags = BH_SYSCALL_F_BLOCKING, .handler = linux_sys_ppoll },
    [LINUX_GENERIC_SYS_READLINKAT]      = { .nr = LINUX_GENERIC_SYS_READLINKAT, .name = "readlinkat", .class_id = BH_SYS_CLASS_IO, .arg_count = 4, .flags = BH_SYSCALL_F_USER_READ | BH_SYSCALL_F_USER_WRITE, .handler = linux_sys_readlinkat },
    [LINUX_GENERIC_SYS_NEWFSTATAT]      = { .nr = LINUX_GENERIC_SYS_NEWFSTATAT, .name = "newfstatat", .class_id = BH_SYS_CLASS_IO, .arg_count = 4, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ | BH_SYSCALL_F_USER_WRITE, .handler = linux_sys_newfstatat },
    [LINUX_GENERIC_SYS_FSTAT]           = { .nr = LINUX_GENERIC_SYS_FSTAT, .name = "fstat", .class_id = BH_SYS_CLASS_IO, .arg_count = 2, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_WRITE, .handler = linux_sys_fstat },
    [LINUX_GENERIC_SYS_TIMERFD_CREATE]  = { .nr = LINUX_GENERIC_SYS_TIMERFD_CREATE, .name = "timerfd_create", .class_id = BH_SYS_CLASS_IO, .arg_count = 2, .flags = BH_SYSCALL_F_FAST, .handler = linux_sys_timerfd_create },
    [LINUX_GENERIC_SYS_TIMERFD_SETTIME] = { .nr = LINUX_GENERIC_SYS_TIMERFD_SETTIME, .name = "timerfd_settime", .class_id = BH_SYS_CLASS_IO, .arg_count = 4, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ | BH_SYSCALL_F_USER_WRITE, .handler = linux_sys_timerfd_settime },
    [LINUX_GENERIC_SYS_TIMERFD_GETTIME] = { .nr = LINUX_GENERIC_SYS_TIMERFD_GETTIME, .name = "timerfd_gettime", .class_id = BH_SYS_CLASS_IO, .arg_count = 2, .flags = BH_SYSCALL_F_FAST | BH_SYSCALL_F_USER_WRITE, .handler = linux_sys_timerfd_gettime },
    [LINUX_GENERIC_SYS_EXIT]            = { .nr = LINUX_GENERIC_SYS_EXIT, .name = "exit", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 1, .handler = bh_sys_thread_exit },
    [LINUX_GENERIC_SYS_EXIT_GROUP]      = { .nr = LINUX_GENERIC_SYS_EXIT_GROUP, .name = "exit_group", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 1, .handler = bh_sys_thread_exit },
    [LINUX_GENERIC_SYS_SET_TID_ADDRESS] = { .nr = LINUX_GENERIC_SYS_SET_TID_ADDRESS, .name = "set_tid_address", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 1, .handler = linux_sys_set_tid_address },
    [LINUX_GENERIC_SYS_FUTEX]           = { .nr = LINUX_GENERIC_SYS_FUTEX, .name = "futex", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 6, .flags = BH_SYSCALL_F_BLOCKING, .handler = linux_sys_futex },
    [LINUX_GENERIC_SYS_CLOCK_GETTIME]   = { .nr = LINUX_GENERIC_SYS_CLOCK_GETTIME, .name = "clock_gettime", .class_id = BH_SYS_CLASS_SYSTEM, .arg_count = 2, .flags = BH_SYSCALL_F_USER_WRITE, .handler = linux_sys_clock_gettime },
    [LINUX_GENERIC_SYS_RT_SIGACTION]    = { .nr = LINUX_GENERIC_SYS_RT_SIGACTION, .name = "rt_sigaction", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 4, .handler = linux_sys_rt_sigaction },
    [LINUX_GENERIC_SYS_RT_SIGPROCMASK]  = { .nr = LINUX_GENERIC_SYS_RT_SIGPROCMASK, .name = "rt_sigprocmask", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 4, .handler = linux_sys_rt_sigprocmask },
    [LINUX_GENERIC_SYS_RT_SIGRETURN]    = { .nr = LINUX_GENERIC_SYS_RT_SIGRETURN, .name = "rt_sigreturn", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 0, .handler = linux_sys_rt_sigreturn },
    [LINUX_GENERIC_SYS_UNAME]           = { .nr = LINUX_GENERIC_SYS_UNAME, .name = "uname", .class_id = BH_SYS_CLASS_SYSTEM, .arg_count = 1, .handler = linux_sys_uname },
    [LINUX_GENERIC_SYS_GETPID]          = { .nr = LINUX_GENERIC_SYS_GETPID, .name = "getpid", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 0, .flags = BH_SYSCALL_F_FAST, .handler = linux_sys_getpid },
    [LINUX_GENERIC_SYS_GETTID]          = { .nr = LINUX_GENERIC_SYS_GETTID, .name = "gettid", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 0, .flags = BH_SYSCALL_F_FAST, .handler = linux_sys_gettid },
    [LINUX_GENERIC_SYS_SOCKETPAIR]      = { .nr = LINUX_GENERIC_SYS_SOCKETPAIR, .name = "socketpair", .class_id = BH_SYS_CLASS_IO, .arg_count = 4, .flags = BH_SYSCALL_F_BLOCKING, .handler = linux_sys_socketpair },
    [LINUX_GENERIC_SYS_BRK]             = { .nr = LINUX_GENERIC_SYS_BRK, .name = "brk", .class_id = BH_SYS_CLASS_MEMORY, .arg_count = 1, .handler = linux_sys_brk },
    [LINUX_GENERIC_SYS_MUNMAP]          = { .nr = LINUX_GENERIC_SYS_MUNMAP, .name = "munmap", .class_id = BH_SYS_CLASS_MEMORY, .arg_count = 2, .handler = linux_sys_munmap },
    [LINUX_GENERIC_SYS_CLONE]           = { .nr = LINUX_GENERIC_SYS_CLONE, .name = "clone", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 5, .flags = BH_SYSCALL_F_BLOCKING, .handler = linux_sys_clone },
    [LINUX_GENERIC_SYS_EXECVE]          = { .nr = LINUX_GENERIC_SYS_EXECVE, .name = "execve", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 3, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ, .handler = linux_sys_execve },
    [LINUX_GENERIC_SYS_MMAP]            = { .nr = LINUX_GENERIC_SYS_MMAP, .name = "mmap", .class_id = BH_SYS_CLASS_MEMORY, .arg_count = 6, .handler = linux_sys_mmap },
    [LINUX_GENERIC_SYS_MPROTECT]        = { .nr = LINUX_GENERIC_SYS_MPROTECT, .name = "mprotect", .class_id = BH_SYS_CLASS_MEMORY, .arg_count = 3, .handler = linux_sys_mprotect },
    [LINUX_GENERIC_SYS_WAIT4]           = { .nr = LINUX_GENERIC_SYS_WAIT4, .name = "wait4", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 4, .flags = BH_SYSCALL_F_BLOCKING, .handler = linux_sys_wait4 },
    [LINUX_GENERIC_SYS_PRLIMIT64]       = { .nr = LINUX_GENERIC_SYS_PRLIMIT64, .name = "prlimit64", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 4, .handler = linux_sys_prlimit64 },
    [LINUX_GENERIC_SYS_GETRANDOM]       = { .nr = LINUX_GENERIC_SYS_GETRANDOM, .name = "getrandom", .class_id = BH_SYS_CLASS_SYSTEM, .arg_count = 3, .flags = BH_SYSCALL_F_USER_WRITE, .handler = linux_sys_getrandom },
};

#if defined(BHARAT_ARCH_ARM) || defined(BHARAT_ARCH_RISCV) || defined(__aarch64__) || defined(__riscv)
const bh_personality_syscall_table_t bh_linux_syscall_table = {
    .name = "linux-generic",
    .abi_version = 1,
    .entry_count = sizeof(linux_syscall_table_generic) / sizeof(linux_syscall_table_generic[0]),
    .table = linux_syscall_table_generic
};
#else
const bh_personality_syscall_table_t bh_linux_syscall_table = {
    .name = "linux-x86_64",
    .abi_version = 1,
    .entry_count = sizeof(linux_syscall_table_x86_64) / sizeof(linux_syscall_table_x86_64[0]),
    .table = linux_syscall_table_x86_64
};
#endif
