---
title: ADR 011: Structured Kernel Panic and Diagnostics
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - adr
see_also:
  - README.md
---
# ADR 011: Structured Kernel Panic and Diagnostics

## Status

Accepted

## Context

As Bharat-OS matures, debugging kernel crashes and diagnosing unrecoverable faults has become more complex. Historically, kernel panics merely printed a string message and halted the system. In edge computing and automotive contexts, merely halting the machine or displaying a raw string without capturing actionable execution state makes root cause analysis extremely difficult, especially when the system automatically reboots and console logs are lost.

Furthermore, fault boundaries span multiple architectures, and we need a consistent way to pass architecture-specific trap frames to a unified diagnostic formatter.

## Decision

We will implement a structured diagnostic and panic reporting system:

1. **`panic_context_t`:** Introduce a standardized context struct that captures instruction pointers, stack pointers, core IDs, thread/process context, and the memory address of the fault.
2. **Fault Breadcrumbs (`fault_diag.h`):** Track lightweight breadcrumbs (e.g., the last executed system call number) during hot execution paths. When a panic occurs, these breadcrumbs are gathered and attached to the panic context.
3. **PStore (Persistent Storage) Recovery Logging:** Write the generated panic diagnostic string to a designated memory region defined by linker scripts (`_pstore_start` to `_pstore_end`). This region is preserved across warm reboots, allowing the system to read and transmit the crash reason during the subsequent boot cycle.
4. **Architecture State Integration:** Delegate hardware-specific register dumping to the HAL layer via `hal_cpu_dump_trap_frame()` and `hal_cpu_dump_state()`.

## Consequences

### Positive

- **Deeper Observability:** Maintainers and tools can parse structured panic attributes rather than relying strictly on ad-hoc print formatting.
- **Resilience:** Unattended edge devices will preserve panic logs across reboots in PStore, improving remote fleet diagnostics.
- **Traceability:** Fault breadcrumbs provide insight into the exact system calls or transitions that preceded the crash.

### Negative

- **Storage Cost:** PStore requires reserving a chunk of RAM that cannot be used by the general allocator.
- **Performance:** Tracking breadcrumbs in hot paths (like system call entry) introduces slight overhead, though it is designed to be minimal.
