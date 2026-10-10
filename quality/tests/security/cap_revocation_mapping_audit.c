/* CAP-P0-004: real capability/VMM/DMA control paths; fake hardware only.
 * Each scenario runs in a fresh process. Capability locks and revoke are
 * unchanged. HAL translation and the separate grant's generic IOMMU boundary
 * are fake; this does not exercise real CPU/device translation.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "capability.h"
#include "mm/aspace.h"
#include "mm/dma.h"
#include "mm/dma_grant.h"
#include "mm/mem_validator.h"
#include "hal/hal_mpa.h"
#include "hal/hal_dma.h"
#include "hal/iommu.h"
#include "time/ktime.h"
#include "../../../core/kernel/src/cap/cap_internal.h"

static capability_table_t tables[2];
static uint32_t cpu;
static unsigned revoke_acks, cpu_unmaps, flushes, dma_unmaps, iommu_unmaps;
static bool cpu_mapping, device_mapping;
static int injected_unmap_error;
bool g_pmm_initialized = true;

uint32_t hal_cpu_get_id(void) { return cpu; }
void arch_cpu_relax(void) {}
void kernel_panic(const char *message) { fprintf(stderr, "%s\n", message); abort(); }
void console_log(int level, const char *format, ...) { (void)level; (void)format; }
bh_kdeadline_t bh_deadline_after_ns(uint64_t ns) { return ns; }
bool bh_deadline_expired(bh_kdeadline_t deadline) { (void)deadline; return false; }
capability_table_t *sched_current_cap_table(void) { return &tables[cpu]; }
bh_process_t *sched_current_process(void) { return NULL; }

urpc_channel_state_t urpc_channel_get_state(uint32_t core) {
    return core < 2 ? URPC_CHANNEL_BOUND : URPC_CHANNEL_CLOSED;
}
int urpc_bootstrap_send(uint32_t destination, uint64_t message) {
    /* Deterministic owner dispatch, not a concurrent/SMP transport test. */
    assert(tables[0].lock.locked.value == 0);
    assert(tables[1].lock.locked.value == 0);
    urpc_msg_type_t type;
    uint64_t payload;
    urpc_unpack_msg(message, &type, &payload);
    uint32_t source = cpu;
    cpu = destination;
    if (type == URPC_CAP_REVOKE) cap_handle_revoke_req(payload, source);
    else if (type == URPC_CAP_REVOKE_ACK) {
        revoke_acks++;
        cap_handle_revoke_ack(payload);
    } else if (type == URPC_CAP_DELEGATE_REQ) cap_handle_delegate_req(payload, source);
    else if (type == URPC_CAP_DELEGATE_ACK) cap_handle_delegate_ack(payload);
    else abort();
    cpu = source;
    return 0;
}

static void init_tables(void) {
    for (unsigned i = 0; i < 2; i++) {
        tables[i].owner_core = i;
        tables[i].cspace_id = cap_cspace_id_make(i, 0, 1);
        tables[i].registry_slot = 0;
        spin_lock_init(&tables[i].lock);
        assert(bh_id_allocator_init(&tables[i].id_allocator, tables[i].id_bitmap,
                                   BHARAT_ARRAY_SIZE(tables[i].entries)) == K_OK);
        g_cap_cspace_registry[i][0].table = &tables[i];
    }
}

static int map_page(phys_addr_t root, virt_addr_t va, phys_addr_t pa, uint32_t flags) {
    (void)root; (void)va; (void)pa; (void)flags;
    cpu_mapping = true;
    return 0;
}
static int unmap_page(phys_addr_t root, virt_addr_t va, phys_addr_t *pa) {
    (void)root; (void)va; *pa = 0x4000;
    cpu_mapping = false; cpu_unmaps++;
    return 0;
}
static void flush_local(virt_addr_t va, uint16_t asid) {
    (void)va; (void)asid; flushes++;
}
static mem_protect_ops_t mpa = {.cpu_ops = {
    .map_page = map_page, .unmap_page = unmap_page, .flush_tlb_local = flush_local}};
mem_protect_ops_t *active_mem_protect = &mpa;
mem_model_t mm_get_validated_model(void) { return MEM_MODEL_MMU_FULL; }
int hal_mem_get_caps(hal_mem_caps_t *caps) { memset(caps, 0, sizeof(*caps)); return 0; }
vm_region_t *aspace_lookup_region(address_space_t *as, virt_addr_t va) {
    (void)as; (void)va; return NULL;
}

