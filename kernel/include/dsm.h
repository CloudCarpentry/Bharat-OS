#ifndef BHARAT_DSM_H
#define BHARAT_DSM_H

#include "mm.h"
#include "rdma.h"
#include "cluster_bus.h"

/*
 * Bharat-OS Distributed Shared Memory (DSM)
 * Transforms multiple separate servers into a Single System Image (SSI)
 * where virtual memory pages can span across the network transparently using RDMA or CXL.
 */

typedef struct {
    uint32_t cluster_id;
    
    // The distributed virtual address space base accessible by all nodes
    virt_addr_t global_vaddr_start;
    uint64_t size_bytes;
    
    // RDMA or CXL handles backing the shared pages
    rdma_queue_pair_t* sync_qp;
    cluster_node_t* cxl_node;
} dsm_region_t;

// Create a globally accessible memory region mapping physical RAM across nodes
int dsm_create_region(uint64_t size, dsm_region_t* out_region);

// Handle page-faults on a Distributed Shared Memory boundary (fetch page via RDMA)
int dsm_handle_page_fault(dsm_region_t* region, virt_addr_t faulted_address);

// Synchronize memory barriers across the cluster for cache coherence
void dsm_cluster_barrier(dsm_region_t* region);

#endif // BHARAT_DSM_H
