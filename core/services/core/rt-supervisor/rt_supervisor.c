#include <bharat/uapi/bootstrap/root_launch.h>
#include <bharat/uapi/init/bootstrap.h>
#include <bharat/uapi/syscall_nr.h>
#include <bharat/uapi/syscall/bh_syscall.h>
#include <stddef.h>

static size_t rt_strlen(const char *s) {
    size_t len = 0;
    while (s && s[len]) len++;
    return len;
}

static void rt_log(const char *msg) {
    bharat_syscall(SYSCALL_WRITE, 1, (uintptr_t)msg, rt_strlen(msg), 0, 0, 0);
}

void _start(const bharat_user_startup_t *startup) {
    rt_log("RT_SUPERVISOR: ENTERED\n");

    // Perform validation of the startup contract
    if (!startup) {
        rt_log("RT_SUPERVISOR_ERROR: Startup struct is NULL\n");
        bharat_syscall(SYSCALL_THREAD_EXIT, 1, 0, 0, 0, 0, 0);
        while (1) {}
    }

    if (startup->abi_version != 1 || startup->struct_size != sizeof(*startup) ||
        (startup->flags & BH_USER_STARTUP_FLAG_ROOT_LAUNCH_EXTENSION) == 0) {
        rt_log("RT_SUPERVISOR_ERROR: Invalid ABI version or struct size\n");
        bharat_syscall(SYSCALL_THREAD_EXIT, 2, 0, 0, 0, 0, 0);
        while (1) {}
    }

    const bh_root_launch_info_t *launch =
        (const bh_root_launch_info_t *)((const uint8_t *)startup +
                                       startup->struct_size);
    if (launch->version != BH_ROOT_LAUNCH_ABI_VERSION ||
        launch->size != sizeof(*launch) ||
        launch->runtime_model != BH_USERSPACE_RUNTIME_STATIC) {
        rt_log("RT_SUPERVISOR_ERROR: Invalid root launch contract\n");
        bharat_syscall(SYSCALL_THREAD_EXIT, 3, 0, 0, 0, 0, 0);
        while (1) {}
    }

    rt_log("RT_RUNTIME: STABLE\n");

    // Keep running in unprivileged unmapped environment (yielding)
    while (1) {
        bharat_syscall(SYSCALL_SCHED_YIELD, 0, 0, 0, 0, 0, 0);
    }
}
