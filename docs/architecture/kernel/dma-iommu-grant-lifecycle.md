# DMA/IOMMU Grant Lifecycle

## Status
Draft / Accepted

## Scope
Defines the lifecycle and management of DMA grants in Bharat-OS, ensuring that all DMA operations are capability-backed and revocable.

## Non-Goals
- Implementing specific IOMMU drivers (HAL responsibility).
- Managing high-level device driver state.

## Ownership Boundary
- **Kernel:** Owns the DMA grant object, state machine, and IOMMU mapping logic.
- **HAL:** Provides IOMMU hardware access (map/unmap).
- **Driver:** Requests grants, maps them, and activates them before initiating DMA.

## Contract
DMA must only be performed on memory ranges explicitly granted via a `bh_dma_grant_t` object.

### State Machine
1. `BH_DMA_GRANT_CREATED`: Object created, parameters validated.
2. `BH_DMA_GRANT_MAPPED`: IOMMU or fallback mapping established.
3. `BH_DMA_GRANT_ACTIVE`: Device is permitted to perform DMA.
4. `BH_DMA_GRANT_REVOKING`: Revocation in progress (unmapping/flushing).
5. `BH_DMA_GRANT_REVOKED`: DMA no longer possible, mapping removed.
6. `BH_DMA_GRANT_FAILED`: Mapping or activation failed.

## Hardware Support Levels
- **IOMMU Present:** `BH_PRIMITIVE_HARDWARE_ENFORCED`. Full isolation.
- **No IOMMU, Fallback Allowed:** `BH_PRIMITIVE_SOFTWARE_FALLBACK`. No isolation, requires trust.
- **No IOMMU, Fallback Forbidden:** `BH_PRIMITIVE_UNSUPPORTED`. DMA operations will fail.

## Failure Behavior
- If IOMMU mapping fails, the grant moves to `FAILED`.
- If revocation fails to flush the IOMMU, it remains in `REVOKING` and must be treated as a security breach if accessed.

## Security Invariants
- No driver can DMA arbitrary memory without a grant.
- Grants must have an owner, device, rights, and a defined range.
- Revocation must be synchronous or guarantee no further device access upon return.

## Testing Requirements
- State transition tests.
- IOMMU-on vs IOMMU-off behavior tests.
- Revocation race condition tests.
