#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <arch/arch_caps.h>
#include <hal/hal_discovery.h>
#include <hal/hal_boot.h>
#include <hal/hal_timer.h>
#include <profile/profile.h>

static jmp_buf panic_escape;
static unsigned launches;
static bool fail_launch;
static uint64_t ticks;

arch_caps_t arch_get_caps(void) {
    return (arch_caps_t){.bits = ARCH_CAP_BIT(ARCH_CAP_SMP)};
}
void console_write_raw(const char *s, size_t n) { (void)s; (void)n; }
void _secondary_trampoline(void) {}
uint64_t hal_timer_read_counter(void) { ticks += 1000; return ticks; }
uint64_t hal_timer_read_freq(void) { return 1000; }
size_t string_length(const char *s) { return strlen(s); }
void arch_cpu_relax(void) {}
void bh_smp_set_cpu_state(uint32_t cpu_id, bh_cpu_boot_state_t state);
bh_cpu_boot_state_t bh_smp_get_cpu_state(uint32_t cpu_id);
void kernel_panic(const char *s) {
    assert(strstr(s, "required CPU count not online"));
    longjmp(panic_escape, 1);
}
int hal_boot_start_cpu(uint32_t id, uint64_t entry) {
    (void)entry;
    ++launches;
    if (fail_launch) return -1;
    bh_smp_set_cpu_state(id, BH_CPU_BOOT_ONLINE);
    return 0;
}

int main(void) {
    system_discovery_t *discovery = hal_get_system_discovery();
    bh_smp_boot_primary_init();
    discovery->topology.cpu_count = 0;
    if (setjmp(panic_escape) != 0) {
        assert(!"BSP-only topology must not launch phantom APs");
    }
    assert(bh_smp_start_secondary_cpus(2) == 0);
    assert(launches == 0 && bh_smp_get_online_core_count() == 1);

    bh_smp_boot_primary_init();
    discovery->topology.cpu_count = 2;
    assert(bh_smp_start_secondary_cpus(2) == 0);
    assert(launches == 1 && bh_smp_get_online_core_count() == 2);

    bh_smp_boot_primary_init();
    fail_launch = true;
    launches = 0;
    if (setjmp(panic_escape) == 0) {
        bh_smp_start_secondary_cpus(2);
        assert(!"A failed required AP must panic in RT mode");
    }
    assert(launches == 1 && bh_smp_get_cpu_state(1) == BH_CPU_BOOT_FAILED);
    puts("PASS: BSP fallback, real CPU inventory, strict AP failure preserved");
    return 0;
}
