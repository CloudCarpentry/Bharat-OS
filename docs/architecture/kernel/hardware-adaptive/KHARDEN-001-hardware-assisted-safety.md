---
title: KHARDEN-001 - Hardware-Assisted Kernel Memory Safety
status: Draft
owner: Architecture Working Group
last_updated: 2026-05-15
tags:
  - docs
  - architecture
  - kernel
  - security
see_also:
  - README.md
---

# KHARDEN-001: Hardware-Assisted Kernel Memory Safety

## Context
Bharat-OS can leverage advanced hardware features for kernel hardening. This should be an optional acceleration of a common hardening contract, not a separate kernel design.

## Design
Establish a unified kernel hardening contract with a software baseline and hardware-enhanced options based on capability discovery.

### Hardening Contract

#### Software Baseline
* Slab redzones
* Allocation generation
* Delayed reuse/quarantine
* Freelist corruption protection
* Stack canaries

#### Hardware-Enhanced (Dispatched via `hal_hw_caps_t`)
* Memory tagging
* Pointer authentication
* Branch target protection
* Shadow stack

### ISA Mapping
* **Arm**: MTE (Memory Tagging Extension), PAC/BTI (Pointer Authentication / Branch Target Identification).
* **RISC-V**: Zicfiss (shadow stack), Zicfilp (landing-pad CFI).
* **x86**: CET (Control-flow Enforcement Technology).

## Architecture Model

```mermaid
flowchart TD
    A[Kernel Hardening Request] --> B{Hardware Support Available?}
    B -->|Yes - Arm MTE| C[Apply Tagging/PAC]
    B -->|Yes - RISC-V CFI| D[Enable Shadow Stack / Landing Pads]
    B -->|Yes - x86 CET| E[Enable CET / Indirect Branch Tracking]
    B -->|No| F[Apply Software Baseline]
    F --> F1(Redzones, Canaries, Quarantine)
```

## Execution Plan
1. **Define Hardening API**: Create the neutral interface for applying memory and control-flow protection.
2. **Software Baseline**: Universally enable software redzones and quarantine mechanisms.
3. **Hardware Capability Hooks**: Tie the API to the new granular capabilities in `hal_hw_caps_t` (e.g., `memory_tagging`, `shadow_stack`).
4. **Architecture Backends**: Implement ISA-specific enablement for MTE, PAC/BTI, Zicfiss/Zicfilp, and CET.
5. **Testing**: Write security tests targeting UAF, buffer overflows, and ROP/JOP attempts to verify both software and hardware defenses.