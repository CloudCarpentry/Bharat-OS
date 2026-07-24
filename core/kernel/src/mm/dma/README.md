# DMA Layer
- Owns pinning, IOVA domains, cache sync, device-visible mappings.
- May call: PMM and HAL IOMMU.
- Forbidden: Generic VMM internals for device mappings.
