---
title: Services Subcomponents Architecture (Repository-Aligned Status + Roadmap)
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - components
see_also:
  - README.md
---
# Services Subcomponents Architecture (Repository-Aligned Status + Roadmap)

This document maps the service layer to the **current repository structure** and highlights consolidation work needed to align with `docs/architecture/folder_structure.md`.

## Current repository-aligned service map

```mermaid
%%{init: {'theme':'base','themeVariables':{'primaryColor':'#14213d','primaryTextColor':'#ffffff','lineColor':'#fca311','fontFamily':'Inter'}}}%%
graph TB
    SV[core/services/] --> CORE[core/services/core/*]
    SV --> SYSTEM[core/services/system/*]
    SV --> DEVICE[core/services/device/*]
    SV --> NETWORK[core/services/network/* + net*]
    SV --> LEGACY[legacy flat managers]

    CORE --> INIT[core/init]
    CORE --> DEVMGR2[core/devmgr]
    CORE --> SUBSYSMGR[core/subsysmgr]

    SYSTEM --> CONSOLE[system/console]
    SYSTEM --> FS[system/filesystem]
    SYSTEM --> BOOTD[system/boot_displayd]
    SYSTEM --> DIAG[system/diag]
    SYSTEM --> SHELL[system/shell]

    DEVICE --> ACCELMGR[device/accelmgr]
    DEVICE --> INPUTMGR[device/inputmgr]
    DEVICE --> ACTUATOR[device/actuator_mgr]

    NETWORK --> NETMGR2[network/netmgr]
    NETWORK --> NETSTACK2[network/netstack]
    NETWORK --> NETMGR1[netmgr]
    NETWORK --> NETSTACK1[netstack]
    NETWORK --> NETFAST[netfast]

    LEGACY --> COREMGR[coremgr]
    LEGACY --> DEVMGR1[devmgr]
    LEGACY --> MEMMGR[memmgr]
    LEGACY --> SCHEDMGR[schedmgr]
    LEGACY --> TELEMETRY[telemetrymgr]
    LEGACY --> STORAGEMGR[storagemgr]
    LEGACY --> SERVICEMGR[servicemgr]
```

## Alignment with `folder_structure.md`

| Target bucket (folder_structure) | Current paths present | Alignment | Notes |
| --- | --- | --- | --- |
| `core/services/core/` | `core/services/core/init`, `core/services/core/devmgr`, `core/services/core/subsysmgr` | Partial | New hierarchy exists, but legacy flat managers still active in parallel. |
| `core/services/system/` | `core/services/system/console`, `boot_displayd`, `filesystem`, `diag`, `shell`, `footprintd` | Strong | Matches target intent. |
| `core/services/security/` | `core/services/security/crypto` | Partial | Crypto exists; keystore/identity services are not yet separated. |
| `core/services/device/` | `core/services/device/accelmgr`, `inputmgr`, `actuator_mgr` | Strong | Good separation for device policy managers. |
| `core/services/network/` | `core/services/network/netmgr`, `core/services/network/netstack` | Partial | Duplicate legacy net daemons remain (`core/services/netmgr`, `core/services/netstack`, `core/services/netfast`). |

## Service status matrix (implementation + structure)

| Service area | Current status | Evidence in tree | Next structural action | Roadmap linkage |
| --- | --- | --- | --- | --- |
| Core management | Partial | both `core/services/core/*` and flat `core/services/*mgr` | Converge all managers under `core/services/core/` with compatibility shims for include paths. | Phase 1 |
| Naming and orchestration | Partial | `core/services/namesvc`, `core/services/servicemgr`, `core/services/core/init` | Consolidate registry + orchestration contracts into a single `core/` namespace. | Phase 1 |
| Network control/data plane | Partial | parallel `core/services/network/*` and top-level `net*` services | Select canonical location (`core/services/network/*`) and deprecate duplicates. | Phase 1, Phase 3 |
| Platform-facing system services | Partial | `core/services/system/filesystem`, `console`, `boot_displayd` | Tighten IPC contracts and phase out ad-hoc direct couplings. | Phase 2 |
| Security services | Scaffold/Partial | `core/services/security/crypto` only | Add keystore, attestation, and policy service boundaries. | Phase 2, Phase 4 |

## Coding tasks identified

1. **Service tree consolidation:** move flat managers (`core/services/coremgr`, `core/services/devmgr`, `core/services/memmgr`, `core/services/schedmgr`, `core/services/storagemgr`, `core/services/telemetrymgr`) behind canonical `core/services/core/*` modules.
2. **Duplicate network path cleanup:** keep `core/services/network/netmgr` and `core/services/network/netstack` as canonical; convert `core/services/netmgr`, `core/services/netstack`, `core/services/netfast` into wrappers or remove after migration.
3. **Security boundary completion:** split `core/services/security/crypto` responsibilities into crypto provider vs key management service and define explicit IPC IDs in `interface/idl/core/services/`.
4. **Header/API normalization:** update `core/services/include/core/services/*` headers to avoid stale include paths during migration.
