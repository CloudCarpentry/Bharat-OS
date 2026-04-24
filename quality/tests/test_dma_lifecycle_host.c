#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mm/dma.h"
#include "mm.h"
#include "hal/hal_dma.h"
#include "hal/iommu.h"
#include "arch/arch_caps.h"
#include "capability.h"
#include "bharat/urpc.h"
#include "bharat/cpu_local.h"

static uint8_t g_page[4096];
static int g_sync_for_device_calls = 0;
static int g_sync_for_cpu_calls = 0;
static int g_hal_map_calls = 0;
static int g_hal_unmap_calls = 0;
static int g_iommu_map_calls = 0;
static int g_iommu_unmap_calls = 0;
static int g_iommu_map_fail = 0;
static int g_dma_coherent = 0;
static int g_urpc_send_calls = 0;
static int g_urpc_inval_req_received = 0;

void *kmalloc(size_t size) {
    return calloc(1, size);
}

void kfree(void *ptr) {
    free(ptr);
}

phys_addr_t mm_alloc_pages_order(int order, uint32_t preferred_numa_node, uint32_t flags) {
    (void)order;
    (void)preferred_numa_node;
    (void)flags;
    return (phys_addr_t)(uintptr_t)g_page;
}

void mm_free_page(phys_addr_t page) {
    (void)page;
}

void *physmap_phys_to_virt(phys_addr_t phys) {
    return (void *)(uintptr_t)phys;
}

int hal_iommu_map(hal_iommu_domain_t *domain, uint64_t iova, uint64_t phys, size_t size, uint64_t prot) {
    (void)domain;
    (void)iova;
    (void)phys;
    (void)size;
    (void)prot;
    g_iommu_map_calls++;
    return g_iommu_map_fail ? -77 : 0;
}

int hal_iommu_unmap(hal_iommu_domain_t *domain, uint64_t iova, size_t size) {
    (void)domain;
    (void)iova;
    (void)size;
    g_iommu_unmap_calls++;
    return 0;
}

static capability_table_t g_test_cap_table;
static bool g_use_cap_table = false;

capability_table_t* sched_current_cap_table(void) {
    return g_use_cap_table ? &g_test_cap_table : NULL;
}

uint32_t hal_cpu_get_id(void) {
    return 0;
}

urpc_channel_state_t urpc_channel_get_state(uint32_t target_core) {
    (void)target_core;
    return URPC_CHANNEL_BOUND;
}

int urpc_bootstrap_send(uint32_t target_core, uint64_t msg) {
    (void)target_core;
    (void)msg;
    g_urpc_send_calls++;

    urpc_msg_type_t type;
    uint64_t payload;
    urpc_unpack_msg(msg, &type, &payload);
    if (type == URPC_IOMMU_INVAL) {
        g_urpc_inval_req_received++;
        // Simulate ACK
        extern void iommu_handle_inval_ack(uint64_t payload);
        iommu_handle_inval_ack(payload);
    }
    return 0;
}

void vmm_process_urpc_messages(void) {
    // No-op for now as we simulate sync ACKs
}

void arch_cpu_relax(void) {
}

bool iommu_available(void) {
    return true;
}

const hal_iommu_ops_t *hal_iommu_get_ops(void) {
    return NULL;
}

cpu_local_t g_cpu_locals[MAX_CPUS];

arch_caps_t arch_get_caps(void) {
    arch_caps_t caps = {0};
    if (g_dma_coherent) {
        arch_caps_set(&caps, ARCH_CAP_DMA_COHERENT);
    }
    return caps;
}

static int test_map(hal_dma_buffer_t *buf) {
    g_hal_map_calls++;
    buf->dma_addr = buf->dma_addr ? buf->dma_addr : buf->phys_addr;
    return 0;
}

static void test_unmap(hal_dma_buffer_t *buf) {
    (void)buf;
    g_hal_unmap_calls++;
}

static void test_sync_for_device(hal_dma_buffer_t *buf) {
    (void)buf;
    g_sync_for_device_calls++;
}

static void test_sync_for_cpu(hal_dma_buffer_t *buf) {
    (void)buf;
    g_sync_for_cpu_calls++;
}

static int test_needs_sync(void) {
    return 1;
}

static void reset_counters(void) {
    g_sync_for_device_calls = 0;
    g_sync_for_cpu_calls = 0;
    g_hal_map_calls = 0;
    g_hal_unmap_calls = 0;
    g_iommu_map_calls = 0;
    g_iommu_unmap_calls = 0;
    g_iommu_map_fail = 0;
    g_urpc_send_calls = 0;
    g_urpc_inval_req_received = 0;
    g_use_cap_table = false;
    memset(&g_test_cap_table, 0, sizeof(g_test_cap_table));
}

static void test_dma_map_unmap_lifecycle_without_iommu(void) {
    reset_counters();
    g_dma_coherent = 0;

    hal_dma_ops_t ops = {
        .map = test_map,
        .unmap = test_unmap,
        .sync_for_device = test_sync_for_device,
        .sync_for_cpu = test_sync_for_cpu,
        .needs_sync = test_needs_sync,
    };
    hal_dma_register_ops(&ops);

    dma_buffer_t *buf = NULL;
    assert(dma_buffer_alloc(1024, DMA_ALLOC_ZERO, &buf) == 0);
    assert(buf != NULL);
    assert(buf->owned_by_device == false);
    assert(dma_buffer_pin(buf, 42) == 0);

    assert(dma_buffer_map_device(1, buf, DMA_MAP_BIDIRECTIONAL) == 0);
    assert(buf->mapped_to_device == true);
    assert(buf->owned_by_device == true);
    assert(buf->active_dir == DMA_MAP_BIDIRECTIONAL);
    assert(g_hal_map_calls == 1);
    assert(g_sync_for_device_calls == 1);
    assert(g_iommu_map_calls == 0);

    assert(dma_buffer_map_device(1, buf, DMA_MAP_BIDIRECTIONAL) != 0);

    assert(dma_buffer_unmap_device(1, buf, DMA_MAP_BIDIRECTIONAL) == 0);
    assert(buf->mapped_to_device == false);
    assert(buf->owned_by_device == false);
    assert(g_hal_unmap_calls == 1);
    assert(g_sync_for_cpu_calls == 1);

    assert(dma_buffer_unpin(buf) == 0);
    assert(dma_buffer_free(buf) == 0);
}

