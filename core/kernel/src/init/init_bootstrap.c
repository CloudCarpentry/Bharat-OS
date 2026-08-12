#include "kernel.h"
#include "console/console_core.h"
#include "sched/sched.h"
#include "boot/boot_info.h"
#include "process/user_image_loader.h"
#include "hal/hal.h"
#include "mm/physmap.h"
#include "lib/base/string.h"

extern boot_info_t* g_boot_info;

static int fdt_str_eq_local(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b && *a == *b) { a++; b++; }
    return (*a == *b);
}

static void init_boot_write(const char *s) {
    console_write_raw(s, string_length(s));
}

static const char *init_boot_arch_name(void) {
    return BHARAT_ARCH_NAME;
}

static const char *init_boot_kstatus_name(kstatus_t status) {
    if (status == K_OK) return "K_OK";
    if (status == K_ERR_INVALID_ARG) return "K_ERR_INVALID_ARG";
    if (status == K_ERR_NOT_FOUND) return "K_ERR_NOT_FOUND";
    if (status == K_ERR_UNSUPPORTED) return "K_ERR_UNSUPPORTED";
    if (status == K_ERR_NO_MEMORY) return "K_ERR_NO_MEMORY";
    if (status == K_ERR_NO_RESOURCES) return "K_ERR_NO_RESOURCES";
    if (status == K_ERR_VM_UNMAPPED) return "K_ERR_VM_UNMAPPED";
    if (status == K_ERR_DENIED) return "K_ERR_DENIED";
    return "K_ERR_OTHER";
}

static void init_boot_fail(const char *stage, kstatus_t status) {
    init_boot_write("BOOT_FAIL: component=INIT stage=");
    init_boot_write(stage);
    init_boot_write(" status=");
    init_boot_write(init_boot_kstatus_name(status));
    init_boot_write(" arch=");
    init_boot_write(init_boot_arch_name());
    init_boot_write("\n");
}

static void init_boot_stage(const char *stage) {
    init_boot_write("INIT_STAGE: ");
    init_boot_write(stage);
    init_boot_write("\n");
}

/* The packaged root is always launched by the canonical loader below.  Lifecycle
 * model selection is tooling policy (ADR-021), never an ISA/MMU branch here. */

// ── Canonical Handoff Router ──


#include "arch/user_entry.h"
#include "slab.h"

static void loader_print_hex64(uint64_t val) {
    char buf[17];
    for (int i = 15; i >= 0; --i) {
        int nibble = (val >> (i * 4)) & 0xF;
        buf[15 - i] = nibble < 10 ? '0' + nibble : 'a' + (nibble - 10);
    }
    buf[16] = '\0';
    console_write_raw(buf, 16);
}



#define ARCH_USER_ENTRY_MAGIC 0x42554855454E5452ULL


void generic_user_init_trampoline(void *arg) {
    console_write_raw("[TRAMPOLINE_REACHED]\n", 21);
    init_boot_stage("USER_ENTRY");



    bh_thread_t *self = sched_current_thread();
    arch_user_entry_t *expected = &self->first_user_entry;
    arch_user_entry_t *entry = (arch_user_entry_t *)arg;

    console_write_raw("ENTRY_TRAMPOLINE: received=", 27);
    loader_print_hex64((uint64_t)(uintptr_t)entry);
    console_write_raw(" expected=", 10);
    loader_print_hex64((uint64_t)(uintptr_t)expected);
    console_write_raw(" received_magic=", 16);
    loader_print_hex64(entry ? entry->flags : 0);
    console_write_raw(" expected_magic=", 16);
    loader_print_hex64(expected->flags);
    console_write_raw("\n", 1);

    if (entry != expected) {
        init_boot_fail("USER_ENTRY_ARG_POINTER_MISMATCH", -1);
        kernel_panic("USER_ENTRY_ARG_POINTER mismatch");
    }

    if (!entry || entry->flags != ARCH_USER_ENTRY_MAGIC) {
        init_boot_fail("USER_ENTRY_MAGIC_MISMATCH", -1);
        kernel_panic("USER_ENTRY_MAGIC mismatch in generic_user_init_trampoline");
    }

    arch_enter_user(entry);

    init_boot_fail("USER_ENTRY_RETURNED", -1);
    kernel_panic("arch_enter_user unexpectedly returned");
}


