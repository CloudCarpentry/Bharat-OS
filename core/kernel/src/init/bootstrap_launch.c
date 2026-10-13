#include "kernel.h"
#include "hal/hal.h"
#include "sched/sched.h"
#include "process/user_image_loader.h"
#include "arch/user_entry.h"
#include "capability.h"
#include "ipc_endpoint.h"
#include "syscall/usercopy.h"
#include "trap/syscall_status.h"
#include "boot/boot_info.h"
#include "mm/physmap.h"
#include "lib/base/string.h"
#include "slab.h"
#include <bharat/uapi/init/bootstrap.h>
#include <bharat/uapi/bootstrap/service_launch.h>

extern boot_info_t *g_boot_info;
extern void generic_user_init_trampoline(void *arg);

static int same_name(const char *a, const char *b) {
    if (!a || !b) return 0;
    for (size_t i = 0; i < 32; ++i) {
        if (a[i] != b[i]) return 0;
        if (a[i] == 0) return 1;
    }
    return 0;
}

static kstatus_t grant_endpoint(capability_table_t *source, uint32_t source_cap,
                               cap_rights_mask_t source_rights,
                               capability_table_t *target, cap_rights_mask_t rights,
                               uint32_t *out) {
    capability_entry_t entry;
    if (cap_table_lookup(source, source_cap, CAP_TYPE_ENDPOINT,
        source_rights | CAP_RIGHT_DELEGATE, &entry) != 0)
        return K_ERR_CAP_DENIED;
    return cap_table_delegate(source, target, source_cap, rights, out) == 0
        ? K_OK : K_ERR_NO_RESOURCES;
}

/* All objects are owned by the calling core. No kernel service graph or READY
 * state is maintained: readiness travels on child-specific userspace IPC. */
