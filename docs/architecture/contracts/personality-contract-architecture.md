---
title: Personality Contract Architecture
status: Proposed
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - contracts
see_also:
  - README.md
---
# Personality Contract Architecture

### Contract Status
- **Spec**: ✅ Documented and versioned
- **Implemented**: 🚧 Pending core/kernel/service behavior merge
- **Validated**: ❌ Pending stress/fault-injection tests


## 1. Purpose

Bharat-OS supports multiple external execution models without bloating the minimal kernel. Personalities exist to control ABI, contract exposure, and runtime behavior while preserving one small, mechanism-focused kernel.

## 2. Core Rule

**Kernel exports mechanisms.**
**Services export policy.**
**Personalities export interface shape.**

This doctrine governs all architectural decisions regarding contract execution models within Bharat-OS.

## 3. What a Personality Owns

A personality is responsible for:
*   Syscall surface selection
*   ABI rules
*   Object/process/thread model adaptation
*   Signal/exception/fault presentation
*   Filesystem/path conventions (if needed)
*   Handle/fd/capability translation
*   Service discovery view
*   SDK selection/wrapping
*   Permission/capability policy overlays
*   App packaging/runtime expectations

## 4. What a Personality Must Not Own

A personality must strictly **avoid** implementing:
*   Scheduler internals
*   Page-table mechanics
*   Raw IPC machinery
*   Device driver policy
*   Memory allocator policy
*   Generic system service logic

These belong in kernel or services, not personality.

## 5. Personality Classes

The existing separation implies three logical domains, which are adopted here:

### 5.1 Native
*   **Bharat-native ABI and services.**
*   Capability-first architecture.
*   Direct use of the native Bharat SDK.
*   The preferred, long-term, first-class model.

### 5.2 Compatibility Personalities
*   **Examples:** Linux, Android, Windows (future).
*   **Purpose:** These adapt and translate foreign expectations into Bharat core/kernel/services, not pollute the kernel with foreign semantics.

### 5.3 Domain Personalities
*   **Examples:** Automotive, safety/minimal, cloud appliance (future).
*   **Purpose:** These are not OS-compatibility layers, but rather policy/interface profiles specifically tailored for special operational environments.

## 6. Relationship to Profile

These are distinct orthogonal axes:
*   **Profile:** Hardware/runtime/system optimization intent (Example: `DESKTOP`, `EDGE`, `MOBILE`, `SAFETY`, `CLOUD`).
*   **Personality:** External interface/ABI/runtime model (Example: `NATIVE`, `LINUX`, `ANDROID`, `AUTOMOTIVE`).

## 7. Relationship to IDL and UAPI

A personality decides:
*   Which UAPI headers are visible
*   Which syscall groups are enabled
*   Which service contracts are available
*   What SDK facade the app links against
*   Whether capabilities are direct or adapted into another abstraction

**Example:**
*   The Native personality exposes capability-native SDK directly.
*   The Linux compatibility personality may expose fd/ioctl-like wrappers but internally binds to Bharat services and capabilities.

## 8. Native Personality Rule

**The NATIVE personality is the "source of truth".**

*   UAPI is authored native-first.
*   IDL is authored native-first.
*   Capabilities are authored native-first.
*   SDK is authored native-first.
*   Compatibility personalities adapt to native contracts, not the other way around.

Bharat-OS must not become an emulation project with a custom kernel. Compatibility is built on top of the native golden path.
