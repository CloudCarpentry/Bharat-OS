# BharatLibC

Isolated, profile-driven standard library foundation for Bharat-OS userspace and freestanding components.

## Features
- Modular library targets (`BharatLibC::Core`, `BharatLibC::BSys`, `BharatLibC::Alloc`, `BharatLibC::StdioMin`)
- Profile validation matrix (`nano_mpu`, `embedded_mmulite`, `general_mmu`, `host_test`)
- Clean, compiler-independent standard headers using GCC/Clang built-ins.
- Complete relocation and include-boundary checks.

See `docs/` for more information on architecture, building, porting, and testing.
