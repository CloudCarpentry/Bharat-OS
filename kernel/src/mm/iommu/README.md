# IOMMU Layer
- Owns HAL-facing / domain-facing, isolated from generic VM policy.
- May call: HAL IOMMU.
- Forbidden: Generic VM policy.
