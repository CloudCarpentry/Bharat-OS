# VM Objects Layer
- Owns anonymous/file/shared/device/DMA objects + COW semantics.
- May call: PMM for physical frame allocations.
- Forbidden: ASpace layers, arch internals.
