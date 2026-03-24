---
title: "DMA Grant Model"
status: "Draft"
version: "0.1"
last_updated: "2024-03-24"
tags: ["architecture", "dma", "capabilities", "memory", "lease"]
---

# DMA Grant Model

## 1. DMA ≠ Normal Memory

In Bharat-OS, hardware accelerators and DMA controllers require direct access to physical memory. However, delegating a standard memory `CAP_TYPE_FRAME` capability to a device or service is dangerous:

- **Temporal Risk:** Device-visible memory has temporal risk. If a kernel crashes or restarts, DMA mappings must die deterministically. A persistent frame capability does not natively support time-bound expiry.
- **Revocation Safety:** Revoking a memory capability might not immediately unmap it from an IOMMU or halt an active DMA transfer, leading to corruption or security breaches.
- **Cache Coherency:** DMA involves complex directionality (e.g., `TO_DEVICE`, `FROM_DEVICE`) and cache flushing requirements that standard frame capabilities do not model.

To secure hardware access, Bharat-OS replaces generic frame delegation with a **LEASE model** for DMA grants.

## 2. Capability Leases

A DMA grant in Bharat-OS is defined as a temporal, revocable lease between an owner (e.g., a process) and a borrower (e.g., an accelerator service or DMA engine).

```c
typedef struct dma_grant {
    uint64_t grant_id;          // Unique ID for the lease
    uint64_t owner_cap;         // The capability holding the original memory frame
    uint64_t borrower_cap;      // The capability granted to the device service

    uintptr_t paddr_base;       // Physical address backing the grant
    uintptr_t iova_base;        // I/O Virtual Address mapped in the IOMMU
    size_t    length;           // Size of the mapped region

    uint32_t rights;            // Granted access rights (e.g., READ, WRITE)
    uint32_t direction;         // TO_DEVICE, FROM_DEVICE, BIDIRECTIONAL

    uint64_t expiry_ns;         // Expiry time in nanoseconds
    uint64_t rev_epoch;         // Revocation epoch of the underlying object

    bool zero_on_release;       // Scrub memory before returning it to the owner
    bool revoke_on_fault;       // Revoke lease if the borrower crashes or faults
} dma_grant_t;
```

## 3. Mandatory Rules for DMA Leases

1.  **MUST Go Via IOMMU:** All DMA grants must be enforced by an IOMMU (Input-Output Memory Management Unit) or SMMU. Direct physical address DMA is only permitted if the hardware platform lacks IOMMU support (and then, only under strict supervisor mediation).
2.  **MUST Be Revocable:** The owner of the memory frame can revoke the DMA lease at any time. Revocation must synchronously tear down the IOMMU mapping before returning.
3.  **MUST Be Bounded Lifetime:** Every lease must have an `expiry_ns`. When the lease expires, the kernel or policy manager automatically revokes the grant, unmapping the IOVA and preventing runaway DMA transfers.
4.  **MUST Be Scrubbed on Release:** If the `zero_on_release` flag is set (crucial for sensitive data or cryptographic operations), the kernel must synchronously zero the physical memory frame before returning control to the owner, preventing data leakage to subsequent users.

## 4. Lifecycle of a DMA Grant

1.  **Request:** A process creates an `Untyped` capability, retypes it to a `Frame`, and requests a `DMA_GRANT` from the kernel, specifying the target device domain and requested lease parameters.
2.  **Mapping:** The kernel maps the frame into the device's IOMMU context, generating an IOVA, and returns a `DMA_GRANT` capability to the requesting process.
3.  **Delegation:** The process delegates the `DMA_GRANT` capability to the device service (e.g., over uRPC).
4.  **Usage:** The device service binds the `DMA_GRANT` to an accelerator queue and submits a job.
5.  **Teardown (Normal):** The job completes, and the device service releases the grant, or the lease expires. The kernel unmaps the IOVA and optionally scrubs the memory.
6.  **Teardown (Fault):** If the device service crashes, the watchdog or fault domain manager invokes revocation, forcing the IOMMU to unmap the frame immediately.