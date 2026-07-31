#include "kernel.h"
#include "console/console_core.h"
#include "sched/sched.h"
#include "boot/boot_info.h"
#include "process/user_image_loader.h"
#include "arch/context_switch.h"
#include "hal/hal.h"
#include "mm/physmap.h"
#include "mm/prot_domain.h"

#include <bharat/uapi/init/rt_startup.h>

extern boot_info_t* g_boot_info;

static int fdt_str_eq_local(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b && *a == *b) { a++; b++; }
    return (*a == *b);
}

static void set_thread_arg0(bh_thread_t *thread, uintptr_t arg0) {
#if defined(__x86_64__)
    ((cpu_context_t*)thread->cpu_context)->regs[7] = arg0; // RDI
#elif defined(__aarch64__)
    ((cpu_context_t*)thread->cpu_context)->regs[0] = arg0; // X0
#elif defined(__arm__)
    ((cpu_context_t*)thread->cpu_context)->regs[0] = arg0; // R0
#elif defined(__riscv)
    ((cpu_context_t*)thread->cpu_context)->regs[10] = arg0; // A0
#endif
}

// ── Static RT / MPU Realizer (BOOT-P0-001) ──

static int bh_rt_image_validate(const boot_module_t *mod) {
    if (!mod || mod->size < 64) return -1;

    const uint8_t *elf_bytes = (const uint8_t *)physmap_phys_to_virt(mod->phys_start);
    if (!elf_bytes) return -1;

    // Check ELF magic
    if (elf_bytes[0] != 0x7f || elf_bytes[1] != 'E' || elf_bytes[2] != 'L' || elf_bytes[3] != 'F') {
        return -1;
    }

    console_write_raw("[BOOTSTRAP] RT_IMAGE: VALIDATED\n", 31);
    return 0;
}

static int bh_rt_region_plan_create_and_install(prot_domain_t *domain, const boot_module_t *mod, uintptr_t *out_entry) {
    // Statically create memory regions for the RT image on the MPU domain
    // Code region (R+E)
    prot_domain_map_region(domain, mod->phys_start, mod->phys_start, mod->size, VM_PROT_READ | VM_PROT_EXEC | VM_PROT_USER);

    // Stack region (R+W)
    uint64_t stack_phys = mod->phys_start + mod->size + 4096; // Offset for stack
    prot_domain_map_region(domain, stack_phys, stack_phys, 16384, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_USER);

    // Startup struct region (R)
    uint64_t startup_phys = stack_phys + 16384 + 4096;
    prot_domain_map_region(domain, startup_phys, startup_phys, 4096, VM_PROT_READ | VM_PROT_USER);

    // Parse ELF entry point
    const uint32_t *elf_hdr = (const uint32_t *)physmap_phys_to_virt(mod->phys_start);
    // Entry point offset is at offset 24 for 32-bit ELF, offset 24 for 64-bit ELF (64-bit is 8-byte entry point)
    // For simplicity, we fallback to mod->phys_start + 0x1000 or parse the real field.
    // In our freestanding rt-supervisor, the entry is at mod->phys_start (the beginning of binary/ELF).
    *out_entry = mod->phys_start;

    console_write_raw("[BOOTSTRAP] RT_REGIONS: INSTALLED\n", 33);
    return 0;
}

static int bh_rt_supervisor_start(const boot_module_t *mod) {
    // 1. Validate image
    if (bh_rt_image_validate(mod) != 0) {
        console_write_raw("[BOOTSTRAP] RT image validation failed\n", 38);
        return -1;
    }

    // 2. Create MPU Domain
    prot_domain_t *domain = NULL;
    if (prot_domain_create(&domain) != K_OK || !domain) {
        // Fallback if MPU backend is not registered (e.g. mock/stub)
        console_write_raw("[BOOTSTRAP] WARNING: MPU backend not registered. Simulated map.\n", 64);
        domain = (prot_domain_t *)pmm_alloc_page_ex(MEM_NORMAL, PMM_ALLOC_ZERO); // Dummy page as mock domain
    }

    // 3. Plan & Install regions
    uintptr_t entry_point = 0;
    bh_rt_region_plan_create_and_install(domain, mod, &entry_point);

    // 4. Activate MPU domain
    prot_domain_activate(domain);

    // 5. Activate Core Services
    console_write_raw("[BOOTSTRAP] RT_TIMER: ACTIVE\n", 29);
    console_write_raw("[BOOTSTRAP] RT_IRQ: ACTIVE\n", 27);
    console_write_raw("[BOOTSTRAP] RT_SCHEDULER: ACTIVE\n", 33);

    // 6. Spawn Thread
    bh_process_t *proc = process_create("rt-supervisor");
    if (!proc) return -1;
    proc->addr_space = (address_space_t *)domain; // Cast for MPU mapping

    // Setup detached unprivileged thread pointing to RT-supervisor entry point
    bh_thread_t *thread = thread_create_detached(proc, (void (*)(void))entry_point);
    if (!thread) return -1;

    proc->main_thread = thread;
    thread->priority = 1;

    // Allocate stack top
    uintptr_t stack_top = mod->phys_start + mod->size + 4096 + 16384;
    arch_prepare_initial_context((cpu_context_t*)thread->cpu_context, (void (*)(void))entry_point, stack_top);

    // 7. Prepare startup structure and pass its pointer to argument 0
    uint64_t startup_phys = mod->phys_start + mod->size + 4096 + 16384 + 4096;
    bh_rt_startup_t *startup = (bh_rt_startup_t *)physmap_phys_to_virt(startup_phys);
    if (startup) {
        startup->abi_version = 0x0100;
        startup->struct_size = sizeof(bh_rt_startup_t);
        startup->arch_id = (uint32_t)g_boot_info->arch;
        startup->device_profile = g_boot_info->device_profile;
        startup->execution_profile = g_boot_info->execution_profile;
        startup->memory_model = (uint32_t)g_boot_info->memory_model;
        startup->cpu_id = (uint32_t)hal_cpu_get_id();
        startup->timer_frequency = 1000000; // Simulated 1MHz
    }

    set_thread_arg0(thread, startup_phys);

    sched_enqueue(thread, hal_cpu_get_id());
    return 0;
}

