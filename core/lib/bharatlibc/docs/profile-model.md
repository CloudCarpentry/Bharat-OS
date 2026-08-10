# Profile Model

BharatLibC defines a set of high-level profiles that dictate memory configurations, execution models, and features.

## Available Profiles

1. **nano_mpu**: Optimized for MPU-only architectures. Single address space, fixed region allocator, minimal I/O, no floating-point printf, no thread/process/virtual memory concepts.
2. **embedded_mmulite**: For lightweight MMU architectures. Restricts virtual memory mapping, provides bounded allocators, and optional single-thread or limited thread execution.
3. **general_mmu**: Fully-featured MMU runtime. Supports POSIX processes, thread structures, and dynamic allocation.
4. **host_test**: Adapts BharatLibC for compiling directly against host operating systems using mock backend dispatchers for unit tests and sanitizers.
