#include "kernel.h"
#include "console/console_core.h"
#include "sched/sched.h"

// Minimal placeholder for the ELF/initramfs bootloader until implemented
static int bootstrap_launch_first_service(void) {
    // A future change will implement:
    // create_process("init");
    // create_aspace();
    // load_embedded_elf(init_image);
    // map_segments();
    // create_user_thread(entry, stack_top);
    // grant bootstrap caps/handles
    // sched_enqueue();

    return -1; // -ENOSYS
}

static void bootstrap_thread_entry(void) {
    console_write_raw("  [BOOTSTRAP] Launching services/init...\n", 41);

    int rc = bootstrap_launch_first_service();
    if (rc != 0) {
        console_write_raw("  [BOOTSTRAP] Failed to launch services/init\n", 45);
        kernel_panic("bootstrap: first service launch failed");
    }

    thread_destroy(sched_current_thread());
    kthread_yield();
}

void kernel_start_init_service(void) {
    // Create a dedicated kernel thread to bootstrap user-space
    kprocess_t *proc = process_create("sysmgr");
    if (proc) {
        uint64_t tid = 0;
        sched_sys_thread_create(proc, bootstrap_thread_entry, &tid);
    } else {
        console_write_raw("  [BOOTSTRAP] Failed to create process for sysmgr\n", 50);
        kernel_panic("bootstrap: could not create sysmgr process");
    }
}