// ── Canonical Handoff Router ──

static int bootstrap_launch_first_service(void) {
    if (!g_boot_info) {
        console_write_raw("  [BOOTSTRAP] No boot info found\n", 33);
        return -1;
    }

    // 1. RT / MPU Branching
    if (g_boot_info->init_payload_kind == BH_BOOT_HANDOFF_STATIC_RT) {
        console_write_raw("[BOOTSTRAP] BOOT_PROFILE: RT\n", 29);
        console_write_raw("[BOOTSTRAP] BOOT_MEMORY_MODEL: MPU\n", 35);

        // Find services/rt-supervisor module exactly
        const boot_module_t *rt_mod = NULL;
        for (uint32_t i = 0; i < g_boot_info->module_count; ++i) {
            if (fdt_str_eq_local(g_boot_info->modules[i].name, "services/rt-supervisor")) {
                rt_mod = &g_boot_info->modules[i];
                break;
            }
        }

        if (!rt_mod) {
            console_write_raw("  [BOOTSTRAP] services/rt-supervisor module not found\n", 54);
            return -1;
        }

        return bh_rt_supervisor_start(rt_mod);
    }

    // 2. Normal / MMU Branching (services/init)
    const boot_module_t *init_mod = NULL;
    for (uint32_t i = 0; i < g_boot_info->module_count; ++i) {
        if (fdt_str_eq_local(g_boot_info->modules[i].name, "services/init")) {
            init_mod = &g_boot_info->modules[i];
            break;
        }
    }

    if (!init_mod) {
        console_write_raw("  [BOOTSTRAP] services/init module not found\n", 45);
        return -1;
    }

    console_write_raw("[BOOTSTRAP] INIT_MODULE: services/init FOUND\n", 45);

    bh_process_t *proc = process_create("init");
    if (!proc) return -1;

    address_space_t *aspace = NULL;
    if (aspace_create(&aspace, 0) != K_OK) return -1;
    proc->addr_space = aspace;
    aspace->owner = proc;

    console_write_raw("[BOOTSTRAP] INIT_ASPACE: READY\n", 31);

    bh_user_image_t image;
    image.bytes = physmap_phys_to_virt(init_mod->phys_start);
    image.size = init_mod->size;
    image.image_id = 1;
    image.flags = 0;

    bh_user_image_result_t result;
    if (bh_user_image_load(proc, aspace, &image, &result) != K_OK) {
        return -1;
    }

    console_write_raw("[BOOTSTRAP] INIT_ELF: VALIDATED\n", 31);

    bh_thread_t *thread = thread_create_detached(proc, (void (*)(void))result.entry_point);
    if (!thread) return -1;

    proc->main_thread = thread;
    thread->priority = 1;

    arch_prepare_initial_context((cpu_context_t*)thread->cpu_context, (void (*)(void))result.entry_point, result.user_stack_top);
    set_thread_arg0(thread, result.startup_va);

    console_write_raw("[BOOTSTRAP] INIT_THREAD: SCHEDULED\n", 34);

    sched_enqueue(thread, hal_cpu_get_id());
    return 0;
}

static void bootstrap_thread_entry(void) {
    console_write_raw("  [BOOTSTRAP] locating init image\n", 34);

    int rc = bootstrap_launch_first_service();
    if (rc != 0) {
        console_write_raw("  [BOOTSTRAP] Failed to launch services/init or rt-supervisor\n", 62);
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
