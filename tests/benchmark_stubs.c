#include "../kernel/include/sched/sched.h"
#include "../kernel/include/slab.h"
#include "../kernel/include/arch/arch_ext_state.h"
#include <stdlib.h>
#include <string.h>

// Mock for kernel_panic for missing symbols in pmm
__attribute__((weak)) void kernel_panic(const char* msg) {
    (void)msg;
    abort();
}

// Mock for test_device_dma_dump missing from boot tests
__attribute__((weak)) void test_device_dma_dump(void) {
}

// Using weak linking so that tests that include actual mm code won't complain about multiple definitions
__attribute__((weak)) address_space_t* mm_create_address_space(void) {
    static address_space_t g_as = { .root_pt = 0x1000U };
    return &g_as;
}

__attribute__((weak)) void hal_core_notify(uint32_t target_core, uint64_t payload_or_reason) {
    (void)target_core;
    (void)payload_or_reason;
}

phys_addr_t __attribute__((weak)) mm_alloc_page(uint32_t preferred_numa_node) {
    (void)preferred_numa_node;
    return 0;
}

#include "../kernel/include/capability.h"

__attribute__((weak)) void cap_table_destroy(capability_table_t* table) {
    (void)table;
}

void __attribute__((weak)) mm_free_page(phys_addr_t page) {
    (void)page;
}

kcache_t* kcache_create(const char* name, size_t size) {
    kcache_t* c = malloc(sizeof(kcache_t));
    if(c) {
        c->object_size = size;
        c->name = name;
    }
    return c;
}

void* kcache_alloc(kcache_t* cache) {
    if(!cache) return NULL;
    return malloc(cache->object_size);
}

void kcache_free(kcache_t* cache, void* obj) {
    (void)cache;
    (void)obj;
}

__attribute__((weak)) uint32_t hal_cpu_get_id(void) {
    return 0;
}

__attribute__((weak)) void hal_cpu_halt(void) {
}

void hal_vmm_switch_address_space(address_space_t* as) {
    (void)as;
}

void hal_fpu_disable(void) {
}

void hal_fpu_enable(void) {
}

void hal_fpu_save_state(void* kthread) {
    (void)kthread;
}

void hal_fpu_restore_state(void* kthread) {
    (void)kthread;
}

void hal_fpu_init_thread_state(void* kthread) {
    (void)kthread;
}

#include "../kernel/include/capability.h"
#include "../kernel/include/bharat/cpu_local.h"
#include "../kernel/include/mm/mm_remote.h"
#include "../kernel/include/bharat/urpc.h"

cpu_local_t g_cpu_locals[MAX_CPUS];
__attribute__((weak)) void vmm_process_urpc_messages(void) {}

urpc_channel_state_t __attribute__((weak)) urpc_channel_get_state(uint32_t target_core) { return URPC_CHANNEL_CLOSED; }
int __attribute__((weak)) urpc_bootstrap_send(uint32_t target_core, uint64_t msg) { return -1; }
int __attribute__((weak)) urpc_bootstrap_recv(uint32_t source_core, uint64_t* out_msg) { return -1; }

__attribute__((weak)) void mm_remote_tlb_flush(uint32_t target_core, uint64_t as_id, virt_addr_t va) {
    (void)target_core;
    (void)as_id;
    (void)va;
}

mm_mailbox_slot_t __attribute__((weak)) g_mm_mailboxes[64];

uint64_t __attribute__((weak)) hal_timer_monotonic_ticks(void) { return 0; }


// Fallback legacy VMM bindings for tests that bypass active_mmu
int __attribute__((weak)) hal_vmm_get_mapping(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t* paddr, uint32_t* flags) {
    (void)root_table; (void)vaddr; (void)paddr; (void)flags;
    return -1;
}

int __attribute__((weak)) vmm_map_device_mmio(virt_addr_t vaddr, phys_addr_t paddr, capability_t *cap, int rw) {
    (void)vaddr; (void)paddr; (void)cap; (void)rw;
    return -1;
}
int __attribute__((weak)) hal_vmm_update_mapping(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    (void)root_table; (void)vaddr; (void)paddr; (void)flags; return -1;
}

