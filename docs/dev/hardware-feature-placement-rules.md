# Hardware Feature Placement and Ownership Rules

## Overview
This document enforces strict boundaries for where different parts of a hardware feature's lifecycle (discovery, contract, implementation, policy, and orchestration) must live. These rules prevent the kernel from accumulating policy, and user-space from acquiring direct hardware control.

## Component Boundaries

### 1. `core/arch/` (ISA Implementation)
- **Role:** Pure ISA-specific mechanisms.
- **Allowed:** Assembly routines, raw register definitions, context switching, basic MMU/TLB instruction wrappers.
- **Forbidden:** Hardware discovery, policy logic, complex dispatching.

### 2. `core/hal/` (Contracts)
- **Role:** The abstraction layer translating standard OS expectations into `core/arch/` or `core/platform/` calls.
- **Allowed:** Interface definitions (`hal_caps.h`, `hal_dma.h`), generic stubs.
- **Forbidden:** Board-specific implementations, vendor-specific driver logic.

### 3. `core/platform/` (SoC/Board Discovery)
- **Role:** SoC and board wiring, discovering what hardware is actually present.
- **Allowed:** Device tree parsing, ACPI tables, bootloader handoffs, populating `bharat_hw_caps_t`.
- **Forbidden:** Exposing board-specific structs globally, managing the lifecycle of services.

### 4. `core/drivers/` (Hardware Control)
- **Role:** Managing the immediate state of hardware components.
- **Allowed:** MMIO reads/writes, interrupt handling for the device, providing an interface to the rest of the OS.
- **Forbidden:** Global policy decisions, direct application-to-driver raw hardware paths.

### 5. `core/kernel/` (Mechanism Only)
- **Role:** Centralized enforcement of mechanisms, secure memory management, capability registration.
- **Allowed:** `bharat_hw_caps_registry` (immutable post-boot), PMM/VMM logic, IPC routing.
- **Forbidden:** Any policy engines, vendor-specific logic, AI scheduling logic (except core mechanisms), driver details.

### 6. `lib/runtime/` (Backend Dispatch Logic)
- **Role:** The intelligence for choosing between hardware backends and software fallbacks.
- **Allowed:** Backend selection based on capabilities (`lib/runtime/backend_dispatch/`), software fallback implementations, UAPI interaction.
- **Forbidden:** Directly manipulating MMIO (must go through core/drivers/kernel).

### 7. `core/services/` (Policy and Orchestration)
- **Role:** Deciding what happens based on system profiles, permissions, and thermal/power state.
- **Allowed:** `accelmgr`, routing, admission control, thermal throttling coordination.
- **Forbidden:** Vendor-specific low-level driver logic, generic kernel mechanisms.

### 8. `core/stacks/`
- **Role:** High-level protocol stacks (e.g., networking, USB) that span multiple domains.
- **Allowed:** Reusing features via `lib/runtime/` dispatch (e.g., networking offload).
- **Forbidden:** Hardcoding driver names or platform-specific hacks.

## PR Checklist for Hardware Features
When introducing a new hardware feature or accelerator, verify:
- [ ] Are raw ISA names avoided in global headers (using `CLASS_` or `HW_CAP_STATE_` instead)?
- [ ] Is the feature dynamically discovered and populated via `core/platform/`?
- [ ] Is the kernel strictly handling registration/mechanism without any vendor policy?
- [ ] Are selection and software fallbacks implemented in `lib/runtime/`?
- [ ] Is global system policy handled by a dedicated service in `core/services/` rather than the kernel?
- [ ] Are direct application-to-hardware paths avoided?
