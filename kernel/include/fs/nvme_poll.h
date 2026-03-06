#ifndef BHARAT_NVME_H
#define BHARAT_NVME_H

#include <stdint.h>
#include "../mm.h"

/*
 * Bharat-OS High-Performance NVMe Polling Interface
 * Bypasses standard kernel interrupt handling and VFS overheads to achieve 
 * millions of IOPS per CPU core for AI/DB workloads (Inspired by Intel SPDK).
 */

// NVMe Submission Queue Entry (64 bytes)
typedef struct {
    uint32_t dword0;
    uint32_t nsid;
    uint64_t reserved;
    uint64_t metadata_ptr;
    
    // Data Pointers (PRP or SGL)
    phys_addr_t data_ptr1;
    phys_addr_t data_ptr2;
    
    uint32_t command_specific[6];
} nvme_sq_entry_t;

// NVMe Completion Queue Entry (16 bytes)
typedef struct {
    uint32_t command_specific;
    uint32_t reserved;
    uint16_t sq_head;
    uint16_t sq_id;
    uint16_t command_id;
    uint16_t status;
} nvme_cq_entry_t;

typedef struct {
    nvme_sq_entry_t* sq_base;
    nvme_cq_entry_t* cq_base;
    uint32_t q_depth;
    
    // Hardware Doorbell physical addresses for user-space direct rings
    uint32_t* sq_tail_doorbell;
    uint32_t* cq_head_doorbell;
} nvme_qp_t;

// Allocate a user-space polling queue pair dedicated to a single thread
int nvme_alloc_polling_queue(uint32_t controller_id, nvme_qp_t* out_qp);

// Submit raw asynchronous NVMe block reads directly to the hardware doorbell
int nvme_submit_read(nvme_qp_t* qp, phys_addr_t dest, uint64_t lba, uint32_t block_count);

// Poll the Completion Queue directly from User-Space (Loop tightly until IO completes)
int nvme_poll_completions(nvme_qp_t* qp, uint32_t max_completions);

#endif // BHARAT_NVME_H
