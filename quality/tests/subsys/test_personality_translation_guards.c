#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "compat/android/android_binder_dist.h"
#include "personality/translation_events.h"
#include "../../personalities/compat/linux/linux_compat.h"

void bh_platform_early_console_write_string(const char* str) {
    (void)str;
}

static void test_linux_translation_counters(void) {
    subsys_instance_t env;
    memset(&env, 0, sizeof(env));
    env.type = SUBSYS_TYPE_LINUX;
    env.is_running = 1;

    assert(linux_subsys_init(&env) == 0);

    bh_translation_counters_reset();

    long ret = linux_syscall_handler(1, 1, (long)"ok", 2, 0, 0, 0);
    assert(ret == 2);

    bh_translation_counters_t counters;
    bh_translation_counters_snapshot(&counters);
    assert(counters.boundary_enter == 1);
    assert(counters.boundary_exit == 1);
    assert(counters.cache_miss == 0);
    assert(counters.fallback == 0);

    ret = linux_syscall_handler(0, 999, 0, 0, 0, 0, 0);
    assert(ret == -9);

    bh_translation_counters_snapshot(&counters);
    assert(counters.boundary_enter == 2);
    assert(counters.boundary_exit == 2);
    assert(counters.cache_miss == 1);

    ret = linux_syscall_handler(9, 0, 0, 0, 0, 0, 0);
    assert(ret == -38);

    bh_translation_counters_snapshot(&counters);
    assert(counters.boundary_enter == 3);
    assert(counters.boundary_exit == 3);
    assert(counters.fallback >= 1);

    printf("[PASS] test_linux_translation_counters\n");
}

static void test_android_binder_translation_counters(void) {
    android_binder_transaction_t txn;
    memset(&txn, 0, sizeof(txn));

    bh_translation_counters_reset();

    assert(android_binder_transact(&txn) == 0);
    assert(android_binder_reply(&txn) == 0);

    bh_translation_counters_t counters;
    bh_translation_counters_snapshot(&counters);

    assert(counters.boundary_enter == 2);
    assert(counters.boundary_exit == 2);
    assert(counters.fallback == 2);
    assert(counters.extra_copy_events == 0);
    assert(counters.extra_copy_bytes == 0);

    printf("[PASS] test_android_binder_translation_counters\n");
}

int main(void) {
    printf("Running personality translation guard tests...\n");

    test_linux_translation_counters();
    test_android_binder_translation_counters();

    printf("Personality translation guard tests passed successfully.\n");
    return 0;
}
