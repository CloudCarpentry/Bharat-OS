#include <stdint.h>
#include <stdbool.h>
#include "profile/profile_policy.h"
#include "device.h"
#include "kernel/status.h"
#include "sched/ai_sched.h"

/* Minimal kernel stubs for moved policy modules */

bool bh_profile_has_trait(uint64_t trait) {
    (void)trait;
    return false;
}

bool bh_profile_allows_personality(uint32_t personality_id) {
    (void)personality_id;
    return true;
}

bool bh_profile_allows_blocking_syscall(void) {
    return true;
}

int device_dispatch_irq(uint32_t irq) {
    (void)irq;
    return -1;
}

int device_register_driver(const device_driver_t *driver) {
    (void)driver;
    return 0;
}

int device_register_mmio_window(const device_mmio_window_t *window) {
    (void)window;
    return 0;
}

int device_lookup_mmio_window_l0(uint32_t class_id, uint32_t device_id, uint32_t window_id, void* out_window) {
    (void)class_id; (void)device_id; (void)window_id; (void)out_window;
    return -1;
}

int device_lookup_mmio_window_l1(uint32_t class_id, uint32_t device_id, uint32_t window_id, void* out_window) {
    (void)class_id; (void)device_id; (void)window_id; (void)out_window;
    return -1;
}

int device_framework_init(void) {
    return 0;
}

int device_register_builtin_drivers(void) {
    return 0;
}

int ptp_init(void) {
    return 0;
}

void sched_notify_ipc_ready(uint32_t core_id, uint32_t msg_type) {
    (void)core_id; (void)msg_type;
}

void sched_process_pending_ai_suggestions(void) {}

int mon_vm_send_map(uint64_t aspace_id, uint64_t vaddr, uint64_t paddr, uint64_t size, uint32_t flags) {
    (void)aspace_id; (void)vaddr; (void)paddr; (void)size; (void)flags;
    return 0;
}

int mon_vm_send_unmap(uint64_t aspace_id, uint64_t vaddr, uint64_t size) {
    (void)aspace_id; (void)vaddr; (void)size;
    return 0;
}

int mon_vm_send_protect(uint64_t aspace_id, uint64_t vaddr, uint64_t size, uint32_t new_flags) {
    (void)aspace_id; (void)vaddr; (void)size; (void)new_flags;
    return 0;
}

int ipc_profile_select_transport(uint32_t service_id) {
    (void)service_id;
    return 0;
}

int ipc_profile_payload_supported(uint32_t transport_id, uint32_t payload_size) {
    (void)transport_id; (void)payload_size;
    return 1;
}

/* AI Scheduler Stubs */

int sched_ai_apply_suggestion(const ai_suggestion_t* suggestion) {
    (void)suggestion;
    return 0;
}

/* Demo App Stubs (Temporary until moved to user-space) */
void kernel_tester_app(void) {}
