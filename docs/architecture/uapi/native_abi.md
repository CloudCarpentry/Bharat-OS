---
title: Bharat-OS Native UAPI Roadmap
status: active
owner: Architecture Team
reviewers: ["Core Team"]
version: 1.1
last_updated: "2026-04-21"
tags: ["architecture", "uapi", "native", "capabilities", "primitives"]
---

# Bharat-OS Native UAPI (Application Binary Interface) Roadmap

## 1. Introduction

Bharat-OS is built upon a capability-oriented microkernel architecture. To fully leverage the security, isolation, and performance characteristics of this architecture, the system defines a **Native Bharat Personality** and corresponding Application Binary Interface (ABI).

The guiding principle for application interfaces in Bharat-OS is:

> **Kernel mechanisms are native. Services are native. UAPI is native. Compat personalities translate into native.**

This document defines the active roadmap for the **Native UAPI** boundary across `include/bharat/uapi/` and `uapi/`. It is the explicit contract for syscall numbers, capability contracts, shared IPC structures, and stable ABI types.

Foreign ABIs (Linux/POSIX/Android) are **not** the core identity of the OS; they are compatibility personalities that consume these native contracts.

## 2. Current Native Primitive Set (as of April 21, 2026)

The latest additive primitive set in the repo now includes:

1. **Intent execution primitives**
   - `SYSCALL_INTENT_SET`, `SYSCALL_INTENT_GET`.
   - UAPI type: `bharat_intent_t` (`include/bharat/uapi/system/intent.h`).
   - Kernel entry path wired through trap dispatch and scheduler syscall stubs.

2. **Memory semantic class primitive**
   - `SYSCALL_MEM_ALLOC_CLASS`.
   - Memory class taxonomy present in `kernel/include/bharat/mem_class.h`.
   - Kernel stub path available in `kernel/src/mm/mem_class.c`.

3. **Fault-domain primitives**
   - `SYSCALL_FAULT_DOMAIN_CREATE`, `SYSCALL_FAULT_DOMAIN_DESTROY`, `SYSCALL_FAULT_DOMAIN_ATTACH`.
   - UAPI type: `bharat_fault_domain_attr_t` (`include/bharat/uapi/system/fault_domain.h`).
   - Kernel path wired through `kernel/src/core/fault_domain.c` and trap dispatch.

These primitives are implemented as **additive v1 APIs** and do not replace legacy compatibility-facing behavior.

## 3. Native Contract Boundaries

The native contract is cleanly separated across the system architecture:

* **`kernel/`**: mechanisms only (scheduling, memory, syscall/traps, capabilities, IPC/uRPC, faults, minimal coordination).
* **`include/bharat/uapi/` + `uapi/`**: Bharat-native ABI and stable public contracts.
* **`services/`**: policy and orchestration (namespace, lifecycle, supervision, telemetry).
* **`personalities/compat/*`**: strict compatibility adapters mapping foreign semantics into native contracts.

## 4. Native UAPI Domains

### 4.1 Handles and Capabilities
Native applications operate on capability handles with explicit rights and delegation.

### 4.2 Process and Thread Primitives
Thread/process lifecycle remains explicit, with intent metadata attach/query as additive execution hints.

### 4.3 Memory and Object Models
Native memory APIs are capability-scoped and now extended with semantic allocation classes for policy-aware placement and accounting.

### 4.4 IPC and Endpoints
Stable endpoint/message discipline for sync IPC and async uRPC rings, with zero-copy as default for high-throughput paths.

### 4.5 Fault Containment
Fault domains provide first-class containment and restart policy metadata, while keeping restart orchestration in services.

### 4.6 Namespace and Sandbox Semantics
Sandbox and namespace visibility remain capability-mediated. No global ambient namespace is assumed.

## 5. Personality Integration Rule (No Translation Tax)

Compatibility personalities must follow this performance rule:

1. **No forced translation for legacy behavior** when mapping is not enabled.
2. **Single-hop mapping at ABI boundary** (personality edge only), never repeated transformations through internal layers.
3. **No text-based translation in hot paths** (string-heavy remapping loops in read/write/epoll/binder-like hot paths are forbidden).
4. **Use shared native primitives directly** (`intent`, `mem_class`, `fault_domain`) where configured.

This keeps Linux/Android compatibility viable without becoming “working but slow.”

## 6. Roadmap: Native Primitives + Personalities

### Phase A (Now)
- Keep primitives additive and versioned.
- Preserve ABI stability for Linux/Android compatibility by default.
- Add observability counters around primitive usage and fallback paths.

### Phase B (Opt-in mappings)
- Linux: map scheduler/cgroup-like hints to `intent_set/get`, selected alloc paths to mem classes, service groups to fault domains.
- Android: map task profiles/cpusets to intent, media/AI buffers to mem classes, critical services to fault domains.

### Phase C (Zero-translation-path maturity)
- Promote binary/enum-based mapping tables over text/string translation.
- Cache mapping decisions per thread/domain/endpoint.
- Enforce per-personality KPI gates (syscall latency, mmap class overhead, fault-path cost).

## 7. Immediate Documentation Follow-up

To reduce fragmentation, personality architecture, performance contract, and roadmap references are consolidated under `docs/architecture/personalities/` with one index entrypoint.
