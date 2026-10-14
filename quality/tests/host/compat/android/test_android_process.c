#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "compat/android/android_process.h"
#include "compat/android/android_security.h"
#include "compat/android/android_boot.h"

/* Mock translation event recorder */
void bh_translation_event_record(int event) {
    (void)event;
}

static void test_android_process_lifecycle(void) {
    printf("  Testing Android task cloning and process credentials...\n");

    android_process_desc_t* init_desc = NULL;
    assert(android_clone_task(NULL, 0, &init_desc) == 0);
    assert(init_desc != NULL);
    assert(init_desc->process_id == 1);
    assert(init_desc->selinux_sid == ANDROID_SEC_SID_INIT);
    assert(init_desc->home_core == 0);

    /* Child process inherits from init */
    android_process_desc_t* child_desc = NULL;
    assert(android_clone_task(init_desc, 0, &child_desc) == 0);
    assert(child_desc != NULL);
    assert(child_desc->process_id > 1);
    assert(child_desc->selinux_sid == ANDROID_SEC_SID_INIT);

    /* Negative: NULL out_desc must fail */
    assert(android_clone_task(NULL, 0, NULL) < 0);
}

static void test_android_futex_and_memory(void) {
    printf("  Testing Android futex wait/wake and mmap contracts...\n");

    uint32_t val = 42;
    /* Futex wait with matching value */
    assert(android_futex_wait(&val, 42, 0) == 0);

    /* Futex wait with mismatching value fails closed (-EAGAIN) */
    assert(android_futex_wait(&val, 99, 0) < 0);

    /* Futex wait with NULL uaddr fails closed (-EFAULT) */
    assert(android_futex_wait(NULL, 42, 0) < 0);

    /* Futex wake */
    assert(android_futex_wake(&val, 1) == 0);
    assert(android_futex_wake(NULL, 1) < 0);

    /* Android mmap: valid length returns valid pointer */
    void* ptr = android_mmap(NULL, 4096, 3 /* PROT_READ|WRITE */, 0x22 /* MAP_PRIVATE|ANON */, -1, 0);
    assert(ptr != (void*)-1 && ptr != NULL);

    /* Android mmap: zero length must fail closed */
    assert(android_mmap(NULL, 0, 3, 0x22, -1, 0) == (void*)-1);
}

static void test_android_epoll_and_signals(void) {
    printf("  Testing Android epoll and signal routing contracts...\n");

    /* Epoll create */
    int epfd = android_epoll_create(16);
    assert(epfd >= 0);
    assert(android_epoll_create(0) < 0);
    assert(android_epoll_create(-1) < 0);

    /* Epoll ctl */
    assert(android_epoll_ctl(epfd, 1, 0, NULL) == 0);
    assert(android_epoll_ctl(-1, 1, 0, NULL) < 0);
    assert(android_epoll_ctl(epfd, 1, -1, NULL) < 0);

    /* Epoll wait */
    int dummy_events[4];
    assert(android_epoll_wait(epfd, dummy_events, 4, 100) == 0);
    assert(android_epoll_wait(-1, dummy_events, 4, 100) < 0);
    assert(android_epoll_wait(epfd, NULL, 4, 100) < 0);
    assert(android_epoll_wait(epfd, dummy_events, 0, 100) < 0);

    /* Signals: kill */
    assert(android_kill(1, 15 /* SIGTERM */) == 0);
    assert(android_kill(0, 15) < 0);  /* Invalid PID 0 */
    assert(android_kill(1, 999) < 0); /* Invalid signal */

    /* Signals: rt_sigaction */
    assert(android_rt_sigaction(15, NULL, NULL) == 0);
    assert(android_rt_sigaction(0, NULL, NULL) < 0);   /* Invalid signal 0 */
    assert(android_rt_sigaction(9, NULL, NULL) < 0);   /* SIGKILL cannot be caught */
    assert(android_rt_sigaction(19, NULL, NULL) < 0);  /* SIGSTOP cannot be caught */
}

static void test_android_boot_integration(void) {
    printf("  Testing Android boot and ServiceManager startup...\n");

    android_personality_t p;
    memset(&p, 0, sizeof(p));
    assert(android_boot_early_init(&p) == 0);
    assert(android_boot_start_init(&p, "/system/bin/init") == 0);
    assert(android_boot_on_servicemanager_ready() == 0);

    /* Negative: NULL personality or path must fail closed */
    assert(android_boot_early_init(NULL) < 0);
    assert(android_boot_start_init(NULL, "/init") < 0);
    assert(android_boot_start_init(&p, NULL) < 0);
}

int main(void) {
    printf("Running test_android_process (Android Process, Memory, & Signal Substrate)...\n");

    test_android_process_lifecycle();
    test_android_futex_and_memory();
    test_android_epoll_and_signals();
    test_android_boot_integration();

    printf("test_android_process passed!\n");
    return 0;
}
