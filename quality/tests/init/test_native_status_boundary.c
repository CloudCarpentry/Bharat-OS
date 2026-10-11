#include <assert.h>
#include <stdio.h>
#include "trap/syscall_context.h"
#include "trap/syscall_status.h"
#include "sched/sched.h"
#include "personality_ops.h"
#include <bharat/uapi/syscall/bh_syscall_status.h>

static bh_process_t process;
static bh_thread_t thread;
static bh_operation_result_t result;
static bh_operation_result_t handle(bh_syscall_ctx_t *ctx) { (void)ctx; return result; }
static const bh_syscall_meta_t entries[] = {{.nr = 0, .name = "test",
    .cap_arg_index = BH_SYS_CAP_INDEX_NONE, .handler = handle}};
const bh_personality_syscall_table_t native_personality = {.table = entries, .entry_count = 1};
bh_thread_t *sched_current_thread(void) { return &thread; }
uint32_t hal_cpu_get_id(void) { return 0; }
void fault_diag_record_syscall(uintptr_t nr) { (void)nr; }
void bh_syscall_stats_inc_total(uint32_t core) { (void)core; }
void bh_syscall_stats_inc_fast(uint32_t core) { (void)core; }
void bh_syscall_stats_inc_slow(uint32_t core) { (void)core; }
void bh_syscall_stats_inc_denied(uint32_t core) { (void)core; }
bool bh_profile_allows_personality(uint32_t p) { (void)p; return true; }
bool bh_profile_allows_blocking_syscall(void) { return true; }
bool bh_profile_has_trait(uint64_t t) { (void)t; return true; }
const void *bh_personality_registry_get_ops(bh_personality_kind_t kind) { (void)kind; return NULL; }
bh_status_t bh_syscall_validate_capability(bh_syscall_ctx_t *ctx, uint32_t cap, uint32_t type, uint64_t rights) {
    (void)ctx; (void)cap; (void)type; (void)rights; return BH_ERR_BAD_CAPABILITY;
}
kstatus_t arch_trap_extract_syscall(const trap_frame_t *f, bh_syscall_regs_t *out) {
    (void)f; *out = (bh_syscall_regs_t){0}; return K_OK;
}
extern long bh_syscall_gate(trap_frame_t *, const trap_info_t *);
int main(void) {
    process.personality.kind = BH_PERSONALITY_NATIVE;
    thread.process = &process;
    trap_frame_t frame = {0}; trap_info_t info = {0};
    const struct { kstatus_t kernel; bh_status_t native; } cases[] = {
        {K_OK, BH_OK}, {K_ERR_AGAIN, BH_ERR_TRY_AGAIN},
        {K_ERR_TIMEOUT, BH_ERR_TIMEOUT}, {K_ERR_DENIED, BH_ERR_ACCESS_DENIED},
        {K_ERR_CAP_STALE, BH_ERR_STALE_CAPABILITY}};
    for (unsigned i = 0; i < sizeof(cases)/sizeof(cases[0]); ++i) {
        result = bh_op_result_kstatus(cases[i].kernel);
        assert(bh_syscall_gate(&frame, &info) == cases[i].native);
    }
    result = bh_op_result_value(123);
    assert(bh_syscall_gate(&frame, &info) == 123);
    puts("PASS: native status normalization without optional personality registration");
}
