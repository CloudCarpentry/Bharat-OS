#include <stdio.h>
#include <assert.h>
#include "compat/android/android_security.h"

static void test_security_domain_transitions(void) {
    printf("  Testing Android security domain transitions...\n");

    /* Same-domain requests must succeed */
    assert(android_sec_transition_domain(ANDROID_SEC_SID_INIT, ANDROID_SEC_SID_INIT) == 0);
    assert(android_sec_transition_domain(ANDROID_SEC_SID_APP, ANDROID_SEC_SID_APP) == 0);

    /* Valid transitions according to Android lifecycle policy */
    assert(android_sec_transition_domain(ANDROID_SEC_SID_KERNEL, ANDROID_SEC_SID_INIT) == 0);
    assert(android_sec_transition_domain(ANDROID_SEC_SID_INIT, ANDROID_SEC_SID_SERVICEMANAGER) == 0);
    assert(android_sec_transition_domain(ANDROID_SEC_SID_INIT, ANDROID_SEC_SID_SYSTEM_SERVER) == 0);
    assert(android_sec_transition_domain(ANDROID_SEC_SID_INIT, ANDROID_SEC_SID_ZYGOTE) == 0);
    assert(android_sec_transition_domain(ANDROID_SEC_SID_ZYGOTE, ANDROID_SEC_SID_APP) == 0);
    assert(android_sec_transition_domain(ANDROID_SEC_SID_ZYGOTE, ANDROID_SEC_SID_ISOLATED_APP) == 0);
    assert(android_sec_transition_domain(ANDROID_SEC_SID_APP, ANDROID_SEC_SID_ISOLATED_APP) == 0);

    /* Unauthorized privilege escalation transitions MUST fail closed */
    assert(android_sec_transition_domain(ANDROID_SEC_SID_APP, ANDROID_SEC_SID_SYSTEM_SERVER) < 0);
    assert(android_sec_transition_domain(ANDROID_SEC_SID_APP, ANDROID_SEC_SID_INIT) < 0);
    assert(android_sec_transition_domain(ANDROID_SEC_SID_ISOLATED_APP, ANDROID_SEC_SID_APP) < 0);
    assert(android_sec_transition_domain(ANDROID_SEC_SID_ISOLATED_APP, ANDROID_SEC_SID_SYSTEM_SERVER) < 0);

    /* Invalid SIDs MUST fail closed */
    assert(android_sec_transition_domain(0, 0) < 0);
    assert(android_sec_transition_domain(ANDROID_SEC_SID_INIT, 0) < 0);
    assert(android_sec_transition_domain(0, ANDROID_SEC_SID_APP) < 0);
}

static void test_security_memory_access(void) {
    printf("  Testing Android security memory access checks...\n");

    android_logical_obj_t obj = {
        .id = 1,
        .home_core = 0,
        .generation = 1,
        .backing_cap = {0}
    };

    /* Valid memory query access must succeed */
    assert(android_sec_check_memory_access(ANDROID_SEC_SID_APP, &obj, 0) == 0);
    assert(android_sec_check_memory_access(ANDROID_SEC_SID_APP, &obj, 1 /* PROT_READ */) == 0);

    /* Isolated app attempting write access to shared memory must be denied */
    assert(android_sec_check_memory_access(ANDROID_SEC_SID_ISOLATED_APP, &obj, 2 /* PROT_WRITE */) < 0);

    /* Invalid object or invalid SID MUST fail closed */
    assert(android_sec_check_memory_access(0, NULL, -1) < 0);
    assert(android_sec_check_memory_access(0, &obj, 0) < 0);
    assert(android_sec_check_memory_access(ANDROID_SEC_SID_APP, NULL, 0) < 0);
}

static void test_security_binder_access(void) {
    printf("  Testing Android security Binder transaction checks...\n");

    /* Intra-domain communication must succeed */
    assert(android_sec_check_binder_access(ANDROID_SEC_SID_APP, ANDROID_SEC_SID_APP, ANDROID_BINDER_ACTION_CALL) == 0);

    /* Privileged domains (Kernel and Init) must succeed */
    assert(android_sec_check_binder_access(ANDROID_SEC_SID_KERNEL, ANDROID_SEC_SID_SERVICEMANAGER, ANDROID_BINDER_ACTION_CALL) == 0);
    assert(android_sec_check_binder_access(ANDROID_SEC_SID_INIT, ANDROID_SEC_SID_SYSTEM_SERVER, ANDROID_BINDER_ACTION_CALL) == 0);

    /* App calling system server and service manager must succeed */
    assert(android_sec_check_binder_access(ANDROID_SEC_SID_APP, ANDROID_SEC_SID_SERVICEMANAGER, ANDROID_BINDER_ACTION_CALL) == 0);
    assert(android_sec_check_binder_access(ANDROID_SEC_SID_APP, ANDROID_SEC_SID_SYSTEM_SERVER, ANDROID_BINDER_ACTION_CALL) == 0);

    /* System server calling apps and zygote must succeed */
    assert(android_sec_check_binder_access(ANDROID_SEC_SID_SYSTEM_SERVER, ANDROID_SEC_SID_APP, ANDROID_BINDER_ACTION_CALL) == 0);
    assert(android_sec_check_binder_access(ANDROID_SEC_SID_SYSTEM_SERVER, ANDROID_SEC_SID_ZYGOTE, ANDROID_BINDER_ACTION_CALL) == 0);

    /* Unauthorized binder calls MUST fail closed */
    /* Apps cannot invoke Zygote, Init, or Kernel directly */
    assert(android_sec_check_binder_access(ANDROID_SEC_SID_APP, ANDROID_SEC_SID_ZYGOTE, ANDROID_BINDER_ACTION_CALL) < 0);
    assert(android_sec_check_binder_access(ANDROID_SEC_SID_APP, ANDROID_SEC_SID_INIT, ANDROID_BINDER_ACTION_CALL) < 0);

    /* Isolated app cannot access ServiceManager or SystemServer directly */
    assert(android_sec_check_binder_access(ANDROID_SEC_SID_ISOLATED_APP, ANDROID_SEC_SID_SERVICEMANAGER, ANDROID_BINDER_ACTION_CALL) < 0);
    assert(android_sec_check_binder_access(ANDROID_SEC_SID_ISOLATED_APP, ANDROID_SEC_SID_SYSTEM_SERVER, ANDROID_BINDER_ACTION_CALL) < 0);

    /* Invalid SIDs and zero action MUST fail closed */
    assert(android_sec_check_binder_access(0, 0, 0) < 0);
    assert(android_sec_check_binder_access(ANDROID_SEC_SID_APP, 0, ANDROID_BINDER_ACTION_CALL) < 0);
    assert(android_sec_check_binder_access(0, ANDROID_SEC_SID_APP, ANDROID_BINDER_ACTION_CALL) < 0);
    assert(android_sec_check_binder_access(ANDROID_SEC_SID_APP, ANDROID_SEC_SID_SYSTEM_SERVER, 0) < 0);
}

int main(void) {
    printf("Running test_android_security (Fail-Closed Contract Verification)...\n");

    test_security_domain_transitions();
    test_security_memory_access();
    test_security_binder_access();

    printf("test_android_security passed!\n");
    return 0;
}
