#include "kernel.h"
#include "console/console_core.h"
#include "sched/sched.h"
#include "boot/boot_info.h"
#include "process/user_image_loader.h"
#include "arch/context_switch.h"
#include "hal/hal.h"
#include "mm/physmap.h"

extern boot_info_t* g_boot_info;

static int bootstrap_launch_first_service(void) {
    if (!g_boot_info) {
        console_write_raw("  [BOOTSTRAP] No boot info found\n", 33);
        return -1;
    }

    // Locate "services/init" module
    const boot_module_t *init_mod = NULL;
    for (uint32_t i = 0; i < g_boot_info->module_count; ++i) {
        // Simplified check, normally would check g_boot_info->modules[i].name
        // if cmdline starts with "services/init" or just use the first module.
        init_mod = &g_boot_info->modules[i];
        break; // Use the first module for now, since it should be init
    }

    if (!init_mod) {
        console_write_raw("  [BOOTSTRAP] services/init module not found\n", 45);
        return -1;
    }

    bh_process_t *proc = process_create("init");
    if (!proc) return -1;

    address_space_t *aspace = NULL;
    if (aspace_create(&aspace, 0) != K_OK) return -1;
    proc->addr_space = aspace;
    aspace->owner = proc;

    bh_user_image_t image;
    image.bytes = physmap_phys_to_virt(init_mod->phys_start);
    image.size = init_mod->size;
    image.image_id = 1;
    image.flags = 0;

    bh_user_image_result_t result;
    if (bh_user_image_load(proc, aspace, &image, &result) != K_OK) {
        return -1;
    }

    bh_thread_t *thread = thread_create_detached(proc, (void (*)(void))result.entry_point);
    if (!thread) return -1;

    proc->main_thread = thread;

    // Use a high base priority for init
    thread->priority = 1;

    arch_prepare_initial_context((cpu_context_t*)thread->cpu_context, (void (*)(void))result.entry_point, result.user_stack_top);

    // Pass startup_va as arg0 to the user process
    // For x86_64, arg0 is in rdi which is gpr[1] via arch_prepare_initial_context (usually, but actually rdi is mapped to arg[0] on syscalls).
    // arch_prepare_initial_context just sets pc and sp. We need to set the argument register.
    // On x86_64, RDI is often offset 7 or similar in cpu_context_t, but let's just use a general way or set it directly.
    // cpu_context_t regs structure varies by arch.
    // For x86_64, rdi is regs[7] in cpu_context_t.
    // To be portable, we should ideally have a `arch_set_arg0` but for now:
#if defined(__x86_64__)
    ((cpu_context_t*)thread->cpu_context)->regs[7] = result.startup_va; // RDI
#elif defined(__aarch64__)
    ((cpu_context_t*)thread->cpu_context)->regs[0] = result.startup_va; // X0
#elif defined(__arm__)
    ((cpu_context_t*)thread->cpu_context)->regs[0] = result.startup_va; // R0
#elif defined(__riscv) && __riscv_xlen == 64
    ((cpu_context_t*)thread->cpu_context)->regs[10] = result.startup_va; // A0
#elif defined(__riscv) && __riscv_xlen == 32
    ((cpu_context_t*)thread->cpu_context)->regs[10] = result.startup_va; // A0
#endif

    console_write_raw("  [BOOTSTRAP] init thread scheduled\n", 36);

    sched_enqueue(thread, hal_cpu_get_id());

    return 0;
}

static void bootstrap_thread_entry(void) {
    console_write_raw("  [BOOTSTRAP] locating init image\n", 34);

    int rc = bootstrap_launch_first_service();
    if (rc != 0) {
        console_write_raw("  [BOOTSTRAP] Failed to launch services/init\n", 45);
        kernel_panic("bootstrap: first service launch failed");
    }

    thread_destroy(sched_current_thread());
    bh_thread_yield();
}

void kernel_start_init_service(void) {
    // Create a dedicated kernel thread to bootstrap user-space
    bh_process_t *proc = process_create("sysmgr");
    if (proc) {
        uint64_t tid = 0;
        sched_sys_thread_create(proc, bootstrap_thread_entry, &tid);
    } else {
        console_write_raw("  [BOOTSTRAP] Failed to create process for sysmgr\n", 50);
        kernel_panic("bootstrap: could not create sysmgr process");
    }
}