int hal_dma_needs_sync(void) { return 0; }
void hal_dma_sync_for_device(hal_dma_buffer_t *buffer) { (void)buffer; }
void hal_dma_sync_for_cpu(hal_dma_buffer_t *buffer) { (void)buffer; }
int hal_dma_map_buffer(hal_dma_buffer_t *buffer) { (void)buffer; return 0; }
void hal_dma_unmap_buffer(hal_dma_buffer_t *buffer) { (void)buffer; dma_unmaps++; }
int hal_iommu_map(hal_iommu_domain_t *domain, uint64_t iova, uint64_t pa,
                  size_t length, uint64_t prot) {
    (void)domain; (void)iova; (void)pa; (void)length; (void)prot;
    device_mapping = true; return 0;
}
int hal_iommu_unmap(hal_iommu_domain_t *domain, uint64_t iova, size_t length) {
    (void)domain; (void)iova; (void)length; iommu_unmaps++;
    if (injected_unmap_error) return injected_unmap_error;
    device_mapping = false; return 0;
}

/* Separate grant API fault injection: this backend deliberately accepts NULL
 * domains so an ACTIVE grant can be reached. Actual iommu_map rejects NULL.
 * Therefore this scenario is an API-contract failure, not device reachability.
 */
bool iommu_available(void) { return true; }
kstatus_t iommu_map(bh_iommu_domain_t *domain, uintptr_t iova, uintptr_t pa,
                    size_t length, uint32_t flags) {
    (void)domain; (void)iova; (void)pa; (void)length; (void)flags;
    device_mapping = true; return K_OK;
}
kstatus_t iommu_unmap(bh_iommu_domain_t *domain, uintptr_t iova, size_t length) {
    (void)domain; (void)iova; (void)length; iommu_unmaps++;
    if (injected_unmap_error) return injected_unmap_error;
    device_mapping = false; return K_OK;
}

static uint32_t grant(cap_type_t type, uint64_t object, uint64_t rights) {
    uint32_t handle;
    assert(cap_table_grant(&tables[cpu], type, object, rights, &handle) == 0);
    return handle;
}

static int stale(void) {
    uint32_t parent = grant(CAP_TYPE_MEMORY, 0x4000,
                           CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_DELEGATE);
    uint32_t child;
    assert(cap_table_delegate(&tables[0], &tables[0], parent,
                             CAP_RIGHT_MEMORY_MAP, &child) == 0);
    bh_memory_object_t memory;
    assert(cap_lookup_memory(&tables[0], child, CAP_RIGHT_MEMORY_MAP, &memory) == K_OK);
    assert(cap_table_revoke(&tables[0], parent) == 0);
    assert(cap_lookup_memory(&tables[0], child, CAP_RIGHT_MEMORY_MAP, &memory) != K_OK);
    assert(cap_lookup_memory(&tables[0], parent, CAP_RIGHT_MEMORY_MAP, &memory) != K_OK);
    uint32_t replacement = 0;
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(tables[0].entries); i++) {
        replacement = grant(CAP_TYPE_MEMORY, 0x9000, CAP_RIGHT_MEMORY_MAP);
        if (bh_cap_index(parent) == bh_cap_index(replacement)) break;
    }
    assert(bh_cap_index(parent) == bh_cap_index(replacement));
    assert(parent != replacement);
    assert(cap_lookup_memory(&tables[0], parent, CAP_RIGHT_MEMORY_MAP, &memory) == K_ERR_CAP_STALE);
    assert(cap_lookup_memory(&tables[0], replacement, CAP_RIGHT_MEMORY_MAP, &memory) == K_OK);
    assert(memory.base == 0x9000);
    assert(cap_table_revoke(&tables[0], parent) != 0);
    assert(cap_lookup_memory(&tables[0], replacement, CAP_RIGHT_MEMORY_MAP, &memory) == K_OK);
    puts("PASS: stale parent/child and reused-slot authority denied");
    return 0;
}

static int remote_stale(void) {
    uint32_t parent = grant(CAP_TYPE_MEMORY, 0x4000,
                           CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_DELEGATE);
    uint32_t child;
    assert(cap_table_delegate(&tables[0], &tables[1], parent,
                             CAP_RIGHT_MEMORY_MAP, &child) == 0);
    bh_memory_object_t memory;
    cpu = 1;
    assert(cap_lookup_memory(&tables[1], child, CAP_RIGHT_MEMORY_MAP, &memory) == K_OK);
    cpu = 0;
    assert(cap_table_revoke(&tables[0], parent) == 0);
    assert(revoke_acks == 1);
    cpu = 1;
    assert(cap_lookup_memory(&tables[1], child, CAP_RIGHT_MEMORY_MAP, &memory) != K_OK);
    puts("PASS: immediate remote child denied after owner ACK");
    return 0;
}

