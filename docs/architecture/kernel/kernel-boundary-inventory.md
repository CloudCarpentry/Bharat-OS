# Kernel Boundary Inventory

This document lists kernel modules and their classification regarding the "Minimal Kernel" boundary enforcement.

## Classification Categories
1. **KEEP_KERNEL_MECHANISM**: Core functionality required for privilege, isolation, scheduling, memory, traps, IPC, bootstrapping, or fault containment.
2. **KEEP_KERNEL_STUB_OR_HOOK**: Minimal hooks or event emitters required to support external services.
3. **MOVE_SERVICE_CORE**: Move to `core/services/core/`.
4. **MOVE_SERVICE_SYSTEM**: Move to `core/services/system/`.
5. **MOVE_SERVICE_SECURITY**: Move to `core/services/security/`.
6. **MOVE_SERVICE_DEVICE**: Move to `core/services/device/`.
7. **MOVE_DRIVER**: Move to `drivers/`.
8. **MOVE_STACK**: Move to `stacks/`.
9. **MOVE_LIB**: Move to `lib/` or `core/lib/`.
10. **MOVE_TOP_LEVEL_STAGING**: Move to root `staging/`.
11. **DELETE_OR_DEPRECATE**: Remove from the codebase.

## Inventory

| Module Path | Classification | Current/Proposed Destination | Notes |
|-------------|----------------|----------------------|-------|
| `core/kernel/src/fs/` | MOVE_SERVICE_SYSTEM | `core/services/system/filesystem/` | Keep only tiny `bootfs` in kernel. |
| `core/kernel/src/security/secure_boot.c` | KEEP_KERNEL_MECHANISM | - | Verification of boot chain. |
| `core/kernel/src/security/isolation.c` | KEEP_KERNEL_MECHANISM | - | Enforcement of domain isolation. |
| `core/kernel/src/security/audit.c` | MOVE_SERVICE_SECURITY | `core/services/security/auditd/` | Kernel keeps only event emitter hooks. |
| `core/kernel/src/security/policy.c` | MOVE_SERVICE_SECURITY | `core/services/security/policyd/` | |
| `core/kernel/src/security/crypto_registry.c` | MOVE_SERVICE_SECURITY | `core/services/security/crypto/` | |
| `core/kernel/src/security/vfio.c` | MOVE_SERVICE_DEVICE | `core/services/device/vfio/` | Plus kernel hooks for IOMMU. |
| `core/kernel/src/power/` | MOVE_SERVICE_SYSTEM | `core/services/system/powerd/` | Move policy, keep primitive hooks in kernel. |
| `core/kernel/src/device/irq_domain.c` | KEEP_KERNEL_MECHANISM | - | Interrupt routing mechanism. |
| `core/kernel/src/device/device_dma.c` | KEEP_KERNEL_MECHANISM | - | DMA isolation mechanism. |
| `core/kernel/src/device/pci.c` | MOVE_DRIVER | `drivers/bus/pci/` | PCI enumeration/config. |
| `core/kernel/src/device/device_manager.c` | MOVE_SERVICE_DEVICE | `core/services/device/devmgr/` | |
| `core/kernel/src/display/` | KEEP_KERNEL_MECHANISM | - | Early boot/panic display only. |
| `core/kernel/src/console/early_console.c` | KEEP_KERNEL_MECHANISM | - | |
| `core/kernel/src/console/console_core.c` | MOVE_SERVICE_SYSTEM | `core/services/system/console/` | |
| `core/kernel/src/sched/ai_sched.c` | MOVE_TOP_LEVEL_STAGING | `staging/ai/` | |
| `core/kernel/src/sched/sched_core.c` | KEEP_KERNEL_MECHANISM | - | |
| `core/kernel/src/profile/profile.c` | KEEP_KERNEL_MECHANISM | - | Tiny descriptor only. |
| `core/kernel/src/profile/profile_policy.c` | MOVE_SERVICE_CORE | `core/services/core/policymgr/` | |
| `core/kernel/src/trace/` | KEEP_KERNEL_STUB_OR_HOOK | - | Bounded event emission in kernel. |
| `core/kernel/src/monitor/` | MOVE_SERVICE_SYSTEM | `core/services/system/monitord/` | |
| `core/kernel/staging/ai/` | MOVE_TOP_LEVEL_STAGING | `staging/ai/` | |
| `core/kernel/staging/distributed/` | MOVE_TOP_LEVEL_STAGING | `staging/distributed/` | |
| `core/kernel/src/ipc/ipc_traffic.c` | MOVE_SERVICE_CORE | `core/services/core/ipcmgr/` | Policy/analytics. |
| `core/kernel/src/ipc/async_ipc.c` | KEEP_KERNEL_MECHANISM | - | If it's a core primitive. |

## Intentionally Retained Mechanism Types
The following types are retained in the kernel (`core/kernel/include/sched/ai_sched.h`) as mechanism-facing contracts:
- `ai_sched_context_t`: Fixed-size metadata in `bh_thread_t` for scheduler accounting.
- `ai_suggestion_t`: Bounded structure for scheduler hint delivery.
- `ai_pmc_sample_t`: Hardware performance counter sample format.
