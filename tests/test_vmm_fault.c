#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../../kernel/include/mm/aspace.h"
#include "../../kernel/include/mm/vm_object.h"
#include "../../kernel/include/mm/fault.h"
#include "../../kernel/include/hal/hal_pt.h"
#include "../../kernel/include/mm/tlb.h"
#include "../../kernel/include/hal/mmu_ops.h"
#include "../../kernel/include/hal/hal_tlb.h"

__attribute__((weak)) int kcache_create(const char *name, size_t obj_size, size_t align) {
    (void)name; (void)obj_size; (void)align;
    return 1;
}

__attribute__((weak)) void *kcache_alloc(int cache_id) {
    (void)cache_id;
    return malloc(256);
}

__attribute__((weak)) void kcache_free(int cache_id, void *obj) {
    (void)cache_id;
    free(obj);
}

__attribute__((weak)) prot_domain_t *prot_domain_create(void) { return (prot_domain_t *)malloc(256); }
__attribute__((weak)) void prot_domain_init(void) {}
__attribute__((weak)) phys_addr_t vmm_get_kernel_root(void) { return 0; }
__attribute__((weak)) int mm_zero_phys_range(phys_addr_t start, size_t len) { (void)start; (void)len; return 0; }

static uint8_t g_fake_page[4096];

static phys_addr_t fake_alloc_page(int node) {
    (void)node;
    return (phys_addr_t)(uintptr_t)g_fake_page;
}

static void fake_zero_page(phys_addr_t paddr, size_t size) {
    // Avoid recursive __builtin_memset loop if the compiler does something weird, just loop.
    uint8_t *dst = (uint8_t *)(uintptr_t)paddr;
    for (size_t i = 0; i < size; i++) {
        dst[i] = 0;
    }
}

__attribute__((weak)) uint16_t numa_get_current_node(void) { return 0; }
__attribute__((weak)) void *sched_current_thread(void) { return NULL; }
__attribute__((weak)) void sched_ai_apply_suggestion(void) {}
__attribute__((weak)) void hal_mm_get_zone_limits(void) {}
__attribute__((weak)) void *hal_get_system_discovery(void) { return NULL; }
__attribute__((weak)) void pt_cache_init(void) {}
__attribute__((weak)) size_t string_length(const char* s) { size_t l=0; while(s[l]) l++; return l; }
__attribute__((weak)) void console_write_raw(const char* s, size_t len) { (void)s; (void)len; }
__attribute__((weak)) int mm_vmm_map_page(address_space_t *as, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) { (void)as; (void)vaddr; (void)paddr; (void)flags; return -1; }
__attribute__((weak)) int mm_vmm_unmap_page(address_space_t *as, virt_addr_t vaddr) { (void)as; (void)vaddr; return -1; }
__attribute__((weak)) int tlb_pending_alloc(void) { return -1; }
__attribute__((weak)) void tlb_reqid_encode(void) {}
__attribute__((weak)) void tlb_pending_get_stats(void) {}
__attribute__((weak)) void bharat_monitor_v1_call_tlb_invalidate(void) {}
__attribute__((weak)) int tlb_pending_is_complete(void) { return 1; }
__attribute__((weak)) void tlb_pending_free(void) {}
__attribute__((weak)) const hal_tlb_caps_t *hal_tlb_caps(void) { return NULL; }
__attribute__((weak)) hal_tlb_ops_t *active_hal_tlb = NULL;
__attribute__((weak)) void *g_tlb_cpu_state = NULL;
__attribute__((weak)) void bharat_msg_header_decode(void) {}
__attribute__((weak)) void bharat_msg_header_encode(void) {}
__attribute__((weak)) void tlb_pending_ack(void) {}
__attribute__((weak)) void tlb_reqid_decode(void) {}

__attribute__((weak)) int tlb_invalidate_all(address_space_t *aspace, virt_addr_t addr, size_t len, uint32_t flags) {
    (void)aspace;
    (void)addr;
    (void)len;
    (void)flags;
    return 0;
}

