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
#include "linux_errno.h"

// Mocks
uint32_t hal_cpu_get_id(void) { return 0; }
void bh_syscall_stats_inc_total(uint32_t core_id) { (void)core_id; }
void bh_syscall_stats_inc_fast(uint32_t core_id) { (void)core_id; }
void bh_syscall_stats_inc_slow(uint32_t core_id) { (void)core_id; }
void bh_syscall_stats_inc_denied(uint32_t core_id) { (void)core_id; }
void fault_diag_record_syscall(uintptr_t nr) { (void)nr; }
long kstatus_to_sysret(kstatus_t st) {
    if (st == K_ERR_UNSUPPORTED) return -38; // Mock -ENOSYS
    return (long)st;
}
kstatus_t arch_trap_extract_syscall(const trap_frame_t *frame, bh_syscall_regs_t *out) {
    out->nr = 999; // Unsupported syscall NR
    return K_OK;
}

// Global state for test
static bh_thread_t mock_thread;
static bh_process_t mock_process;

bh_thread_t* sched_current_thread(void) { return &mock_thread; }

// External symbols expected by bh_syscall_gate
const bh_personality_syscall_table_t *personality_native_get_table(void) { return NULL; }
const bh_personality_syscall_table_t *personality_linux_get_table(void) {
    static bh_personality_syscall_table_t empty_table = { .table = NULL, .max_syscall_nr = 0 };
    return &empty_table;
}
const bh_personality_syscall_table_t *personality_android_get_table(void) { return NULL; }
const bh_personality_syscall_table_t *personality_windows_get_table(void) { return NULL; }

// Profile/Policy Mocks
bool bh_profile_allows_personality(uint32_t personality) { return true; }
bool bh_profile_allows_blocking_syscall(void) { return true; }
bool bh_profile_has_trait(uint64_t trait) { return true; }

extern long bh_syscall_gate(trap_frame_t *frame, const trap_info_t *info);

void test_linux_unsupported(void) {
    printf("Testing unsupported Linux syscall returns -ENOSYS...\n");

    mock_thread.process = &mock_process;
    mock_process.personality.kind = BH_PERSONALITY_LINUX;

    long ret = bh_syscall_gate((trap_frame_t*)0x1, (const trap_info_t*)0x1);
    printf("Ret: %ld (expected -38)\n", ret);
    assert(ret == -38);

    printf("Unsupported Linux syscall test passed.\n");
}

int main(void) {
    test_linux_unsupported();
    return 0;
}
