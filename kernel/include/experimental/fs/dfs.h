#ifndef BHARAT_DFS_H
#define BHARAT_DFS_H

#include "../../fs/vfs.h"
#include "../cluster_bus.h"
#include "../rdma.h"
#include "bfs.h"


/*
 * Bharat-OS Distributed Network File System (DFS)
 * [EXPERIMENTAL] — Not part of the v1 kernel core. Deferred to a later release.
 * A globally scalable, POSIX-compliant namespace designed for Data Centers.
 * Inspired by Ceph/Lustre, it shards BFS subvolumes across network links
 * using RDMA or CXL for high-throughput, low-latency concurrent storage.
 */

// A DFS metadata server handle
typedef struct {
  uint32_t mds_id;
  rdma_queue_pair_t *sync_queue;
} dfs_mds_t;

// A DFS object storage daemon (OSD) handle
typedef struct {
  uint32_t osd_id;
  uint64_t capacity_gb;
  rdma_queue_pair_t *data_queue;
} dfs_osd_t;

// Represents a sharded file descriptor spread across multiple OSD nodes
typedef struct {
  vfs_node_t *vfs_link;
  uint64_t object_id;

  // Mapping table identifying which OSD holds which block of the file
  dfs_osd_t **stripe_map;
  uint32_t stripe_count;
} dfs_inode_t;

// Mounts a remote Ceph-like DFS cluster onto a local VFS path using RDMA
int dfs_mount_cluster(const char *mount_point, const char *mds_ip, int port);

// Request a lock on a distributed file to ensure strict POSIX consistency
int dfs_acquire_distributed_lock(dfs_inode_t *dfs_node, int exclusive);

// Stream a file block directly from a remote storage node onto a local GPU
// (GPUDirect Storage bypass)
int dfs_rdma_to_gpu_bypass(dfs_inode_t *dfs_node, uint64_t file_offset,
                           void *local_gpu_ptr, uint32_t size);

#endif // BHARAT_DFS_H