char _pstore_start[1];
char _pstore_end[1];

#define TEST(name) void name()
#define ASSERT_EQ(a, b) assert((a) == (b))
#define ASSERT_NE(a, b) assert((a) != (b))
#define ASSERT_NOT_NULL(a) assert((a) != NULL)

extern void hal_pt_init(void);

static hal_pt_ops_t mock_pt_ops = {0};
static int map_page_called = 0;
static int map_page_result = 0;

static phys_addr_t mock_create_address_space(phys_addr_t root) {
    return root;
}

static void mock_destroy_address_space(phys_addr_t root) {
    (void)root;
}

static int mock_map_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    (void)root_pt;
    (void)vaddr;
    (void)paddr;
    (void)flags;
    map_page_called++;
    return map_page_result;
}

static hal_tlb_ops_t mock_tlb_ops = {0};

void hal_pt_init(void) {
    mock_pt_ops.create_address_space = mock_create_address_space;
    mock_pt_ops.destroy_address_space = mock_destroy_address_space;
    mock_pt_ops.map_page = mock_map_page;
    active_hal_pt = &mock_pt_ops;

    // We also mock active_hal_tlb since tlb_shootdown.c relies on it!
    active_hal_tlb = &mock_tlb_ops;
}

static int mock_tlb_invalidate_all_called = 0;
// We actually need to intercept tlb_invalidate_all which is defined in tlb_shootdown.c.
// But tlb_shootdown.c relies on tlb_invalidate_local/tlb_invalidate_remote.
// Let's rely on tlb_invalidate_local instead.
int tlb_invalidate_local(vm_aspace_t *as, uintptr_t va, size_t len, tlb_inv_kind_t kind) {
    (void)as;
    (void)va;
    (void)len;
    (void)kind;
    mock_tlb_invalidate_all_called++;
    return 0;
}

TEST(fault_invalid_event) {
    printf("Running fault_invalid_event...\n");
    ASSERT_EQ(vm_handle_fault(NULL), VM_FAULT_KILL);

    vm_fault_event_t event = {0};
    ASSERT_EQ(vm_handle_fault(&event), VM_FAULT_KILL); // no aspace
    printf("Passed fault_invalid_event\n");
}

TEST(fault_no_region_lookup) {
    printf("Running fault_no_region_lookup...\n");
    address_space_t *as = NULL;
    ASSERT_EQ(0, aspace_create(&as, 0));
    ASSERT_NOT_NULL(as);

    vm_fault_event_t event = {
        .aspace = as,
        .fault_addr = 0x1000,
        .access = VM_FAULT_READ,
        .arch_code = 0
    };

    ASSERT_EQ(vm_handle_fault(&event), VM_FAULT_KILL); // VM_FAULT_SIGSEGV converts to KILL
    aspace_destroy(as);
    printf("Passed fault_no_region_lookup\n");
}

TEST(fault_permission_denied) {
    printf("Running fault_permission_denied...\n");
    address_space_t *as = NULL;
    ASSERT_EQ(0, aspace_create(&as, 0));
    ASSERT_NOT_NULL(as);

    vm_object_t *obj = vm_object_create_anon(0x1000, 0);
    aspace_region_attach(as, 0x1000, 0x1000, CAP_RIGHT_READ, 0, VM_INHERIT_NONE, obj, 0, NULL);

    vm_fault_event_t event = {
        .aspace = as,
        .fault_addr = 0x1000,
        .access = CAP_RIGHT_WRITE, // requested write, region is read-only
        .arch_code = 0
    };

    ASSERT_EQ(vm_handle_fault(&event), VM_FAULT_KILL);

    event.access = CAP_RIGHT_EXECUTE;
    ASSERT_EQ(vm_handle_fault(&event), VM_FAULT_KILL);

    aspace_destroy(as);
    vm_object_release(obj);
    printf("Passed fault_permission_denied\n");
}

