# Multiprocessor and NUMA Baseline (v1)

This document captures the current multiprocessor/NUMA baseline for Bharat-OS.

## Implemented baseline

- Secondary-core boot hook:
  - `mk_boot_secondary_cores(core_count)` invokes platform multicore boot path.
  - On RISC-V builds, `multicore_boot_secondary_cores` uses OpenSBI `sbi_send_ipi` to wake secondary HARTs.
- Per-core URPC channel matrix:
  - `mk_init_per_core_channels(core_count, ring_size)` builds sender->receiver channels.
  - `mk_get_channel(sender, receiver)` retrieves typed channel handles.
- Lockless URPC robustness:
  - strict ring validation (non-null buffer, min capacity),
  - typed errors for invalid/full/empty/no-channel,
  - wrap-around-safe ring indexing.
- Scalable message allocator baseline:
  - core-local message pool (`mk_msg_pool_t`) with alloc/free APIs for URPC producers.
- NUMA descriptors:
  - `numa_node_descriptor_t` with node memory range and CPU counts,
  - APIs for set/get descriptors and active-node counting.

## Deferred for production

- Real AP startup sequencing (x86 SIPI/SIPI) and per-hart handshake barriers.
- Dynamic SRAT/FDT NUMA topology parsing and distance matrix population.
- Per-node scheduler queues and memory-placement policies.
- Cross-node URPC transport tuning and backpressure controls.
