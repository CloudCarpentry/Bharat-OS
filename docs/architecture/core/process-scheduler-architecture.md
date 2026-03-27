# Process & Scheduler Architecture in Bharat-OS

This document outlines the fundamental shift in how Bharat-OS handles processes, threads, and scheduling compared to traditional monolithic operating systems like Linux, Windows, macOS, iOS, and Android.

---

## 1. How Traditional OS Handle Process & Threads

### Core Idea (All Mainstream OS)
They all follow a **monolithic process model with threads inside**.

### Common Structure
```text
Process (address space)
   ├── Thread 1
   ├── Thread 2
   ├── Thread 3
```

### Key Characteristics

| Aspect                  | Linux                          | Windows        | macOS / iOS  | Android      |
| ----------------------- | ------------------------------ | -------------- | ------------ | ------------ |
| Process = Address Space | ✅                              | ✅              | ✅            | ✅            |
| Threads share memory    | ✅                              | ✅              | ✅            | ✅            |
| Scheduler scope         | Global / per-CPU               | Global         | Global       | Linux-based  |
| Kernel ownership        | Central kernel                 | Central kernel | XNU (hybrid) | Linux kernel |
| IPC                     | Syscalls, pipes, shared memory | Handles, RPC   | Mach ports   | Binder       |

### Fundamental Limitation (ALL of them)
**They assume a shared kernel with shared global state.**

Problems:
* Global scheduler state
* Global process tables
* Shared memory coordination
* Lock contention
* Weak isolation between threads
* Hard to scale across cores cleanly

Even modern systems like the Linux kernel, Windows NT kernel, and XNU kernel still rely heavily on shared structures, locks, and interrupts.

---

## 2. Bharat-OS Model

The Bharat-OS architecture is fundamentally different:
👉 **Multikernel + Capability-driven + Service-oriented**

### Key Shift: Process vs Thread is NOT the primary abstraction

Instead, Bharat-OS thinks in:

#### ✅ 1. Execution Units (not classic threads)
* Lightweight schedulable entities.
* Owned **per-core**.
* No global thread table.

#### ✅ 2. Address Spaces (capability-controlled)
* Managed via `vm_manager`.
* Not tightly bound to "process" like Linux.

#### ✅ 3. Services (first-class citizens)
* `process_manager`
* `netmgr`
* `vm_manager`
* **Note:** These are NOT apps; they are OS building blocks.

#### ✅ 4. Capabilities (security + ownership)
* Define who can:
  * Access memory.
  * Invoke services.
  * Control resources.

### Bharat-OS Structure (Conceptual)

```mermaid
flowchart TD
    subgraph Core [Core (Per CPU)]
        S[Scheduler (local)]
        EU1[Execution Unit 1]
        EU2[Execution Unit 2]
        LQ[Local Queues]
        S --> EU1
        S --> EU2
        S --> LQ
    end

    subgraph Services [Services (User/System Space)]
        PM[process_manager]
        VM[vm_manager]
        NM[netmgr]
        DM[devmgr]
    end

    subgraph Comm [Communication]
        IPC[IPC / URPC message-based]
    end

    subgraph Security [Security]
        CAP[Capability Tokens]
    end

    Core <--> Comm
    Services <--> Comm
    Comm <--> Security
```

### BIG Difference from Linux/Windows

| Concept   | Traditional OS       | Bharat-OS                       |
| --------- | -------------------- | ------------------------------- |
| Process   | Central abstraction  | Just one service-managed entity |
| Threads   | Inside process       | Per-core execution units        |
| Scheduler | Global               | Per-core ownership              |
| Memory    | Implicit sharing     | Capability-controlled           |
| IPC       | Optional             | Mandatory                       |
| Kernel    | Central authority    | Distributed (multikernel)       |
| Isolation | Weak between threads | Strong everywhere               |

---

## 3. Deep Differences

### 3.1 Scheduler Model
* **Traditional OS:** Global run queue or per-CPU with sync. Needs locks.
* **Bharat-OS:** **Each core owns its scheduler.** No shared run queue.
  👉 Matches the principle: *"No shared mutable state across cores"*

