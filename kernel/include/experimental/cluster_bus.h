#ifndef BHARAT_CXL_H
#define BHARAT_CXL_H

#include "../hal/gpu.h"
#include <stdint.h>


/*
 * Bharat-OS CXL & NVLink Cluster Interconnect
 * [EXPERIMENTAL] — Not part of the v1 kernel core.
 * Provides driver capabilities to treat multiple physical GPUs/NPUs
 * acting over CXL or NVLink as a single massively-parallel compute node for AI
 * workload execution.
 */

// Interconnect Type
typedef enum {
  LINK_TYPE_PCIE_GEN5,
  LINK_TYPE_CXL_2_0,
  LINK_TYPE_CXL_3_0,
  LINK_TYPE_NVLINK_V4
} link_type_t;

// Remote Compute Node attached via interconnect
typedef struct {
  uint32_t node_id;
  link_type_t type;
  uint64_t link_bandwidth_gbps;

  // Remote exposed resources
  uint64_t remote_memory_base;
  uint64_t remote_memory_size;

  gpu_device_t *attached_accelerators;
  uint32_t accelerator_count;
} cluster_node_t;

// Discover nodes on the high-speed topology (Host/Device fabric)
int cluster_fabric_enumerate(cluster_node_t *nodes, uint32_t max_nodes);

// Map remote node accelerator memory directly into the local physical memory
// map (CXL.mem)
int cluster_map_remote_memory(cluster_node_t *node);

// Send AI/Compute kernels directly via CXL.cache
int cluster_dispatch_kernel(cluster_node_t *node, gpu_device_t *target,
                            void *kernel_buf, uint32_t size);

#endif // BHARAT_CXL_H
