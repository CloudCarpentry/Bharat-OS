# SMP-P1-001 — URPC Ring Memory Ordering Audit

## 1. Ownership Model Identification
A full review of the `urpc_ring` implementations across the Bharat-OS kernel indicates the intended and effectively utilized ownership model is **Single-Producer / Single-Consumer (SPSC)**.
- **`multikernel.c` (URPC core transport):** Explicitly operates as SPSC per channel matrix with core-to-core mappings.
- **`urpc_bootstrap.c`:** Uses a one-to-one channel model per secondary core.
- **`lib/ds/urpc_ring.c`:** Mislabeled as MPMC in headers. It attempted MPMC synchronization using lock-free CAS loops but violated correctness constraints (described below). Given architecture requirements for uncontended fast paths (`KURPC-FAST-001-spsc-lanes.md`), transforming this to a standard SPSC is structurally correct and mitigates the defect.

## 2. Memory Ordering Audit & Correctness Defects
The codebase possessed multiple distinct memory ordering defects:
1. **`lib/ds/urpc_ring.c` (Payload publication defect):** The producer loop explicitly used an `atomic_compare_exchange_weak_explicit` to update the tail index (publishing the slot) *before* executing the `urpc_memcpy` to actually write the payload. This resulted in consumers reading incomplete or stale data.
2. **`urpc_bootstrap.c` (Compiler vs Hardware Barrier defect):** The queue index updates utilized `volatile` and `__asm__ volatile("" : : : "memory")` compiler barriers. While this prevents compiler instruction reordering, it does not instruct the CPU hardware to serialize memory access.

## 3. Happens-Before Argument and Fix Justification
The required happens-before relationships for an SPSC ring buffer are:
- **Publication (Producer):** Payload writes must strictly happen before the queue head is advanced. We use C11 C_atomic `atomic_store_explicit(..., memory_order_release)`.
- **Consumption (Consumer):** Queue head observation must strictly happen before payload reads. We use `atomic_load_explicit(..., memory_order_acquire)`.
- **Reclamation (Consumer -> Producer):** The consumer updates the tail index with `memory_order_release` to indicate free slots. The producer observes it with `memory_order_acquire`.

## 4. ARM64 and RISC-V Memory-Model Implications
Both ARM64 and RISC-V employ **weakly ordered memory models**.
- A CPU is permitted to reorder loads and stores unless explicitly constrained by synchronization instructions.
- A compiler barrier (`__asm__ volatile("" ::: "memory")`) ensures the compiler doesn't reorder instructions in the generated binary, but the CPU itself may still reorder them in its store buffers or load queues.
- C11 `<stdatomic.h>` with explicit `memory_order_release` and `memory_order_acquire` automatically translates to the appropriate architecture-specific barrier instructions:
  - **ARM64:** `dmb ish` (Data Memory Barrier, Inner Shareable) or specialized store-release/load-acquire instructions (`stlr`/`ldar`).
  - **RISC-V:** `fence rw, w` / `fence r, rw` or load-reserve/store-conditional with `.aq`/`.rl` flags.
- Relying on `volatile` and compiler barriers as observed in `urpc_bootstrap.c` inevitably results in transient, extremely difficult-to-reproduce memory corruption on physical ARM64 and RISC-V hardware.