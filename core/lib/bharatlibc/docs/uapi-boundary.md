# UAPI Dependency Boundary

To avoid deep architectural coupling, BharatLibC has a strict include boundary rule:

**DO NOT INCLUDE KERNEL-PRIVATE HEADERS.**

## Permitted Include Paths
- `core/lib/bharatlibc/include/**`
- Compiler freestanding headers (builtins/intrinsics)
- Installed or mock UAPI package (`bharat/uapi/**`)

## Prohibited Paths
- `core/kernel/**`
- `core/hal/**`
- `core/arch/**`
- `core/services/**`
- `core/drivers/**`

This boundary is automatically audited via compile-time trace/dependency file parsing during building.