static int cpu_lifetime(bool in_flight) {
    uint32_t handle = grant(CAP_TYPE_MEMORY, 0x4000,
                           CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_MEMORY_UNMAP);
    bh_memory_object_t memory;
    address_space_t space = {0};
    space.root_pt = 1;
    assert(cap_lookup_memory(&tables[0], handle, CAP_RIGHT_MEMORY_MAP, &memory) == K_OK);
    if (!in_flight) assert(mm_vmm_map_page(&space, 0x10000, memory.base, PAGE_USER) == 0);
    unsigned before = flushes;
    assert(cap_table_revoke(&tables[0], handle) == 0);
    if (in_flight) assert(mm_vmm_map_page(&space, 0x10000, memory.base, PAGE_USER) == 0);
    assert(cap_lookup_memory(&tables[0], handle, CAP_RIGHT_MEMORY_MAP, &memory) != K_OK);
    bool unsafe = cpu_mapping;
    printf("CPU mapping=%d unmaps=%u flushes_during_revoke=%u acks=%u\n",
           cpu_mapping, cpu_unmaps, flushes - before - (in_flight ? 1 : 0), revoke_acks);
    /* Positive control: explicit VMM unmap reaches the same fake backend. */
    assert(mm_vmm_unmap_page(&space, 0x10000) == 0);
    assert(!cpu_mapping && cpu_unmaps == 1);
    return unsafe ? 1 : 0;
}

static int dma_lifetime(void) {
    uint8_t page[4096];
    iova_domain_t domain = {.iommu_hw_state = (void *)1};
    dma_buffer_t buffer = {.cpu_addr = page, .phys_addr = 0x4000,
        .iova = 0x8000, .size = sizeof(page), .pin_count = 1, .domain = &domain};
    uint32_t handle = grant(CAP_TYPE_DMA_GRANT, (uint64_t)(uintptr_t)&buffer,
                           CAP_RIGHT_DMA_MAP | CAP_RIGHT_MEMORY_UNMAP);
    assert(dma_buffer_map_device(handle, &buffer, DMA_MAP_TO_DEVICE) == 0);
    assert(device_mapping && buffer.mapped_to_device);
    assert(cap_table_revoke(&tables[0], handle) == 0);
    assert(dma_buffer_map_device(handle, &buffer, DMA_MAP_TO_DEVICE) == -100);
    assert(dma_buffer_unmap_device(handle, &buffer, DMA_MAP_TO_DEVICE) == -100);
    bool unsafe = device_mapping && buffer.mapped_to_device;
    printf("DMA mapping=%d owned=%d unmaps=%u iommu_unmaps=%u acks=%u\n",
           device_mapping, buffer.owned_by_device, dma_unmaps, iommu_unmaps, revoke_acks);
    /* An independently authorized owner can still perform actual teardown. */
    uint32_t cleanup = grant(CAP_TYPE_DMA_GRANT, (uint64_t)(uintptr_t)&buffer,
                            CAP_RIGHT_MEMORY_UNMAP);
    assert(dma_buffer_unmap_device(cleanup, &buffer, DMA_MAP_TO_DEVICE) == 0);
    assert(!device_mapping && !buffer.mapped_to_device && iommu_unmaps == 1);
    return unsafe ? 1 : 0;
}

static int grant_failure(void) {
    bh_dma_grant_create_args_t args = {.owner_id = 1, .device_id = 2,
        .paddr = 0x4000, .iova = 0x8000, .length = 4096};
    bh_dma_grant_id_t id;
    bh_dma_grant_state_t state;
    assert(bh_dma_grant_create(&args, &id) == K_OK);
    assert(bh_dma_grant_map(id) == K_OK);
    assert(bh_dma_grant_activate(id) == K_OK);
    injected_unmap_error = K_ERR_TIMEOUT;
    kstatus_t status = bh_dma_grant_revoke(id, 0);
    assert(bh_dma_grant_get_state(id, &state) == K_OK);
    printf("grant revoke=%d state=%d device_mapping=%d unmap_calls=%u\n",
           status, state, device_mapping, iommu_unmaps);
    return status == K_OK && state == BH_DMA_GRANT_REVOKED && device_mapping ? 1 : 0;
}

int main(int argc, char **argv) {
    assert(argc == 2);
    init_tables();
    if (!strcmp(argv[1], "stale")) return stale();
    if (!strcmp(argv[1], "remote-stale")) return remote_stale();
    if (!strcmp(argv[1], "cpu")) return cpu_lifetime(false);
    if (!strcmp(argv[1], "inflight")) return cpu_lifetime(true);
    if (!strcmp(argv[1], "dma")) return dma_lifetime();
    if (!strcmp(argv[1], "grant-failure")) return grant_failure();
    return 2;
}
