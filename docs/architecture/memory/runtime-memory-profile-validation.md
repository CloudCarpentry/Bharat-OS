# Runtime Memory Profile Validation

## Status
Partial / M0 baseline

## Problem
Compile-time memory model selection is not enough. The kernel must verify at boot that detected hardware capabilities satisfy the selected memory contract.

## Goals
- Validate detected HAL memory capabilities.
- Validate selected memory model/profile contract.
- Fail closed on impossible combinations.
- Keep validation kernel-internal for now.

## Non-Goals
- No PMM per-core magazines.
- No TLB shootdown redesign.
- No VM lifecycle rewrite.
- No UAPI exposure.
- No full IOMMU implementation.

## Capability Sources
- HAL/architecture reports `hal_mem_caps_t`.
- Kernel normalizes to `mem_runtime_caps_t`.
- Build/profile derives `mem_profile_contract_t`.

## Validation Rules
### MMU_FULL
- Requires `supports_mmu`.
- Requires `supports_page_protection`.
- Requires `supports_region_protection`.
- Requires `supports_shared_memory`.
- Optionally requires `supports_demand_paging` if `CONFIG_DEMAND_PAGING_REQUIRED` is set.
- Optionally requires `supports_iommu` if `CONFIG_IOMMU_REQUIRED` is set.

### MMU_LITE
- Requires `supports_mmu_lite`.
- Requires `supports_page_protection`.
- Requires `supports_region_protection`.
- Requires `supports_shared_memory`.

### MPU_ONLY
- Requires `supports_mpu`.
- Requires `supports_region_protection`.
- Rejects `require_demand_paging`.

## Failure Policy
Invalid combinations fail closed during boot validation, resulting in a kernel panic.

## Test Requirements
- Unit tests for synthetic capability/contract combinations.
- Boot logs should indicate the success or failure of the validation.
