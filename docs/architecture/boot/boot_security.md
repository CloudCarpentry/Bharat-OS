# Boot Security Metadata

**Status**: Active
**Version**: 1.0
**Owner**: Bharat-OS Security Team

## 1. Context
`boot_security.h` introduces a structured evaluation model (`boot_security_evaluate()`) running immediately post-adapter. It explicitly tracks `secure_boot_present`, `secure_boot_verified`, `measured_boot_present`, `tpm_present`, and measurement metadata.

## 2. Policy Enforcement
- If the hardware profile insists on secure boot (`boot_security_info_t.secure_boot_verified`), it fails validation if missing.
- `BOOT_MODE_RECOVERY` can permit insecure setups via `BOOT_SEC_DECISION_WARN_AND_ALLOW`.
- `BOOT_MODE_DIAGNOSTIC` / `BOOT_MODE_MANUFACTURING` implicitly pass if dev-certs or unsigned policies exist in profile.

## 3. Decisions
- `BOOT_SEC_DECISION_ALLOW`
- `BOOT_SEC_DECISION_DENY`
- `BOOT_SEC_DECISION_WARN_AND_ALLOW`
