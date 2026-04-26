#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "trap/syscall_context.h"
#include "kernel/status.h"
#include "bh_personality.h"
#include "bh_personality_registry.h"
#include "personality_ops.h"
#include "bh_process_personality.h"
#include "sched/sched.h"

// Mocks
uint32_t hal_cpu_get_id(void) { return 0; }
void bh_syscall_stats_inc_total(uint32_t core_id) { (void)core_id; }
void bh_syscall_stats_inc_fast(uint32_t core_id) { (void)core_id; }
void bh_syscall_stats_inc_slow(uint32_t core_id) { (void)core_id; }
void bh_syscall_stats_inc_denied(uint32_t core_id) { (void)core_id; }
void fault_diag_record_syscall(uintptr_t nr) { (void)nr; }
long kstatus_to_sysret(kstatus_t st) { return (long)st; }
kstatus_t arch_trap_extract_syscall(const trap_frame_t *frame, bh_syscall_regs_t *out) {
    out->nr = 100; // Mock NR
    return K_OK;
}

// Global state for test
static bh_thread_t mock_thread;
static bh_process_t mock_process;

bh_thread_t* sched_current_thread(void) { return &mock_thread; }

// Dummy handlers
static long mock_native_handler(bh_syscall_ctx_t *ctx) { return 123; }
static long mock_linux_handler(bh_syscall_ctx_t *ctx) { return 456; }

static bh_syscall_desc_t native_table[] = { [100] = {100, "test", 0, 0, 0, mock_native_handler} };
static bh_syscall_desc_t linux_table[] = { [100] = {100, "test", 0, 0, 0, mock_linux_handler} };

static bh_personality_syscall_table_t native_ptable = { .table = native_table, .max_syscall_nr = 100 };
static bh_personality_syscall_table_t linux_ptable = { .table = linux_table, .max_syscall_nr = 100 };

// External symbols expected by bh_syscall_gate
const bh_personality_syscall_table_t *personality_native_get_table(void) { return &native_ptable; }
const bh_personality_syscall_table_t *personality_linux_get_table(void) { return &linux_ptable; }
const bh_personality_syscall_table_t *personality_android_get_table(void) { return NULL; }
const bh_personality_syscall_table_t *personality_windows_get_table(void) { return NULL; }

// Profile/Policy Mocks
bool bh_profile_allows_personality(uint32_t personality) { return true; }
bool bh_profile_allows_blocking_syscall(void) { return true; }
bool bh_profile_has_trait(uint64_t trait) { return true; }

extern long bh_syscall_gate(trap_frame_t *frame, const trap_info_t *info);

void test_syscall_routing(void) {
    printf("Testing syscall routing by personality...\n");

    mock_thread.process = &mock_process;

    // Test Native
    mock_process.personality.kind = BH_PERSONALITY_NATIVE;
    long ret = bh_syscall_gate((trap_frame_t*)0x1, (const trap_info_t*)0x1);
    printf("Ret (native): %ld\n", ret);
    assert(ret == 123);

#ifdef BHARAT_ENABLE_COMPAT_LINUX
    // Test Linux
    mock_process.personality.kind = BH_PERSONALITY_LINUX;
    ret = bh_syscall_gate((trap_frame_t*)0x1, (const trap_info_t*)0x1);
    printf("Ret (linux): %ld\n", ret);
    assert(ret == 456);
#endif

    printf("Syscall routing tests passed.\n");
}

int main(void) {
    test_syscall_routing();
    return 0;
}
