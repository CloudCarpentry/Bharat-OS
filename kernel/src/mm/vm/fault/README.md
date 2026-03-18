# VM Fault Layer
- Owns lazy allocation, COW break, stack growth, page-in/out policy.
- May call: ASpace, VM Objects, PMM, HAL map helpers.
- Forbidden: Raw arch code directly.
