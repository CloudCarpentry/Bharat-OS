#ifndef BHARAT_BFS_H
#define BHARAT_BFS_H

#include "../../fs/vfs.h"
#include <stdint.h>


/*
 * Bharat-OS Native File System (BFS)
 * [EXPERIMENTAL] — Not part of the v1 kernel core. Deferred to a later release.
 * ZFS/Btrfs equivalent next-generation storage supporting CoW, snapshots,
 * atomic transactions, and zero-knowledge native XTS-AES encryption.
 */

// BFS B-Tree Node Pointer representing a logical block (CoW structure)
typedef struct {
  uint64_t logical_block_a; // Primary mirror
  uint64_t logical_block_b; // Redundant mirror
  uint32_t checksum_blake3; // Cryptographic hash for bit-rot detection

  // Compression parameters (Zstd)
  uint8_t compression_algo;
  uint32_t compressed_size;
  uint32_t uncompressed_size;
} bfs_block_ptr_t;

// Capability Key for XTS-AES encryption
typedef struct {
  uint8_t key[32]; // 256-bit AES
  uint8_t tweak[16];
} bfs_crypto_key_t;

// Atomic snapshot representation
typedef struct {
  uint64_t snapshot_id;
  uint64_t root_btree_node; // The frozen root of the filesystem tree
  uint64_t timestamp;
} bfs_snapshot_t;

// Initialize a new BFS volume on a raw block device
int bfs_format_volume(vfs_node_t *block_device);

// Take an instant, zero-cost atomic snapshot of the BFS subvolume
int bfs_create_snapshot(const char *subvolume_path,
                        bfs_snapshot_t *out_snapshot);

// Enable XTS-AES zero-knowledge encryption on a directory bounds
int bfs_enable_encryption(const char *path, bfs_crypto_key_t *master_key);

// Enable transparent inline block deduplication
int bfs_enable_dedup(const char *pool_name);

#endif // BHARAT_BFS_H
