#include <bharat/runtime/runtime.h>
#include <stdint.h>
#include <stddef.h>

#include <bharat/uapi/init/bootstrap.h>
#include <bharat/uapi/syscall_nr.h>
#include <bharat/uapi/syscall/bh_syscall.h>
#include <bharat/uapi/syscall_args.h>
#include <bharat/uapi/time/time.h>
#include <bharat/syscalls.h>


static bharat_handle_t g_bootstrap_cap = BHARAT_INVALID_HANDLE;
static const bharat_user_startup_t *g_startup_ptr = NULL;

void bharat_runtime_init(const void *startup_ptr) {
    /* The startup block is an ABI structure, so reject obviously malformed
     * pointers before reading capability-bearing fields from it. */
    uintptr_t startup_addr = (uintptr_t)startup_ptr;
    if (startup_addr < 4096U ||
        (startup_addr % _Alignof(bharat_user_startup_t)) != 0U) {
        startup_ptr = NULL;
    }
    // Initialize memory, TLS, thread structs
    g_startup_ptr = (const bharat_user_startup_t *)startup_ptr;
    const bharat_user_startup_t *startup = (const bharat_user_startup_t *)startup_ptr;
    if (startup) {
        g_bootstrap_cap = startup->bootstrap.bootstrap_cap;
    } else {
        g_bootstrap_cap = BHARAT_INVALID_HANDLE;
    }
}

void bharat_runtime_shutdown(void) {
    // Stub implementation: clean up resources, close handles
}

bharat_handle_t bharat_runtime_get_bootstrap_cap(void) {
    return g_bootstrap_cap;
}

const bharat_user_startup_t *bharat_runtime_get_startup(void) {
    return g_startup_ptr;
}

int bharat_bootstrap_probe(void) {
    bharat_sys_cap_invoke_args_t args = {.cap_id = g_bootstrap_cap,
        .opcode = BH_BOOTSTRAP_OP_PROBE};
    return bharat_syscall(BH_SYS_CAPABILITY_INVOKE, (uintptr_t)&args, 0, 0, 0, 0, 0);
}

int bharat_bootstrap_stop(uint32_t process_cap) {
    bharat_sys_cap_invoke_args_t args = {.cap_id = process_cap,
        .opcode = BH_PROCESS_OP_TERMINATE};
    int status = bharat_syscall(BH_SYS_CAPABILITY_INVOKE, (uintptr_t)&args, 0, 0, 0, 0, 0);
    if (status != 0) return status;
    /* Give the scheduler's deferred reaper an opportunity. Failed compensation
     * is reported to init, which retains authority and quarantines the child. */
    status = bharat_sched_yield();
    if (status != 0) return status;
    args.opcode = BH_PROCESS_OP_REAP;
    return bharat_syscall(BH_SYS_CAPABILITY_INVOKE, (uintptr_t)&args, 0, 0, 0, 0, 0);
}

int bharat_bootstrap_launch(const char *name, uint32_t service_id,
                           uint32_t namesvc_cap, uint32_t delegate_launch,
                           bh_bootstrap_launch_result_t *out) {
    bh_bootstrap_launch_request_t req = {.version = BH_BOOTSTRAP_SERVICE_ABI,
        .service_id = service_id, .namesvc_cap = namesvc_cap, .delegate_launch = delegate_launch};
    if (!name || !out) return -1;
    size_t i;
    for (i = 0; i < sizeof(req.module_name) - 1 && name[i]; ++i) req.module_name[i] = name[i];
    if (name[i]) return -1;
    bharat_sys_cap_invoke_args_t args = {.cap_id = g_bootstrap_cap,
        .opcode = BH_BOOTSTRAP_OP_LAUNCH, .arg0 = (uintptr_t)&req, .arg1 = (uintptr_t)out};
    return bharat_syscall(BH_SYS_CAPABILITY_INVOKE, (uintptr_t)&args, 0, 0, 0, 0, 0);
}

