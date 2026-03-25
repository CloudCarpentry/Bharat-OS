#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "boot/boot_info.h"
#include "../kernel/src/kernel_boot.h"
#include "../kernel/include/secure_boot.h"
#include "../kernel/include/boot/boot_mode.h"
#include <stdlib.h>

// Stubs for kernel_boot calls
int bharat_secure_boot_verify_early(void) { return 0; }
int bharat_audit_init(void) { return 0; }
int bharat_credentials_init(void) { return 0; }
int bharat_isolation_init(void) { return 0; }
int bharat_policy_init(int mode) { return 0; }
int bharat_secure_boot_stage_hook(bharat_boot_stage_t stage, uint64_t magic) { return 0; }

bool boot_validate_all(boot_info_t* bi) { return true; }
int boot_mode_resolve(const struct boot_info *bi, bharat_boot_mode_t *out_mode) {
    if (out_mode) *out_mode = BHARAT_BOOT_MODE_NORMAL;
    return 0;
}

// HAL Stubs
void hal_init(void) {}
void hal_discovery_init(const boot_info_t *boot) {}
void profile_init(void) {}
void bharat_subsystems_init(const char *profile) {}

// String stubs for arch independent tests
void* arch_memcpy(void* dest, const void* src, size_t n, uint32_t flags) {
    (void)flags;
    char* d = (char*)dest;
    const char* s = (const char*)src;
    while(n--) *d++ = *s++;
    return dest;
}
void* arch_memset(void* s, int c, size_t n, uint32_t flags) {
    (void)flags;
    char* p = (char*)s;
    while(n--) *p++ = (char)c;
    return s;
}
void* arch_memmove(void* dest, const void* src, size_t n, uint32_t flags) {
    (void)flags;
    char* d = (char*)dest;
    const char* s = (const char*)src;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--) *--d = *--s;
    }
    return dest;
}

// Console stubs
void console_write_raw(const char *data, size_t len) {
    // Optional: write to stdout for debug, but silence usually preferable for clean tests
    // printf("%.*s", (int)len, data);
}

int boot_selftest_run_stage(bharat_boot_stage_t stage) {
    (void)stage;
    return 0;
}

bharat_boot_mode_t bharat_boot_mode_select(void) {
    return BHARAT_BOOT_MODE_NORMAL;
}

const char* bharat_boot_mode_name(bharat_boot_mode_t mode) {
    (void)mode;
    return "NORMAL";
}

size_t string_length(const char* str) {
    size_t len = 0;
    while (str && str[len]) len++;
    return len;
}

// PMM/VMM Stubs for test
int mock_pmm_init_called = 0;
int mm_pmm_init(uint32_t magic, const boot_info_t *boot) {
    mock_pmm_init_called++;
    if (magic != 0xB4A2A705) return -1;
    return 0;
}

int mock_vmm_init_called = 0;
int vmm_init(void) {
    mock_vmm_init_called++;
    return 0;
}

int mock_zswap_init_called = 0;
int zswap_init(void) {
    mock_zswap_init_called++;
    return 0;
}

__attribute__((weak)) void arch_mmu_init(void) {}
void hal_mmu_final_setup(void) {}

// Lots of stubs for the runtime features being called in boot_common_platform_services
int ptp_init(void) { return 0; }
int mk_boot_secondary_cores(uint32_t c) { return 0; }
int mk_init_per_core_channels(uint32_t c, uint32_t q) { return 0; }
void hal_irq_init_boot(void) {}
void hal_timer_init(void) {}
int device_register_builtin_drivers(void) { return 0; }
void arch_cpu_caps_init(void) {}
void arch_cpu_caps_system_finalize(void) {}
void hal_discovery_publish_cpu_caps(void) {}
void arch_ext_state_boot_init(void) {}
void ipc_async_init(void) {}
int trap_init(void) { return 0; }
int mk_establish_channel(uint32_t c, void* ch) { return 0; }
void hal_cpu_enable_interrupts(void) {}
void kernel_run_boot_tests(void) {}
void hello_world_app(void) {}
void kernel_tester_app(void) {}
void bharat_demo_app_legacy(void) {}
void bharat_demo_app(void) {}
void hal_cpu_halt(void) { while(1); }
int boot_video_map(const boot_info_t *boot) { return 0; }
int boot_gui_run(void) { return 0; }

// Boot policy stub
static bharat_boot_policy_t g_mock_policy = { .enable_zswap = 1, .enable_ai_governor = 0, .smp_target_cores = 1 };

const bharat_boot_policy_t* bharat_boot_active_policy(void) {
    return &g_mock_policy;
}

void kernel_panic(const char *msg) {
    printf("PANIC CALLED: %s\n", msg);
    assert(0); // Fail the test
}

static void test_boot_early(void) {
    printf("[TEST] Boot Early initialization\n");
    boot_info_t boot = {0};
    boot.magic = 0xB4A2A705;

    // Shouldn't panic, should just print banner and init subsystems
    boot_common_early(&boot);
    printf("[TEST] Boot Early Passed.\n");
}

static void test_boot_security(void) {
    printf("[TEST] Boot Security initialization\n");
    boot_info_t boot = {0};
    boot_common_security(&boot);
    printf("[TEST] Boot Security Passed.\n");
}

static void test_boot_memory(void) {
    printf("[TEST] Boot Memory initialization\n");
    boot_info_t boot = {0};
    boot.magic = 0xB4A2A705;
    boot.mem_region_count = 1;
    boot.mem_regions[0].phys_start = 0x100000;
    boot.mem_regions[0].size = 0x10000000;
    boot.mem_regions[0].type = BOOT_MEM_USABLE;

    mock_pmm_init_called = 0;
    mock_vmm_init_called = 0;
    mock_zswap_init_called = 0;

    boot_common_memory(&boot);

    assert(mock_pmm_init_called == 1);
    assert(mock_vmm_init_called == 1);
    assert(mock_zswap_init_called == 1);

    printf("[TEST] Boot Memory Passed.\n");
}

int main(void) {
    printf("Running End-to-End Boot Initialization Tests...\n");
    test_boot_early();
    test_boot_security();
    test_boot_memory();
    printf("All E2E Boot Tests passed!\n");
    return 0;
}
void test_device_dma_dump(void){}
