# File System Service (`services/file_system`)

## Overview
The `file_system` service is responsible for providing a unified namespace and persistent storage management for the Bharat-OS microkernel. It acts as the user-space Virtual File System (VFS) daemon, routing IPC and URPC requests (such as `open()`, `read()`, `write()`, `close()`) from applications and compatibility personalities to underlying storage backends (e.g., FAT, littlefs, or block devices).

This persistent storage layer is crucial for saving state, managing logs, and supporting critical functionality like Over-The-Air (OTA) updates on IoT, edge devices, and richer OS profiles.

## Responsibilities
- **Namespace Management**: Mount and manage a hierarchical view of underlying filesystems and storage drivers.
- **Request Dispatch**: Handle incoming capability-checked VFS operations over IPC/URPC.
- **Backend Abstraction**: Map unified filesystem requests to concrete block drivers or raw blob storage.
- **Storage Profiles**: Support small, deterministic backends (e.g., `littlefs`) for the RTOS/Edge profiles, and richer, scalable backends for desktop/cloud environments.
- **OTA Recovery Storage**: Ensure robust fallback, dual-bank, or A/B storage abstractions for fail-safe updates.

## Current Status
- **Status**: Scaffold / Stub
- The core headers (`kernel/include/fs/vfs.h`) provide the capability-aware VFS structure.
- `services/file_system/main.c` currently starts and calls an empty initialization stub (`vfs_init()`).
- No concrete filesystem drivers (like FAT or littlefs) or block storage integration are implemented yet.

## Planned API & Dependencies
- **Dependencies**:
  - `lib/urpc` for high-throughput, asynchronous IO.
  - `subsys_manager` for integration with POSIX and other personalities.
  - `drivers/storage` (Planned) for block-device primitives.
- **Forbidden Dependencies**: The `file_system` must not directly interface with hardware MMIO; it should rely on capability-mediated URPC messages to separate driver instances.

## Roadmap
1. Implement in-memory `ramfs` or `tmpfs` for early boot support.
2. Integrate a small footprint persistent filesystem (like `littlefs` or `FAT32`) for Edge and IoT profiles.
3. Bridge `services/file_system` with block device IPC (e.g., to an NVMe or eMMC user-space driver).
4. Introduce directory caching and distributed VFS resolution for multikernel environments.
