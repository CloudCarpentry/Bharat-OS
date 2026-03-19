# services/storagemgr

## Purpose
A higher-level policy and orchestration service for block and file storage.

## Responsibilities
- **Block Queue Policy**: Managing block device access requests.
- **Filesystem Mounts**: Coordinating namespace mounts.
- **Replication / Durability Policy**: Orchestrating storage durability and mirroring.
- **Tiered Storage**: Managing policies across NVMe, SSD, and slower block storage.
- **NVMe Queue Distribution**: Distributing commands to hardware queues optimally.
- **Recovery Orchestration**: Coordinating fault recovery protocols on storage elements.

## Dependencies
- **May depend on:** `lib/runtime`, `lib/ipc`, standard C library headers.
- **Must not depend on:** `subsys/*`, `ui/*`, direct kernel-private headers.

## Immediate TODOs
- Expose basic mount capabilities.
- Define internal policies for simple RAM/flash block queues.

## Status
Stub.
