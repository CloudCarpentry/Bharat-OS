---
title: "BIDL Security Annotations"
status: "Draft"
version: "0.1"
last_updated: "2024-03-24"
tags: ["architecture", "ipc", "bidl", "contracts", "security"]
---

# BIDL Security Annotations

## 1. Moving Policy into the Contract

In Bharat-OS, the interface contract must define not only the data layout but also the **security policy**. Relying on handwritten dispatcher code to enforce capability checks, transport rules, or timeouts leads to vulnerabilities (e.g., missed checks, inconsistent handling).

BIDL enforces security constraints explicitly through annotations. The generated C code must **enforce** these annotations. No hand-written dispatcher should be able to bypass the generated check path in production mode.

## 2. Capability and Authority Annotations

Every cross-boundary operation that touches authority must declare its requirements.

### `@requires`
Specifies the capability type and rights required to invoke the method.
```bidl
@requires(capability="ACCEL_QUEUE", rights="ENQUEUE")
method submit_job(handle queue, handle buffer, JobDesc desc) -> (Status status, u64 ticket);
```

### `@authority`
Defines the scope of the authority (e.g., is it bound to the object, the session, or a specific lease).
```bidl
@authority(scope="object")
```

## 3. Transport and Protocol Guarantees

BIDL must constrain how messages are transmitted, mapping to the underlying uRPC L1 protocol engine.

### `@transport`
Specifies the mandatory transport type and its required reliability class.
```bidl
@transport(type="urpc", reliability="acked")
```

### `@timeout`
Defines the maximum allowed time for the operation to complete before the transport layer aborts it.
```bidl
@timeout(ms=50)
```

### `@idempotent`
Indicates whether the operation can be safely retried by the transport layer upon timeout or failure. State-changing operations MUST be `false`.
```bidl
@idempotent(false)
```

## 4. Lifecycle and Delegation Annotations

When transferring capabilities (especially for hardware access), lifecycle annotations constrain the grant.

### `@revocable`
Explicitly states that the capability being passed or returned can be revoked by the owner.
```bidl
@revocable(true)
```

### `@lease_required` / `@lease_max_ms`
Indicates that the passed capability is a temporal lease, and bounds its maximum lifetime.
```bidl
@lease_required(true)
@lease_max_ms(1000)
```

## 5. Accelerator and DMA Annotations

### `@dma`
For methods that bind memory, specifies the required IOMMU mapping direction.
```bidl
@requires(capability="DMA_GRANT", rights="MAP|UNMAP")
@dma(direction="to_device")
method bind_buffer(handle grant, handle queue) -> (Status status);
```

## 6. Fault Domain and System Constraints

### `@fault_domain`
Restricts capability transfer across fault domain boundaries. Some transfers are only legal within the same fault domain, while others might cross only if supervisor-mediated.
```bidl
@fault_domain("same-domain")
```

### `@profile`
Restricts the method to specific OS profiles (Real-Time, General Purpose, Mixed Criticality, Safety).
```bidl
@profile("rt")
```

## 7. Generated Code Enforcement

When the BIDL compiler processes these annotations, it generates:
1.  **Capability Lookup:** Code to resolve the handle and verify the capability type.
2.  **Rights Check:** Code to ensure the caller holds a superset of the required rights.
3.  **Transport Validation:** Metadata that the uRPC layer uses to set timeouts, enforce retries, and require ACKs.
4.  **Domain & Lease Checks:** Pre-dispatch code that validates fault domain transfers and lease constraints.

Any failure in these generated checks results in an immediate `K_ERR_DENIED` (or similar protocol-level rejection) before the service implementation is ever invoked.