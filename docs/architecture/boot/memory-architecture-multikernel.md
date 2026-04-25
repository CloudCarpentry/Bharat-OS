---
title: 1. Executive Summary
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - boot
see_also:
  - README.md
---
# 1. Executive Summary

This document defines the **Bharat-OS Memory Architecture** aligned with:

* **Multikernel design (per-core kernel instances)**
* **Profile-driven hardware support (MMU / MMU-lite / MPU)**
* **Strict separation of concerns across `/arch`, `/hal`, `/kernel`, `/services`**

The design eliminates:

* global MMU state
* duplicated abstractions (`hal_vmm_*`, `mmu_ops_t`)
* unsafe cross-core memory mutation

And introduces:

* unified **memory authority path**
* **backend-driven memory model**
* **message-based TLB coordination**
* **lifecycle-governed DMA/IOMMU**

---

# 2. Design Principles

## 2.1 Multikernel First

> Treat machine as distributed system of cores, not shared-memory kernel

* Each core owns its memory state
* No shared kernel memory structures
* Cross-core = message passing

---

## 2.2 Mechanism vs Policy

| Layer    | Responsibility                        |
| -------- | ------------------------------------- |
| Kernel   | Mechanism (mapping, fault, switching) |
| Services | Policy (paging, placement, restart)   |

---

## 2.3 Profile-driven Memory

Supports:

| Profile | Model            |
| ------- | ---------------- |
| Server  | Full MMU + IOMMU |
| Mobile  | MMU              |
| Edge    | MMU-lite         |
| IoT     | MPU              |
| Tiny    | No-MMU           |

---

## 2.4 Strict Folder Ownership

```text
/arch     → hardware implementation
/hal      → contracts only
/kernel   → authority + orchestration
/services → policy
```

---

# 3. Gap Resolution Mapping

---

## 3.1 Address Spaces ✅ → FIXED

### Problem

* multiple mapping paths
* direct arch access

### Solution

👉 Enforce **single authority path**

```text
Page Fault / Syscall -> address_space_t -> Region Tree -> VM Object -> Memory Backend -> Arch Implementation
```

---

### Multikernel constraint

* address space is **per-core owned**
* cross-core usage via **capability + message**

---

## 3.2 Page Tables (HAL) ⚠️ → FIXED

### Problem

* `hal_vmm_*` vs `hal_pt_*` vs `mmu_ops_t`
* arch code in wrong folders

---

### Solution

#### HAL = contracts only

```text
core/hal/include/
  hal_mem_model.h
  hal_pt.h
  hal_tlb.h
  hal_mpu.h
  hal_dma.h
  hal_iommu.h
```

---

#### ARCH = implementation

```text
core/arch/
  x86_64/mm/
  arm64/mm/
  riscv64/mm/
```

---

### Memory Backend Model (NEW)

```c
typedef struct {
    int (*map)(...);
    int (*unmap)(...);
    int (*protect)(...);
    int (*activate)(...);
} hal_mem_backend_ops_t;
```

---

### Supported backends

* Page Table (MMU)
* Page Table Lite (MMU-lite)
* MPU (region-based)
* Minimal (identity)

---

## 3.3 TLB Coordination ⚠️ → FIXED

### Problem

* no contract
* unsafe multicore behavior

---

### Solution: Message-based TLB

```text
CoreA -> TLB_INVALIDATE_REQ -> CoreB
CoreB -> flush local TLB
CoreB -> ACK -> CoreA
```

---

### API

```c
hal_tlb_flush_page(core, va, asid);
hal_tlb_flush_range(core, start, len, asid);
hal_tlb_flush_aspace(core, asid);
```

---

## 3.4 DMA / IOMMU ❌ → FIXED

---

### DMA Lifecycle

```text
Alloc -> Map -> Sync for Device -> Device Use -> Sync for CPU -> Unmap -> Free
```

---

### Multikernel rule

* DMA buffers are **owned**
* cross-core requires **grant/capability**

---

### IOMMU Lifecycle

```text
create domain → map → attach device → use → unmap → destroy
```

---

# 4. Memory Model (Critical Addition)

```c
typedef enum {
    HAL_MEM_MODEL_NONE,
    HAL_MEM_MODEL_MPU,
    HAL_MEM_MODEL_MMU_LITE,
    HAL_MEM_MODEL_MMU
} hal_mem_model_t;
```

---

### Feature Descriptor

```c
typedef struct {
    hal_mem_model_t mem_model;
    bool has_tlb;
    bool has_iommu;
    bool supports_asid;
    bool supports_user_space;
} hal_mem_features_t;
```

---

# 5. Per-Core Memory State

```c
typedef struct {
    address_space_t *current_as;
    void *root_pt;
    uint64_t asid;
    tlb_local_state_t tlb;
    dma_context_t dma_ctx;
} core_mem_state_t;
```

---

### ❗ NO GLOBAL STATE

* ❌ active_mmu
* ❌ global_pt
* ❌ global_tlb

---

# 6. MMU / MMU-lite / MPU Support

---

## MMU

* full PT
* ASID
* TLB shootdown

---

## MMU-lite

* reduced PT
* limited mapping

---

## MPU

* region-based
* limited entries
* no TLB

---

## No-MMU

* identity mapping
* minimal protection

---

# 7. 32-bit vs 64-bit

Use:

```c
uintptr_t
paddr_t
size_t
```

Avoid:

* hardcoded 64-bit PTE assumptions