static void test_dma_capability_authorization(void) {
    reset_counters();
    g_use_cap_table = true;
    spin_lock_init(&g_test_cap_table.lock);
    g_test_cap_table.next_id = 1;

    hal_dma_ops_t ops = {
        .map = test_map,
        .unmap = test_unmap,
        .sync_for_device = test_sync_for_device,
        .sync_for_cpu = test_sync_for_cpu,
        .needs_sync = test_needs_sync,
    };
    hal_dma_register_ops(&ops);

    dma_buffer_t *buf = NULL;
    assert(dma_buffer_alloc(1024, DMA_ALLOC_ZERO, &buf) == 0);
    assert(dma_buffer_pin(buf, 42) == 0);

    // Try map without capability - should fail with -100
    assert(dma_buffer_map_device(1, buf, DMA_MAP_BIDIRECTIONAL) == -100);

    // Grant capability
    uint32_t cap_id = 0;
    assert(cap_table_grant(&g_test_cap_table, CAP_TYPE_DMA_GRANT, (uint64_t)buf, CAP_RIGHT_DMA_MAP | CAP_RIGHT_MEMORY_UNMAP, &cap_id) == 0);

    // Try map with wrong cap_id
    assert(dma_buffer_map_device(cap_id + 1, buf, DMA_MAP_BIDIRECTIONAL) == -100);

    // Try map with correct cap_id
    assert(dma_buffer_map_device(cap_id, buf, DMA_MAP_BIDIRECTIONAL) == 0);
    assert(buf->mapped_to_device == true);

    // Try unmap with correct cap_id
    assert(dma_buffer_unmap_device(cap_id, buf, DMA_MAP_BIDIRECTIONAL) == 0);
    assert(buf->mapped_to_device == false);

    assert(dma_buffer_unpin(buf) == 0);
    assert(dma_buffer_free(buf) == 0);
}

static void test_iommu_cross_core_invalidation(void) {
    reset_counters();

    iova_domain_t *iova_domain = NULL;
    assert(iova_domain_create(0x100000, 0x200000, &iova_domain) == 0);
    iommu_domain_t domain;
    domain.flags = 0;
    domain.hal_priv = NULL;

    // Simulate other cores being bound
    // In our test stub, urpc_channel_get_state returns BOUND for any core.
    // However, MAX_CPUS might be large. Let's just check if urpc_bootstrap_send is called.

    extern int iommu_invalidate_domain(iommu_domain_t * dom);
    iommu_invalidate_domain(&domain);

    // Should have sent URPC_IOMMU_INVAL to other cores (MAX_CPUS - 1)
    assert(g_urpc_send_calls > 0);
    assert(g_urpc_inval_req_received > 0);

    assert(iova_domain_destroy(iova_domain) == 0);
}

static void test_dma_iommu_map_failure_rolls_back(void) {
    reset_counters();

    hal_dma_ops_t ops = {
        .map = test_map,
        .unmap = test_unmap,
        .sync_for_device = test_sync_for_device,
        .sync_for_cpu = test_sync_for_cpu,
        .needs_sync = test_needs_sync,
    };
    hal_dma_register_ops(&ops);

    dma_buffer_t *buf = NULL;
    assert(dma_buffer_alloc(2048, DMA_ALLOC_NONE, &buf) == 0);
    assert(dma_buffer_pin(buf, 7) == 0);

    iova_domain_t *domain = NULL;
    assert(iova_domain_create(0x100000, 0x200000, &domain) == 0);
    domain->iommu = (void *)1;
    domain->iommu_hw_state = (void *)0x1234;

    assert(dma_buffer_bind_domain(buf, domain) == 0);
    g_iommu_map_fail = 1;
    assert(dma_buffer_map_device(9, buf, DMA_MAP_BIDIRECTIONAL) == -77);
    assert(buf->mapped_to_device == false);
    assert(buf->owned_by_device == false);
    assert(g_hal_map_calls == 0);

    g_iommu_map_fail = 0;
    assert(dma_buffer_map_device(9, buf, DMA_MAP_BIDIRECTIONAL) == 0);
    assert(g_iommu_map_calls >= 2);
    assert(dma_buffer_unmap_device(9, buf, DMA_MAP_BIDIRECTIONAL) == 0);
    assert(g_iommu_unmap_calls >= 1);

    assert(dma_buffer_unpin(buf) == 0);
    assert(dma_buffer_free(buf) == 0);
    assert(iova_domain_destroy(domain) == 0);
}

int main(void) {
    test_dma_map_unmap_lifecycle_without_iommu();
    test_dma_capability_authorization();
    test_iommu_cross_core_invalidation();
    test_dma_iommu_map_failure_rolls_back();
    printf("All test_dma_lifecycle_host tests passed.\n");
    return 0;
}