void __attribute__((weak)) arch_cpu_relax(void) {
}

__attribute__((weak)) void hal_serial_write(const char *s) { (void)s; }
__attribute__((weak)) void hal_serial_write_hex(uint64_t val) { (void)val; }
__attribute__((weak)) void hal_cpu_dump_trap_frame(const void *trap_frame) { (void)trap_frame; }
__attribute__((weak)) void hal_cpu_dump_state(void) {}
__attribute__((weak)) void hal_cpu_disable_interrupts(void) {}
__attribute__((weak)) void hal_cpu_reboot(void) {}

// kmalloc and kfree for testing
void* __attribute__((weak)) kmalloc(size_t size) {
    return malloc(size);
}
void __attribute__((weak)) kfree(void* ptr) {
    free(ptr);
}

// Stubs for numa.c dependencies in tests
#include "../kernel/include/hal/hal_pt.h"
hal_pt_ops_t* active_hal_pt = NULL;

void __attribute__((weak)) tlb_shootdown(address_space_t *as, virt_addr_t vaddr) {
    (void)as;
    (void)vaddr;
}

__attribute__((weak)) void mm_inc_page_ref(phys_addr_t page) { (void)page; }
__attribute__((weak)) phys_addr_t mm_alloc_pages_order(int order, uint32_t preferred_numa_node, uint32_t flags) { (void)order; (void)preferred_numa_node; (void)flags; return 0; }
__attribute__((weak)) void hal_tlb_flush(unsigned long long vaddr) {
    (void)vaddr;
}

__attribute__((weak)) void hal_send_ipi_payload(uint32_t target_core, uint64_t payload) {
    (void)target_core;
    (void)payload;
}

__attribute__((weak)) int hal_timer_source_init(uint64_t period_ns) {
    (void)period_ns;
    return 0;
}

__attribute__((weak)) uint64_t _hal_timer_monotonic_ticks_stub(void) {
    return 0;
}

__attribute__((weak)) void arch_prepare_initial_context(cpu_context_t* ctx, void (*entry)(void), uint64_t stack_top) {
    (void)ctx;
    (void)entry;
    (void)stack_top;
}

__attribute__((weak)) void arch_context_switch(cpu_context_t* prev, cpu_context_t* next) {
    (void)prev;
    (void)next;
}

__attribute__((weak)) int arch_ext_state_thread_init(kthread_t *t) {
    (void)t;
    return 0;
}

int __attribute__((weak)) vmm_handle_cow_fault(address_space_t* as, virt_addr_t vaddr) {
    (void)as;
    (void)vaddr;
    return 0;
}

__attribute__((weak)) void arch_ext_state_thread_destroy(kthread_t *t) {
    (void)t;
}

__attribute__((weak)) void arch_ext_state_save(kthread_t *t) {
    (void)t;
}

__attribute__((weak)) void arch_ext_state_restore(kthread_t *t) {
    (void)t;
}

__attribute__((weak)) void sched_thread_exit_trampoline(void) {
}

__attribute__((weak)) uint64_t sched_get_ticks(void) { return 0; }

__attribute__((weak)) void mm_switch_active_aspace(address_space_t *aspace) {
    (void)aspace;
}

__attribute__((weak)) void vmm_process_local_urpc_messages(uint32_t core_id) {
    (void)core_id;
}

__attribute__((weak)) void* physmap_phys_to_virt(phys_addr_t phys) {
    return (void*)phys;
}

__attribute__((weak)) int cap_can_transfer(uint32_t type) {
    (void)type;
    return 1;
}

__attribute__((weak)) int cap_transfer_rights_valid(uint32_t current_rights, uint32_t requested_rights) {
    (void)current_rights;
    (void)requested_rights;
    return 1;
}

__attribute__((weak)) struct capability_table* sched_current_cap_table(void) {
    return NULL;
}

#include <stdbool.h>

__attribute__((weak)) bool arch_cpu_has(int feature) {
    (void)feature;
    return false;
}

