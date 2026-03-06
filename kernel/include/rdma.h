#ifndef BHARAT_RDMA_H
#define BHARAT_RDMA_H

#include <stdint.h>
#include "mm.h"
#include "io_subsys.h"

/*
 * Bharat-OS High-Speed Data Movement & RDMA Layer
 * Provides zero-copy data transfer bypassing CPU processing, sending data 
 * directly from memory to SmartNICs/DPUs or remote servers.
 */

// Memory Region descriptor registered with the Network Interface
typedef struct {
    void* local_vaddr;
    phys_addr_t local_paddr;
    uint32_t length;
    uint32_t security_lkey; // Local access key
    uint32_t security_rkey; // Remote access key for RDMA
} rdma_memory_region_t;

// Queue Pair containing Send and Receive queues
typedef struct {
    uint32_t qp_number;
    
    // Ring queues conceptually similar to io_uring
    io_ring_t sq; // Send Queue
    io_ring_t rq; // Receive Queue
} rdma_queue_pair_t;

// Register a memory region with the hardware to enable Zero-Copy transfers
int rdma_register_memory(void* vaddr, uint32_t len, rdma_memory_region_t* out_mr);

// Post a Send work request directly pointing to the memory region
int rdma_post_send(rdma_queue_pair_t* qp, rdma_memory_region_t* mr, uint32_t offset, uint32_t len);

// Post an RDMA Read Work Request (Read memory from a remote server)
int rdma_post_read(rdma_queue_pair_t* local_qp, rdma_memory_region_t* local_mr, 
                  uint64_t remote_addr, uint32_t remote_rkey, uint32_t len);

// Post an RDMA Write Work Request (Write memory directly to remote server RAM)
int rdma_post_write(rdma_queue_pair_t* local_qp, rdma_memory_region_t* local_mr, 
                   uint64_t remote_addr, uint32_t remote_rkey, uint32_t len);

#endif // BHARAT_RDMA_H
