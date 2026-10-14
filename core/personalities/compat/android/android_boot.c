#include "compat/android/android_boot.h"
#include "compat/android/android_binder_dist.h"
#include "compat/android/android_process.h"
#include "compat/android/android_security.h"
#include <stddef.h>

static android_process_desc_t* g_android_init_proc = NULL;
static android_binder_handle_t g_servicemanager_handle = {0};

int android_boot_early_init(android_personality_t* p) {
    if (!p) {
        return -1;
    }

    /* Initialize Android personality descriptor */
    int res = android_personality_register(p);
    if (res != 0) {
        return res;
    }

    /* Initialize core 0 distributed Binder endpoint */
    android_binder_endpoint_t ep;
    res = android_binder_dist_init_core(&ep, 0);
    if (res != 0) {
        return res;
    }

    return 0;
}

int android_boot_start_init(android_personality_t* p, const char* init_path) {
    if (!p || !init_path) {
        return -1;
    }

    /* Spawn root Android init task (PID 1) */
    int res = android_clone_task(NULL, 0, &g_android_init_proc);
    if (res != 0 || !g_android_init_proc) {
        return -1;
    }

    return 0;
}

int android_boot_on_servicemanager_ready(void) {
    /* Register ServiceManager node 0 with authoritative security SID */
    return android_binder_node_register(
        0,                                   /* service_id = 0 */
        0,                                   /* home_core = 0 */
        g_android_init_proc ? g_android_init_proc->process_id : 1,
        ANDROID_SEC_SID_SERVICEMANAGER,
        &g_servicemanager_handle
    );
}