// Ensure these functions don't recurse through lib/base/string.c
__attribute__((weak)) void arch_memset_raw(void *dst, int val, size_t len) {
    volatile unsigned char *d = (volatile unsigned char *)dst;
    unsigned char v = (unsigned char)val;
    for (size_t i = 0; i < len; i++) {
        d[i] = v;
    }
}

__attribute__((weak)) void* arch_memcpy(void* dest, const void* src, size_t n, uint32_t flags) { (void)flags; return __builtin_memcpy(dest, src, n); }
__attribute__((weak)) void* arch_memset(void* s, int c, size_t n, uint32_t flags) { (void)flags; arch_memset_raw(s, c, n); return s; }
__attribute__((weak)) void* arch_memmove(void* dest, const void* src, size_t n, uint32_t flags) { (void)flags; return __builtin_memmove(dest, src, n); }


__attribute__((weak)) int console_current_phase(void) { return 1; }
__attribute__((weak)) int aspace_destroy(address_space_t *aspace) { (void)aspace; return 0; }
__attribute__((weak)) void vm_debug_validate_active_tracking(void) {}
__attribute__((weak)) int boot_selftest_run_stage(int stage) { (void)stage; return 0; }
__attribute__((weak)) int bharat_boot_mode_select(void) { return 0; }
__attribute__((weak)) const char* bharat_boot_mode_name(int mode) { (void)mode; return "NORMAL"; }


__attribute__((weak)) void trap_dispatch_syscall(trap_frame_t* frame) {
    (void)frame;
}

__attribute__((weak)) void trap_handle_fault(trap_frame_t* frame) {
    (void)frame;
}


__attribute__((weak)) int vmm_map_page(virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    (void)vaddr; (void)paddr; (void)flags;
    return 0;
}

__attribute__((weak)) int vmm_unmap_page(virt_addr_t vaddr) {
    (void)vaddr;
    return 0;
}

__attribute__((weak)) int vmm_init(void) {
    return 0;
}

__attribute__((weak)) personality_ops_t default_personality_ops = {0};

__attribute__((weak)) void hal_cpu_enable_interrupts(void) {}
__attribute__((weak)) struct rb_node* rb_first(const struct rb_root* root) { (void)root; return NULL; }
__attribute__((weak)) void rb_erase(struct rb_node* node, struct rb_root* root) { (void)node; (void)root; }
__attribute__((weak)) void rb_insert_color(struct rb_node* node, struct rb_root* root) { (void)node; (void)root; }


__attribute__((weak)) void pmm_pcache_init_all(void) {}
__attribute__((weak)) void pmm_drain_remote_frees(uint32_t core) { (void)core; }


__attribute__((weak)) void* g_pmm_cores[64] = {0}; // Add mock for pmm_core_t* array

#include "../kernel/include/arch/arch_caps.h"
__attribute__((weak)) arch_caps_t arch_get_caps(void) {
    arch_caps_t stub_caps = {0};
    return stub_caps;
}

#include "../kernel/include/arch/memops.h"
#include "../kernel/include/mm/prot_domain.h"
#include "../kernel/include/hal/hal_mm.h"
__attribute__((weak)) void hal_mm_backend_caps(hal_mm_backend_caps_t *out) {
    if (out) {
        __builtin_memset(out, 0, sizeof(hal_mm_backend_caps_t));
    }
}

__attribute__((weak)) prot_domain_t* mock_prot_domain_create(void) { return (prot_domain_t*)0x1234; }
__attribute__((weak)) prot_domain_ops_t mmu_full_ops_x86_64 = { .create = mock_prot_domain_create };
__attribute__((weak)) prot_domain_ops_t mmu_lite_ops_common = { .create = mock_prot_domain_create };
__attribute__((weak)) prot_domain_ops_t prot_none_ops = { .create = mock_prot_domain_create };
__attribute__((weak)) void* sched_find_thread_slot_by_tid_local(void* rq, uint64_t tid) { (void)rq; (void)tid; return NULL; }
