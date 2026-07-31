#include <bharat/uapi/init/rt_startup.h>
#include <bharat/uapi/syscall_nr.h>
#include <bharat/uapi/syscall/bh_syscall.h>

static size_t rt_strlen(const char *s) {
    size_t len = 0;
    while (s && s[len]) len++;
    return len;
}

static void rt_log(const char *msg) {
    bharat_syscall(SYSCALL_WRITE, 1, (uintptr_t)msg, rt_strlen(msg), 0, 0, 0);
}

void _start(const bh_rt_startup_t *startup) {
    rt_log("RT_SUPERVISOR: ENTERED\n");

    // Perform validation of the startup contract
    if (!startup) {
        rt_log("RT_SUPERVISOR_ERROR: Startup struct is NULL\n");
        bharat_syscall(SYSCALL_THREAD_EXIT, 1, 0, 0, 0, 0, 0);
        while (1) {}
    }

    if (startup->abi_version != 0x0100 || startup->struct_size != sizeof(bh_rt_startup_t)) {
        rt_log("RT_SUPERVISOR_ERROR: Invalid ABI version or struct size\n");
        bharat_syscall(SYSCALL_THREAD_EXIT, 2, 0, 0, 0, 0, 0);
        while (1) {}
    }

    // Verify timer/scheduler properties (can be simulated or actual check)
    if (startup->timer_frequency == 0) {
        rt_log("RT_SUPERVISOR_ERROR: Invalid timer frequency\n");
        bharat_syscall(SYSCALL_THREAD_EXIT, 3, 0, 0, 0, 0, 0);
        while (1) {}
    }

    rt_log("RT_RUNTIME: STABLE\n");

    // Keep running in unprivileged unmapped environment (yielding)
    while (1) {
        bharat_syscall(SYSCALL_THREAD_YIELD, 0, 0, 0, 0, 0, 0);
    }
}