int bharat_bootstrap_report(uint32_t type, int32_t status) {
    if (!g_startup_ptr || !g_startup_ptr->bootstrap.system_control_endpoint) return -1;
    bh_bootstrap_service_event_t event = {.version = BH_BOOTSTRAP_SERVICE_ABI,
        .type = type, .service_id = (uint32_t)g_startup_ptr->bootstrap.flags, .status = status};
    bharat_sys_endpoint_send_args_t args = {
        .send_cap = g_startup_ptr->bootstrap.system_control_endpoint,
        .payload_len = sizeof(event), .payload_ptr = (uintptr_t)&event,
        .timeout_ticks = UINT64_MAX};
    return bharat_syscall(BH_SYS_ENDPOINT_SEND, (uintptr_t)&args, 0, 0, 0, 0, 0);
}

int bharat_bootstrap_poll(uint32_t receive_cap, bh_bootstrap_service_event_t *event) {
    uint32_t length = 0;
    bharat_sys_endpoint_receive_args_t args = {.recv_cap = receive_cap,
        .out_payload_capacity = sizeof(*event), .out_payload_ptr = (uintptr_t)event,
        .out_len_ptr = (uintptr_t)&length, .timeout_ticks = 0};
    int result = bharat_syscall(BH_SYS_ENDPOINT_RECEIVE, (uintptr_t)&args, 0, 0, 0, 0, 0);
    if (result == 0 && length != sizeof(*event)) return -1;
    return result;
}

int bharat_runtime_now_ns(uint64_t *out) {
    return bharat_syscall(BH_SYS_TIME_GET, BH_CLOCK_MONOTONIC, (uintptr_t)out, 0, 0, 0, 0);
}

static size_t runtime_strlen(const char *s) {
    size_t len = 0;
    while (s && s[len]) len++;
    return len;
}

void bharat_runtime_log(const char *msg) {
    bharat_syscall(SYSCALL_WRITE, 1, (uintptr_t)msg, runtime_strlen(msg), 0, 0, 0);
}

void bharat_runtime_panic(const char *reason) {
    if (reason) {
        bharat_runtime_log("PANIC: ");
        bharat_runtime_log(reason);
    }
    while(1) {} // Unreachable
}

int bharat_runtime_main_wrapper(int argc, char **argv, int (*main_fn)(int, char**)) {
    bharat_runtime_init(NULL);

    int result = -1;
    if (main_fn) {
        result = main_fn(argc, argv);
    }

    bharat_runtime_shutdown();
    return result;
}

// ── 64-bit Helper Functions for 32-bit Architectures ────────────────────────
// These are required when the compiler needs to perform 64-bit math or atomics
// on a 32-bit target without native support.

#if defined(BHARAT_ARCH_32BIT) || defined(__arm__) || (defined(__riscv) && __riscv_xlen == 32)

uint64_t __aeabi_uidivmod(unsigned int n, unsigned int d) {
    if (d == 0) return 0;
    unsigned int q = 0, r = 0;
    for (int i = 31; i >= 0; i--) {
        r <<= 1; r |= (n >> i) & 1;
        if (r >= d) { r -= d; q |= (1U << i); }
    }
    return ((uint64_t)r << 32) | q;
}

unsigned int __aeabi_uidiv(unsigned int n, unsigned int d) {
    return (unsigned int)__aeabi_uidivmod(n, d);
}

float __aeabi_fdiv(float n, float d) { (void)n; (void)d; return 1.0f; }

uint64_t __udivdi3(uint64_t n, uint64_t d) {
    if (d == 0) return 0;
    uint64_t q = 0, r = 0;
    for (int i = 63; i >= 0; i--) {
        r <<= 1; r |= (n >> i) & 1;
        if (r >= d) { r -= d; q |= (1ULL << i); }
    }
    return q;
}
uint64_t __umoddi3(uint64_t n, uint64_t d) {
    if (d == 0) return 0;
    uint64_t r = 0;
    for (int i = 63; i >= 0; i--) {
        r <<= 1; r |= (n >> i) & 1;
        if (r >= d) r -= d;
    }
    return r;
}
uint64_t __aeabi_uldivmod(uint64_t n, uint64_t d) { return __udivdi3(n, d); }

// 64-bit atomic fetch-and-add for 32-bit.
// WARNING: This implementation is NOT multicore safe. 
// It is intended only for early bootstrap or single-core systems.
uint64_t __atomic_fetch_add_8(volatile void *ptr, uint64_t val, int memorder) {
    (void)memorder;
    uint64_t *p = (uint64_t *)ptr;
    uint64_t old = *p;
    *p = old + val;
    return old;
}

#endif
