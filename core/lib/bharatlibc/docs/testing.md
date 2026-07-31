# Testing BharatLibC

BharatLibC maintains a strict quality and test requirement suite.

## Execution and Sanitizers

All unit tests are compiled on the host and must run cleanly with:
- AddressSanitizer (ASan)
- UndefinedBehaviorSanitizer (UBSan)

## Mandatory Verification Tests

1. `test_public_header_self_containment`: Validates that each public header compiles successfully on its own without assuming pre-requisite include state.
2. `test_no_private_kernel_headers`: Scans output dependency files (`.d`) or compilation graphs to verify that no kernel-private headers are included.
3. `test_standalone_relocation`: Relocates the whole project structure to an isolated directory and ensures it compiles, installs, and links correctly against a clean test consumer.
4. Core logic tests: covers memory logic, string logic, error mappings, allocator exhaustions, and capability compatibility checks.