### 3.2 Thread vs Execution Model
* **Traditional OS:** Thread = shared memory worker. Synchronization needed (mutex, semaphores).
* **Bharat-OS:** Execution unit = isolated. Communication via messages.
  👉 Reduces race conditions, deadlocks, and cache thrashing.

### 3.3 Process Concept
* **Traditional OS:** Process = everything (memory + threads + resources).
* **Bharat-OS:** Process is managed by `process_manager`, is just a container, and is not a scheduling unit.
  👉 Decoupling provides a massive advantage.

### 3.4 IPC (The Real Revolution)
* **Traditional OS:** IPC is secondary (optional optimization).
* **Bharat-OS:** IPC is the **mandatory path**.
  👉 Aligns with microkernel ideas (e.g., Mach), but is better because it is capability-secured, per-core optimized, and URPC-based.

### 3.5 Security Model
* **Traditional OS:** UID/GID / ACL / sandbox.
* **Bharat-OS:** Capability = **unforgeable authority**.
  👉 This is a **way stronger** security model.

---

## 4. Practical Benefits

### 🚀 4.1 Massive Scalability (Multi-core & Future CPUs)
* **Traditional OS:** Struggle beyond 32–128 cores due to lock contention bottlenecks.
* **Bharat-OS:** Per-core ownership and message passing scale naturally to 64 cores, 256 cores, and heterogeneous cores (big.LITTLE, NPU, GPU).

### 🚀 4.2 True Isolation (Not Fake Like Threads)
* **Traditional OS:** Threads can corrupt process memory; bugs propagate.
* **Bharat-OS:** Isolation by default; no accidental sharing. Huge for automotive, medical, and cloud isolation.

### 🚀 4.3 Fault Containment
* **Traditional OS:** One bad thread can crash a process, sometimes causing a kernel panic.
* **Bharat-OS:** Strict fault domain isolation and service restart policies.

### 🚀 4.4 Security (Real Capability Model)
* **Traditional OS:** Permission checks scattered; bypass bugs are common.
* **Bharat-OS:** Capability enforced at IPC boundaries.

### 🚀 4.5 Clean Service-Oriented OS
* **Traditional OS:** Kernel bloated; drivers + policy mixed.
* **Bharat-OS:** Kernel = minimal mechanism; services = real OS policy.

### 🚀 4.6 Better for Mixed-Critical Systems
* **Traditional OS:** Hard to isolate systems like automotive (infotainment + braking), drones, or medical (UI + life-critical).
* **Bharat-OS:** Separate services + capabilities makes this the biggest advantage.

### 🚀 4.7 Easier Evolution (Future-proof)
* **Traditional OS:** Hard to change kernel behavior.
* **Bharat-OS:** Change services independently, swap personalities (Linux/Android compatibility).

---

## 5. Strong Opinion

If executed correctly:
👉 Bharat-OS is NOT just another OS.
👉 It becomes closer to:
* A distributed system inside a single machine.
* A microkernel done right.
* A practical multikernel (like Barrelfish).

---

## 6. Where You Must Be Careful

Because this model also has risks, we must carefully consider:

### ⚠️ 1. IPC Overhead
* Must optimize URPC (zero-copy, batching).

### ⚠️ 2. Debugging Complexity
* Distributed debugging needed.

### ⚠️ 3. Service Maturity
* Ensure the roadmap covers gaps like the `process_manager` and `vm_manager`.

### ⚠️ 4. Strict Capability Enforcement
* Capability enforcement MUST be strict, otherwise the whole model collapses.

---

## 7. Simple Summary

### Traditional OS
```text
Process = everything
Threads = shared workers
Kernel = central authority
IPC = optional
```

### Bharat-OS
```text
Execution = per-core units
Process = service-managed container
Kernel = minimal mechanism
Services = real OS
IPC = mandatory
Capabilities = security
```

---

## Final Take

When fully committed to no shared state, strict capability enforcement, and a service-first architecture, this model delivers:
✅ Better scalability than Linux
✅ Better isolation than Windows/macOS
✅ Better security than Android/iOS
✅ Better flexibility than all of them
