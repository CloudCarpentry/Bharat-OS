1. Add `brk` boundary tracking to `bh_process_t` in `core/kernel/include/sched/sched.h`.
   - Add `uintptr_t brk_start;` and `uintptr_t brk_current;` to `struct bh_process`.
2. Initialize `brk` boundaries in `process_create`.
   - Modify `process_create` in `core/kernel/src/sched/sched.c` and `core/kernel/src/sched_stub.c` to set `brk_start` and `brk_current` to `0`.
   - Read the modified files using `cat` to verify changes.
3. Implement `linux_sys_brk` in `core/personalities/compat/linux/linux_syscall.c`.
   - Ensure to use the `vm_map_region` and `vm_unmap_region` functions declared in `core/kernel/include/mm/aspace.h`.
   - We will use `#include "mm.h"` which contains the `#define PAGE_SIZE 4096`.
   - Ensure to use mathematical formula for page align. Example: `(x + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1)` for ROUND UP. Example for ROUND DOWN `x & ~(PAGE_SIZE - 1)`.
   - The `brk` syscall receives a new `brk` value.
   - If the new value is 0 or less than `brk_start`, return the current `brk` value.
   - If the new value requires expansion, use `vm_map_region` with an anonymous mapping to map from page-aligned `brk_current` to page-aligned `new_brk`.
     - Align `brk_current` using round up. Align `new_brk` using round up.
   - If the new value requires shrinkage, use `vm_unmap_region` to unmap from page-aligned `new_brk` to page-aligned `brk_current`.
     - Align `new_brk` using round up. Align `brk_current` using round up.
   - Update `ctx->process->brk_current` to the new value and return it.
   - Verify the file was updated correctly via `read_file` or `cat`.
   - Run compilation (`./nirmaan build desktop-x86_64 --mode release`) to ensure no syntax/type errors.
4. Run tests
   - Run `ctest --test-dir build/x86_64-qemu-desktop-release` to run the tests and ensure no regressions.
5. Complete pre commit steps to ensure proper testing, verification, review, and reflection are done.
