# HMEM Production Roadmap

This roadmap defines the path to evolve Bharat-OS's Heterogeneous Memory (HMEM) mechanism from a P0 prototype into a capability-secured, per-core production subsystem.

## Phase P0 — Real CPU-backed userspace HMEM

*   **Canonical VM-backed HMEM objects**: Replace kernel-heap allocation with page/VM-backed storage.
*   **Process capability ownership**: Integrate `CAP_TYPE_HMEM` and enforce rights (read, write, map, query, destroy, share, derive) locally to a process's CSpace.
*   **Owner-local registry**: Remove the global registry lock; objects are tracked per-core or per-owner and interacted with via uRPC.
*   **Native syscall integration**: Make `hmem_create`, `hmem_map_cpu`, etc., genuine ABI calls.
*   **SDK native backend**: Split `posix_memalign` host backend from the Bharat-native syscall-driven backend.
*   **Real userspace application**: Use an actual lifecycle flow (`servicemgr → process_manager → vm_manager → ...`) rather than a direct kernel call.
*   **QEMU end-to-end validation**: Pass tests strictly through guest processes.

## Phase P1 — Sharing, scalability and security

*   **Cross-process HMEM sharing**: Robust sharing mechanisms allowing other processes to derive capability handles.
*   **Capability attenuation/revocation**: Guarantee derived capabilities cannot elevate privileges and can be safely revoked.
*   **SMP and uRPC operations**: Verify cross-core HMEM allocation and usage.
*   **Memory quotas**: Bind object allocation to namespace quotas.
*   **Object lifetime and cleanup**: Enforce strict quarantine and teardown sequences (LIVE → DYING → DEAD).
*   **Stress and fuzz testing**: Validating against malicious user input (invalid capabilities, invalid mappings, etc.).

## Phase P2 — Device integration

*   **DMA/IOMMU mapping**: Properly map objects for hardware access.
*   **Device capability validation**: Enforce split authority (`HMEM_MAP_DEVICE` right + DMA authority).
*   **Virtual DMA accelerator**: A virtual IOMMU loopback accelerator in QEMU to validate workflows.
*   **Fences and asynchronous completion**: Explicitly track timelines and synchronization objects.
*   **Safe CPU/device ownership transfer**: Avoid data races when accelerator engines consume CPU-generated HMEM data.

## Phase P3 — Heterogeneous compute

*   **Real accelerator backend**: Integration with initial GPU/NPU architectures.
*   **GPU/NPU integration**: Extended cache synchronization primitives, driver placement, and direct-device mapping capabilities.
*   **Runtime placement policy**: Give `accelmgr` and `vm_manager` authority to select placement (NUMA, host, remote).
*   **Tensor execution and graph runtime**: Deploy machine-learning stacks taking full advantage of HMEM capabilities.
*   **Memory migration where supported**: Migrate handles transparently between nodes/accelerator memory.

## Phase P4 — Commercial readiness

*   **Physical hardware validation**: Prove speedup/zero-copy benefits on real edge and GP hardware.
*   **Security and performance hardening**: Penetration testing against capability derivation/revocation edges.
*   **Observability**: Integrate performance events (copies avoided, map counts, teardown timing) into system telemetry.
*   **API stability**: Formal freeze of UAPI structure formats and capability semantics.
*   **Fault injection**: Introduce errors during mappings, memory starvation, and process crashing, verifying deterministic cleanup.
*   **Long-duration stress testing**: Ensure system stability under heavy concurrency and memory load over hours or days.
*   **Reproducible release evidence**: Emit exact validation JSONs (`hmem-results.json`) per build architecture.
