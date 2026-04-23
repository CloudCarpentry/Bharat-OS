# Footprint Enforcement

Footprint enforcement is performed as part of target validation.

## Build-time checks

The build pipeline enforces:

1. `profile_id` exists in `configs/footprint/footprint_matrix.csv`.
2. Target `arch` matches the matrix row.
3. Protection model compatibility (MMU/MPU expectations).
4. Target memory (from run config) satisfies `boot_min_ram_kb`.

## Rejection rules

A target is rejected when any of the following is true:

- Missing `footprint_profile` declaration.
- Unknown profile id.
- Arch/profile mismatch.
- RAM lower than `boot_min_ram_kb`.

## CI guidance

CI should run target validation across the ISA/profile matrix and fail fast on any footprint contract violations.

Suggested command:

```bash
python3 tools/build.py build --target-yaml tools/targets/qemu/arm32_micro.yaml
```
