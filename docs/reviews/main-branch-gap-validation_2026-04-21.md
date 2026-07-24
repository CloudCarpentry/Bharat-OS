---
title: Main branch gap validation (refresh)
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - reviews
see_also:
  - README.md
---
# Main branch gap validation (refresh)

Date: 2026-04-21  
Scope: refresh of outdated `main-branch-gap-validation.md` against current repository state.

## Summary

The previous gap report had correct strategic direction, but several statements were stale and mixed three states: **missing**, **present-but-stubbed**, and **implemented-but-needing-hardening**.

Current status should be tracked as:

1. **Implemented baseline + hardening needed**
   - Cross-core IPC/URPC transport paths and timeout-aware endpoint behavior exist.
   - TLB coordination paths and host stress tests exist.
   - Capability delegation/revocation tree logic exists.

2. **Present but still production-gap**
   - IOMMU/driver depth and DMA isolation completeness.
   - RT admission/control rigor and mixed-criticality guardrails.
   - Observability depth (high-fidelity traces/metrics tooling).

3. **Actionable immediate code-quality gap (completed in this change)**
   - `lib/ipc` receive-path header validation previously collapsed all decode header validation failures into `ERR_VERSION`, losing `ERR_LENGTH` and other precise status codes.
   - This has been corrected so receive now returns the exact validation status code.

## Implemented task in this update

**Task:** Harden production-grade IPC decode semantics by preserving specific protocol validation errors.

- Updated `bharat_ipc_recv_ex()` to return the exact `ipc_check_header()` result instead of forcing `ERR_VERSION` for every validation failure.
- Added host-contract test coverage for:
  - invalid header version -> `ERR_VERSION`
  - oversized payload length field -> `ERR_LENGTH`

## Remaining prioritized production gaps

1. TLB shootdown completion semantics (explicit ack/failure accounting under load).
2. URPC backpressure policy enforcement and saturation behavior tests.
3. IOMMU real domain map/unmap enforcement and DMA negative tests.
4. RT EDF/RMS admission control proof-oriented validation for mixed profiles.

