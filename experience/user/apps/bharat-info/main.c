#include <bharat/bh_native.h>
#include <bharat/uapi/syscall_nr.h>
#include <bharat/uapi/syscall/bh_syscall.h>
#include <stddef.h>
#include <stdint.h>

extern long bh_syscall(long sysno, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);

static size_t util_strlen(const char *text) {
    size_t length = 0;
    while (text[length] != '\0') {
        ++length;
    }
    return length;
}

static void util_print(const char *text) {
    (void)bh_syscall(SYSCALL_WRITE, 1, (uintptr_t)text, util_strlen(text), 0, 0, 0);
}

static void util_print_uint(uint64_t val) {
    char buf[32];
    int i = 30;
    buf[31] = '\0';
    if (val == 0) {
        buf[i--] = '0';
    } else {
        while (val > 0) {
            buf[i--] = '0' + (val % 10);
            val /= 10;
        }
    }
    util_print(&buf[i + 1]);
}

int main(void) {
    util_print("===================================\n");
    util_print("        Bharat-OS Diagnostics      \n");
    util_print("===================================\n\n");

    util_print("[VERSION] Kernel version: <Unsupported: Syscall/IPC not exposed>\n");
    util_print("[IDENTITY] Process/Thread ID: <Unsupported>\n");

    bh_time_t time_ns = 0;
    int res = bh_time_get(BH_CLOCK_MONOTONIC, &time_ns);
    if (res == 0) {
        util_print("[UPTIME] Monotonic Uptime (ns): ");
        util_print_uint((uint64_t)time_ns);
        util_print("\n");
    } else {
        util_print("[UPTIME] Failed to get monotonic time\n");
    }

    void* addr = NULL;
    size_t size = 4096;
    res = bh_alloc_ex(size, 0, 0, &addr);
    if (res == 0 && addr != NULL) {
        util_print("[MEMORY] Allocated 4KB memory at address: ");
        util_print_uint((uintptr_t)addr);
        util_print("\n");
        // Replaced unmap since it is not exposed in bh_native.c
        // Usually, in these smoke tests we just allocate. Unmap is unsupported in bh_native.c natively
        util_print("[MEMORY] Release unsupported natively through bh_native\n");
    } else {
        util_print("[MEMORY] Failed to allocate memory\n");
    }

    util_print("\nDiagnostics complete.\n");

    return 0;
}