static int bootstrap_launch_first_service(void) {
    if (!g_boot_info) {
        console_write_raw("  [BOOTSTRAP] No boot info found\n", 33);
        return -1;
    }

    /* The package contains one authoritative root, independent of ISA/profile. */
    const boot_module_t *init_mod = NULL;
    for (uint32_t i = 0; i < g_boot_info->module_count; ++i) {
        if (g_boot_info->modules[i].phys_start == g_boot_info->init_payload_phys &&
            g_boot_info->modules[i].size == g_boot_info->init_payload_size) {
            init_mod = &g_boot_info->modules[i];
            break;
        }
    }

    if (!init_mod && g_boot_info->module_count > 0U) {
        /*
         * Some early boot protocols preserve the payload but not the module
         * command-line name.  The P0 package contains services/init as the
         * first module, so use that deterministic package contract as a
         * fallback while still rejecting an empty module table.
         */
        init_mod = &g_boot_info->modules[0];
    }

    if (!init_mod) {
        /*
         * Userspace init markers are userspace-originated proof and must never be
         * synthesized by the kernel.  An empty canonical module table means
         * the trusted boot handoff did not provide services/init, so fail
         * closed instead of fabricating lifecycle success.
         */
        console_write_raw("BOOT_FAIL: INIT_MODULE_MISSING\n", 31);
        init_boot_fail("MODULE_DISCOVERED", K_ERR_NOT_FOUND);
        return -1;
    }

    console_write_raw("[BOOTSTRAP] ROOT_MODULE_FOUND\n", 30);
    /* Transitional evidence marker retained while boot contracts migrate. */
    console_write_raw("[BOOTSTRAP] INIT_MODULE: services/init FOUND\n", 45);
    init_boot_stage("MODULE_DISCOVERED");
    init_boot_stage("MODULE_RESERVED");

    bh_process_t *proc = process_create("root");
    if (!proc) {
        init_boot_fail("ASPACE_READY", K_ERR_NO_MEMORY);
        return -1;
    }

    /*
     * process_create() owns exactly one address space for the process. Reuse
     * that authority here instead of allocating a second space and overwriting
     * proc->addr_space.
     */
    address_space_t *aspace = proc->addr_space;
    if (!aspace) {
        init_boot_fail("ASPACE_READY", K_ERR_VM_UNMAPPED);
        return -1;
    }
    aspace->owner = proc;

    console_write_raw("[BOOTSTRAP] INIT_ASPACE: READY\n", 31);
    init_boot_stage("ASPACE_READY");

    bh_user_image_t image;
    image.bytes = physmap_phys_to_virt(init_mod->phys_start);
    image.size = init_mod->size;
    image.image_id = 1;
    image.flags = 0;
    if (!image.bytes) {
        init_boot_fail("MODULE_MAPPED", K_ERR_VM_UNMAPPED);
        return -1;
    }
    init_boot_stage("MODULE_MAPPED");

    bh_user_image_result_t result;
    kstatus_t load_status = bh_user_image_load(proc, aspace, &image, &result);
    if (load_status != K_OK) {
        init_boot_fail("ELF_PLAN", load_status);
        return -1;
    }

    console_write_raw("[BOOTSTRAP] INIT_ELF: VALIDATED\n", 32);


    // Allocate entry struct on heap (immortal for init, or freed if panic)


    arch_user_entry_t local_entry;
    local_entry.flags = ARCH_USER_ENTRY_MAGIC;
    kstatus_t prep_status = arch_user_entry_prepare(&local_entry, aspace, result.entry_point, result.user_stack_top, result.startup_va);
    if (prep_status != K_OK) {
        init_boot_fail("USER_ENTRY_PREPARE", prep_status);
        return -1;
    }

    bh_thread_t *thread = thread_create_detached_arg(proc, generic_user_init_trampoline, &local_entry);

    if (!thread) {
        init_boot_fail("THREAD_CREATED", -1);
        return -1;
    }
    init_boot_stage("THREAD_CREATED");

    console_write_raw("ENTRY_PREP: expected_arg=", 25);
    loader_print_hex64((uint64_t)(uintptr_t)&thread->first_user_entry);
    console_write_raw("\n", 1);

    proc->main_thread = thread;
    thread->priority = 24;

    int status = sched_enqueue(thread, hal_cpu_get_id());
    if (status != 0) {
        init_boot_fail("THREAD_ENQUEUED", status);
        console_write_raw("[BOOTSTRAP] Error: sched_enqueue failed for init thread\n", 56);
        return -1;
    }

    console_write_raw("[BOOTSTRAP] INIT_THREAD: SCHEDULED\n", 35);
    init_boot_stage("THREAD_ENQUEUED");
    return 0;
}

static void bootstrap_thread_entry(void) {
    console_write_raw("  [BOOTSTRAP] locating init image\n", 34);

    int rc = bootstrap_launch_first_service();
    if (rc != 0) {
        console_write_raw("  [BOOTSTRAP] Failed to launch services/init or rt-supervisor\n", 62);
        kernel_panic("bootstrap: first service launch failed");
    }

    console_write_raw("[LAUNCH_FIRST_SERVICE_RETURNED]\n", 32);
    thread_destroy(sched_current_thread());
    bh_thread_yield();
}

void kernel_start_init_service(void) {
    /*
     * Bootstrapping services/init is a kernel lifecycle step, not scheduler
     * policy.  Perform the bounded image validation and initial-thread enqueue
     * synchronously so the headless boot evidence contract is emitted before
     * the first voluntary reschedule.  Once INIT_THREAD is scheduled, normal
     * scheduling decides when the user thread runs.
     */
    console_write_raw("  [BOOTSTRAP] locating init image\n", 34);
    int rc = bootstrap_launch_first_service();
    if (rc != 0) {
        console_write_raw("  [BOOTSTRAP] Failed to launch services/init or rt-supervisor\n", 62);
        kernel_panic("bootstrap: first service launch failed");
    }
}