int cap_invoke(uintptr_t cap_id, uintptr_t opcode, uintptr_t arg0, uintptr_t arg1) {
    bh_process_t *parent = sched_current_process();
    if (!parent || !parent->security_sandbox_ctx || cap_id > UINT32_MAX)
        return K_ERR_CAP_INVALID;
    capability_table_t *parent_table = parent->security_sandbox_ctx;
    if (opcode >= BH_PROCESS_OP_START && opcode <= BH_PROCESS_OP_TERMINATE) {
        bh_process_object_t object;
        kstatus_t status = cap_lookup_process(parent_table, cap_id, CAP_RIGHT_PROCESS_MANAGE, &object);
        if (status != K_OK) return status;
        bh_process_t *child = object.process;
        if (!child || !child->security_sandbox_ctx) return K_ERR_CAP_STALE;
        if (child == parent || child->owner_core_id != hal_cpu_get_id()) return K_ERR_CAP_OWNERSHIP;
        if (opcode == BH_PROCESS_OP_QUERY) {
            if (!child->main_thread) return K_ERR_BAD_STATE;
            bh_process_image_result_t result = {.process_id = child->process_id,
                .thread_id = child->main_thread->thread_id,
                .entry_point = child->main_thread->first_user_entry.entry_pc,
                .process_cap = cap_id};
            return bh_status_to_kstatus(bh_copy_to_user((void *)arg1, &result, sizeof(result)));
        }
        if (opcode == BH_PROCESS_OP_START) {
            if (!child->main_thread || child->main_thread->owner_state != THREAD_OWNER_NONE)
                return K_ERR_BAD_STATE;
            return sched_enqueue(child->main_thread, hal_cpu_get_id());
        }
        if (opcode == BH_PROCESS_OP_TERMINATE) {
            if (!child->main_thread) return K_OK;
            /* The existing endpoint backend has no cancellation operation.
             * Retain blocked objects rather than free a linked wait-queue node. */
            if (child->main_thread->state == THREAD_STATE_BLOCKED) return K_ERR_UNSUPPORTED;
            status = sched_terminate_tid(child->main_thread->thread_id);
            if (status == K_OK) child->main_thread = NULL;
            return status;
        }
        /* Scheduler-owned deferred teardown must finish before process_destroy
         * can succeed; its live-thread check provides bounded retry behavior. */
        if (child->main_thread) return K_ERR_BAD_STATE;
        return process_destroy(child);
    }
    capability_entry_t authority;
    if (cap_table_lookup(parent_table, (uint32_t)cap_id, CAP_TYPE_BOOTSTRAP,
                         CAP_RIGHT_BOOTSTRAP_LAUNCH, &authority) != 0)
        return K_ERR_CAP_DENIED;
    if (authority.object_ref != (uint64_t)(uintptr_t)parent ||
        parent->owner_core_id != hal_cpu_get_id()) return K_ERR_CAP_OWNERSHIP;
    if (opcode == BH_BOOTSTRAP_OP_PROBE) return K_OK;
    if (opcode == BH_BOOTSTRAP_OP_CREATE_IMAGE) {
        bh_process_image_request_t req;
        bh_status_t copy = bh_copy_from_user(&req, (const void *)arg0, sizeof(req));
        if (copy != BH_OK) return bh_status_to_kstatus(copy);
        if (req.version != BH_BOOTSTRAP_SERVICE_ABI || !req.image_size ||
            req.image_size > BH_BOOTSTRAP_IMAGE_MAX || req.image_address > UINTPTR_MAX ||
            req.image_size > UINTPTR_MAX - req.image_address ||
            req.priority > SCHED_MAX_PRIORITY || req.name[31] != 0)
            return K_ERR_INVALID_ARG;
        copy = bh_user_range_validate((void *)arg1, sizeof(bh_process_image_result_t), BH_USER_ACCESS_WRITE);
        if (copy != BH_OK) return bh_status_to_kstatus(copy);
        void *bytes = kmalloc(req.image_size);
        if (!bytes) return K_ERR_NO_MEMORY;
        /* Each fault-safe copy is page-bounded; usercopy deliberately rejects
         * a single copy exceeding its small-buffer limit. */
        for (size_t offset = 0; offset < req.image_size; offset += PAGE_SIZE) {
            size_t amount = req.image_size - offset;
            if (amount > PAGE_SIZE) amount = PAGE_SIZE;
            copy = bh_copy_from_user((uint8_t *)bytes + offset,
                (const void *)(uintptr_t)(req.image_address + offset), amount);
            if (copy != BH_OK) { kfree(bytes); return bh_status_to_kstatus(copy); }
        }
        bh_process_t *child = process_create(req.name);
        if (!child) { kfree(bytes); return K_ERR_NO_RESOURCES; }
        bh_user_image_t image = {.bytes = bytes, .size = req.image_size};
        bh_user_image_result_t loaded;
        kstatus_t status = bh_user_image_load(child, child->addr_space, &image, &loaded);
        kfree(bytes);
        uint32_t process_cap = 0;
        bh_thread_t *thread = NULL;
        if (status == K_OK) {
            arch_user_entry_t entry = {.flags = 0x42554855454e5452ULL};
            status = arch_user_entry_prepare(&entry, child->addr_space, loaded.entry_point,
                                            loaded.user_stack_top, loaded.startup_va);
            if (status == K_OK) {
                thread = thread_create_detached_arg(child, generic_user_init_trampoline, &entry);
                if (!thread) status = K_ERR_NO_RESOURCES;
            }
        }
        if (status == K_OK && cap_table_delegate(child->security_sandbox_ctx, parent_table,
            loaded.self_process_cap, CAP_RIGHT_PROCESS_MANAGE, &process_cap) != 0)
            status = K_ERR_NO_RESOURCES;
        if (status == K_OK) {
            child->main_thread = thread;
            thread->priority = req.priority;
            bh_process_image_result_t result = {.process_id = child->process_id,
                .thread_id = thread->thread_id, .entry_point = loaded.entry_point, .process_cap = process_cap};
            status = bh_status_to_kstatus(bh_copy_to_user((void *)arg1, &result, sizeof(result)));
        }
        if (status != K_OK) {
            if (thread) (void)thread_destroy(thread);
            if (process_cap) cap_table_revoke(parent_table, process_cap);
            (void)process_destroy(child);
        }
        return status;
    }
    if (opcode != BH_BOOTSTRAP_OP_LAUNCH) return K_ERR_UNSUPPORTED;

    bh_bootstrap_launch_request_t req;
    bh_status_t copy = bh_copy_from_user(&req, (const void *)arg0, sizeof(req));
    if (copy != BH_OK) return bh_status_to_kstatus(copy);
    copy = bh_user_range_validate((void *)arg1, sizeof(bh_bootstrap_launch_result_t), BH_USER_ACCESS_WRITE);
    if (copy != BH_OK) return bh_status_to_kstatus(copy);
    if (req.version != BH_BOOTSTRAP_SERVICE_ABI || req.service_id == 0 ||
        req.delegate_launch > 1 || req.module_name[0] == 0 || req.module_name[31] != 0)
        return K_ERR_INVALID_ARG;
    /* The loader gives an authorized launcher LAUNCH/BIND/DELEGATE. Never
     * amplify the caller's bootstrap authority when creating that child. */
    const cap_rights_mask_t delegated_authority = CAP_RIGHT_BOOTSTRAP_BIND | CAP_RIGHT_DELEGATE;
    if (req.delegate_launch && (authority.rights & delegated_authority) != delegated_authority)
        return K_ERR_CAP_DENIED;
    const boot_module_t *module = NULL;
    if (g_boot_info) {
        for (uint32_t i = 0; i < g_boot_info->module_count; ++i) {
            if (same_name(g_boot_info->modules[i].name, req.module_name)) {
                module = &g_boot_info->modules[i];
                break;
            }
        }
    }
    if (!module || module->phys_start == g_boot_info->init_payload_phys)
        return K_ERR_NOT_FOUND;

    bh_process_t *child = process_create(req.module_name);
    if (!child) return K_ERR_NO_RESOURCES;
    capability_table_t *child_table = child->security_sandbox_ctx;
    bh_user_image_t image = {.bytes = physmap_phys_to_virt(module->phys_start),
        .size = module->size, .image_id = req.service_id,
        .flags = req.delegate_launch ? BH_USER_IMAGE_BOOTSTRAP_AUTHORITY : 0};
    bh_user_image_result_t loaded;
    kstatus_t status = bh_user_image_load(child, child->addr_space, &image, &loaded);
    bh_thread_t *thread = NULL;
    uint32_t event_send = 0;
    bh_bootstrap_launch_result_t result = {0};
    if (status != K_OK) goto fail;
    uintptr_t startup_phys = 0;
    uint32_t prot;
    status = prot_domain_query_region(child->addr_space->prot_domain, loaded.startup_va, &startup_phys, &prot);
    if (status != K_OK) goto fail;
    bharat_user_startup_t *startup = physmap_phys_to_virt(startup_phys);
    if (!startup) { status = K_ERR_VM_UNMAPPED; goto fail; }
    uint32_t child_service_send = (uint32_t)startup->bootstrap.namesvc_endpoint;

    if (req.namesvc_cap) {
        /* Discovery SEND authority is delegated, never copied as a local ID. */
        uint32_t namesvc_send = 0;
        status = grant_endpoint(parent_table, req.namesvc_cap,
            CAP_RIGHT_ENDPOINT_SEND | CAP_RIGHT_DELEGATE, child_table,
            CAP_RIGHT_ENDPOINT_SEND, &namesvc_send);
        if (status != K_OK) goto fail;
        startup->bootstrap.namesvc_endpoint = namesvc_send;
    } else {
        /* Bind the root's discovery endpoint to the first discovery server.
         * BOOTSTRAP_BIND is distinct from ordinary SEND delegation. */
        if (!(authority.rights & CAP_RIGHT_BOOTSTRAP_BIND)) { status = K_ERR_CAP_DENIED; goto fail; }
        /* Its own freshly created endpoint is returned to the caller below. */
    }
    status = grant_endpoint(child_table, child_service_send,
        CAP_RIGHT_ENDPOINT_SEND, parent_table,
        CAP_RIGHT_ENDPOINT_SEND | CAP_RIGHT_DELEGATE, &result.service_send_cap);
    if (status != K_OK) goto fail;

    if (ipc_endpoint_create(parent_table, &event_send, &result.event_receive_cap) != IPC_OK) {
        status = K_ERR_NO_RESOURCES; goto fail;
    }
    uint32_t child_event_send = 0;
    status = grant_endpoint(parent_table, event_send, CAP_RIGHT_ENDPOINT_SEND,
        child_table, CAP_RIGHT_ENDPOINT_SEND, &child_event_send);
    if (status != K_OK) goto fail;
    /* Keep the parent root alive: revoking it would revoke the child's derived
     * sender. It stays private to the retaining lifecycle authority's CSpace. */
    startup->bootstrap.system_control_endpoint = child_event_send;
    startup->bootstrap.flags = req.service_id;
    if (cap_table_delegate(child_table, parent_table, loaded.self_process_cap,
        CAP_RIGHT_PROCESS_MANAGE, &result.process_cap) != 0) {
        status = K_ERR_NO_RESOURCES; goto fail;
    }
    result.process_id = child->process_id;

    arch_user_entry_t entry = {.flags = 0x42554855454e5452ULL};
    status = arch_user_entry_prepare(&entry, child->addr_space, loaded.entry_point,
                                    loaded.user_stack_top, loaded.startup_va);
    if (status != K_OK) goto fail;
    thread = thread_create_detached_arg(child, generic_user_init_trampoline, &entry);
    if (!thread) { status = K_ERR_NO_RESOURCES; goto fail; }
    child->main_thread = thread;
    thread->priority = 24;
    copy = bh_copy_to_user((void *)arg1, &result, sizeof(result));
    if (copy != BH_OK) { status = bh_status_to_kstatus(copy); goto fail; }
    status = sched_enqueue(thread, hal_cpu_get_id());
    if (status == K_OK) return K_OK;
fail:
    if (thread) (void)thread_destroy(thread);
    if (result.process_cap) cap_table_revoke(parent_table, result.process_cap);
    if (result.service_send_cap) cap_table_revoke(parent_table, result.service_send_cap);
    if (result.event_receive_cap) cap_table_revoke(parent_table, result.event_receive_cap);
    if (event_send) cap_table_revoke(parent_table, event_send);
    (void)process_destroy(child);
    return status;
}