TEST(fault_object_resolution_failure) {
    printf("Running fault_object_resolution_failure...\n");
    address_space_t *as = NULL;
    ASSERT_EQ(0, aspace_create(&as, 0));
    ASSERT_NOT_NULL(as);

    vm_object_t *obj = vm_object_create_anon(0x1000, 0);
    obj->ops = NULL; // Break the object ops to trigger resolution failure
    aspace_region_attach(as, 0x1000, 0x1000, CAP_RIGHT_READ, 0, VM_INHERIT_NONE, obj, 0, NULL);

    vm_fault_event_t event = {
        .aspace = as,
        .fault_addr = 0x1000,
        .access = CAP_RIGHT_READ,
        .arch_code = 0
    };

    ASSERT_EQ(vm_handle_fault(&event), VM_FAULT_KILL); // SIGBUS -> KILL

    aspace_destroy(as);
    vm_object_release(obj);
    printf("Passed fault_object_resolution_failure\n");
}

TEST(fault_success_path) {
    printf("Running fault_success_path...\n");
    address_space_t *as = NULL;
    ASSERT_EQ(0, aspace_create(&as, 0));
    ASSERT_NOT_NULL(as);

    vm_object_t *obj = vm_object_create_anon(0x1000, 0);
    aspace_region_attach(as, 0x1000, 0x1000, CAP_RIGHT_READ | CAP_RIGHT_WRITE, 0, VM_INHERIT_NONE, obj, 0, NULL);

    vm_fault_event_t event = {
        .aspace = as,
        .fault_addr = 0x1000,
        .access = CAP_RIGHT_READ | CAP_RIGHT_WRITE,
        .arch_code = 0
    };

    map_page_called = 0;
    map_page_result = 0; // success
    mock_tlb_invalidate_all_called = 0;

    vm_fault_result_t res = vm_handle_fault(&event);
    ASSERT_EQ(res, VM_FAULT_RESOLVED);
    ASSERT_EQ(map_page_called, 1);
    ASSERT_EQ(mock_tlb_invalidate_all_called, 1);

    aspace_destroy(as);
    vm_object_release(obj);
    printf("Passed fault_success_path\n");
}

TEST(fault_backend_repair_failure) {
    printf("Running fault_backend_repair_failure...\n");
    address_space_t *as = NULL;
    ASSERT_EQ(0, aspace_create(&as, 0));
    ASSERT_NOT_NULL(as);

    vm_object_t *obj = vm_object_create_anon(0x1000, 0);
    aspace_region_attach(as, 0x1000, 0x1000, CAP_RIGHT_READ, 0, VM_INHERIT_NONE, obj, 0, NULL);

    vm_fault_event_t event = {
        .aspace = as,
        .fault_addr = 0x1000,
        .access = CAP_RIGHT_READ,
        .arch_code = 0
    };

    map_page_called = 0;
    map_page_result = -4; // ENOSYS - mock failure in hal_pt->map_page
    mock_tlb_invalidate_all_called = 0;

    ASSERT_EQ(vm_handle_fault(&event), VM_FAULT_KILL); // Returns KILL on backend failure ENOSYS
    ASSERT_EQ(map_page_called, 1);
    ASSERT_EQ(mock_tlb_invalidate_all_called, 0); // shouldn't reach TLB invalidation

    aspace_destroy(as);
    vm_object_release(obj);
    printf("Passed fault_backend_repair_failure\n");
}

int main() {
    hal_pt_init();
    extern void prot_domain_init(void);
    prot_domain_init();

    vm_object_test_set_allocators(fake_alloc_page, fake_zero_page);

    fault_invalid_event();
    fault_no_region_lookup();
    fault_permission_denied();
    fault_object_resolution_failure();
    fault_success_path();
    fault_backend_repair_failure();

    vm_object_test_reset_allocators();

    printf("All FAULT tests passed successfully!\n");
    return 0;
}
