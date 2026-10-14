# Codebase Inconsistency Review

This document summarizes inconsistencies found in the codebase regarding header inclusion placement, naming conventions, and empty or improper comments.

## 1. `#include` Directives Placed After Code

The following files have `#include` directives that are placed after executable code or non-preprocessor declarations, which violates the standard practice of placing all includes at the top of the file:

- `./staging/ai/ai_kernel_bridge.c`: 1 found
- `./core/services/vm_manager/vm_manager.c`: 1 found
- `./core/services/namesvc/main.c`: 3 found
- `./core/services/common/runtime/src/service_runtime.c`: 2 found
- `./core/services/process_manager/process_manager.c`: 1 found
- `./core/services/core/init/init_manifest.h`: 4 found
- `./core/services/servicemgr/servicemgr.c`: 1 found
- `./core/services/system/console/test_console.c`: 1 found
- `./core/services/system/filesystem/fd_mgr.c`: 1 found
- `./core/services/system/filesystem/ramfs/test_ramfs.c`: 1 found
- `./core/services/netstack/src/main.c`: 2 found
- `./core/services/netstack/src/driver_virtio_adapter.c`: 1 found
- `./core/services/netmgr/src/ipc_auth.c`: 1 found
- `./core/services/netmgr/include/ipc_auth.h`: 1 found
- `./core/services/netmgr/include/ipc_dispatch.h`: 1 found
- `./core/services/netmgr/include/ipc_contract.h`: 2 found
- `./core/services/netmgr/tests/test_netmgr_namesvc_binding.c`: 1 found
- `./core/stacks/media/compositor/compositor_core.c`: 1 found
- `./core/drivers/virtio/core/virtqueue.c`: 1 found
- `./core/drivers/accel/virt_accel.c`: 1 found
- `./core/arch/riscv/riscv32/hal_timer.c`: 1 found
- `./core/arch/riscv/riscv64/cpu_caps.c`: 1 found
- `./core/arch/riscv/riscv64/hal_timer.c`: 1 found
- `./core/arch/riscv/riscv64/hal_cpu.c`: 2 found
- `./core/arch/mpu/arm32/mpu_backend.c`: 1 found
- `./core/arch/arm/arm32/hal_cpu.c`: 1 found
- `./core/arch/arm/arm64/cpu_caps.c`: 1 found
- `./core/arch/arm/arm64/hal_cpu.c`: 2 found
- `./core/arch/x86/x86_64/boot_video_efi.c`: 1 found
- `./core/arch/x86/x86_64/hal_cpu.c`: 3 found
- `./core/arch/x86/x86_64/boot/multiboot_parse.c`: 1 found
- `./core/kernel/src/kernel_boot.c`: 3 found
- `./core/kernel/src/mm/vmm.c`: 4 found
- `./core/kernel/src/mm/prot_domain.c`: 1 found
- `./core/kernel/src/mm/vm/fault/fault.c`: 1 found
- `./core/kernel/src/mm/vm/distributed/vmm.c`: 1 found
- `./core/kernel/src/init/init_bootstrap.c`: 2 found
- `./core/kernel/src/personality/native/native_syscall_handlers.c`: 10 found
- `./core/kernel/src/display/boot_gui.c`: 4 found
- `./core/kernel/src/sched/sched.c`: 1 found
- `./core/kernel/src/tests/ktest_pmm.c`: 1 found
- `./core/kernel/src/tests/ktest_capability.c`: 2 found
- `./core/kernel/src/tests/test_vm_space.c`: 5 found
- `./core/kernel/src/trap/syscall_gate.c`: 1 found
- `./core/kernel/src/trap/trap.c`: 23 found
- `./core/kernel/include/personality_ops.h`: 2 found
- `./core/kernel/include/device.h`: 1 found
- `./core/kernel/include/io_subsys.h`: 1 found
- `./core/kernel/include/capability.h`: 4 found
- `./core/kernel/include/mm.h`: 8 found
- `./core/kernel/include/profile/profile.h`: 2 found
- `./core/kernel/include/core/multikernel.h`: 1 found
- `./core/kernel/include/arch/arch_cpu_caps.h`: 4 found
- `./core/kernel/include/mm/aspace.h`: 1 found
- `./core/kernel/include/mm/vm_object.h`: 2 found
- `./core/kernel/include/mm/pmm.h`: 2 found
- `./core/kernel/include/mm/iommu.h`: 1 found
- `./core/kernel/include/mm/dma.h`: 3 found
- `./core/kernel/include/mm/pmm_pcache.h`: 1 found
- `./core/kernel/include/mm/mm_remote.h`: 1 found
- `./core/kernel/include/mm/vm_space.h`: 2 found
- `./core/kernel/include/security/isolation.h`: 1 found
- `./core/kernel/include/hal/hal_discovery.h`: 2 found
- `./core/kernel/include/hal/hal.h`: 1 found
- `./core/kernel/include/hal/hal_iommu.h`: 1 found
- `./core/kernel/include/hal/hal_pt.h`: 1 found
- `./core/kernel/include/kernel/primitive.h`: 1 found
- `./core/kernel/include/sched/sched.h`: 1 found
- `./core/kernel/include/tests/ktest.h`: 1 found
- `./core/kernel/include/bharat/cpu_local.h`: 3 found
- `./core/kernel/include/trap/syscall_context.h`: 1 found
- `./core/lib/cap/include/bharat/cap/cap_validate.h`: 1 found
- `./core/lib/syscall/common/syscall.c`: 2 found
- `./core/lib/capwire/cap_proxy.c`: 1 found
- `./core/lib/ipc/include/bharat/ipc/ipc.h`: 2 found
- `./core/lib/runtime/backend_dispatch/backend_dispatch.h`: 2 found
- `./core/lib/runtime/backend_dispatch/backend_registry.c`: 1 found
- `./core/lib/include/ipc_user.h`: 4 found
- `./core/personalities/native/native_syscall.c`: 1 found
- `./core/personalities/compat/linux/linux_compat.c`: 9 found
- `./quality/tests/test_capability_misuse.c`: 3 found
- `./quality/tests/benchmark_stubs.c`: 11 found
- `./quality/tests/test_device_framework.c`: 4 found
- `./quality/tests/test_vmm_aspace.c`: 3 found
- `./quality/tests/test_trap_syscall.c`: 4 found
- `./quality/tests/test_pmm_production.c`: 4 found
- `./quality/tests/test_capability_ipc.c`: 4 found
- `./quality/tests/test_early_alloc.c`: 1 found
- `./quality/tests/test_syscall_abi.c`: 1 found
- `./quality/tests/test_ai_kernel_bridge.c`: 3 found
- `./quality/tests/test_tier_a_integration.c`: 9 found
- `./quality/tests/test_vfs_storage.c`: 1 found
- `./quality/tests/test_vm_stress.c`: 2 found
- `./quality/tests/arch/test_x86_process_isolation.c`: 1 found
- `./quality/tests/host/test_sched.c`: 1 found
- `./quality/tests/host/test_init.c`: 8 found
- `./quality/tests/host/test_capability_ownership.c`: 1 found
- `./quality/tests/host/test_secure_mem.c`: 1 found
- `./quality/tests/host/gfx/test_boot_display_bridge.c`: 1 found
- `./quality/tests/host/mm/test_tlb_hardening.c`: 1 found
- `./quality/tests/host/console/test_console.c`: 1 found
- `./quality/tests/host/hal/test_interrupt_common.c`: 1 found
- `./quality/tests/host/fs/test_ramfs_add_child.c`: 2 found
- `./quality/tests/host/fs/test_ramfs_streq.c`: 2 found
- `./quality/tests/host/sched/test_ai_sched_fallback.c`: 2 found
- `./quality/tests/ui/test_shell_navigation.c`: 1 found
- `./experience/user/shell/execplan.c`: 3 found
- `./interface/include/bharat/uapi/servicemgr/contract.h`: 1 found

## 2. Naming Convention Violations

The following files contain variables or functions that appear to use CamelCase instead of the standard `snake_case` naming convention:

- `./core/drivers/bus/usb/usb_core.h`: 10 found
- `./experience/user/ui/fbui/input/fb_input_showcase.c`: 2 found

## 3. Empty or Improper Code Comments

The following files contain empty comments or comments with improper formatting (e.g., standalone `//`, `/* */`, or `/** */`):

- `./core/services/security/crypto/main.c`: 1 found
- `./core/arch/arm/arm64/hal_pt_arm64.c`: 2 found
- `./core/lib/msg/crc.c`: 1 found
- `./core/lib/msg/arch/arm64/crc_ieee.c`: 1 found
- `./quality/tests/test_smp_hal.c`: 2 found
- `./quality/tests/test_timer_common.c`: 4 found

## Recommended Linter Ruleset for Agents

To avoid these issues in the future and manage exceptions, the following ruleset should be adopted by automated agents and linting tools:


1. **Include Placement Rule**:
   - All `#include` directives must appear before any code declarations (structs, typedefs, variables, functions) in the file.
   - *Exception Handling*: If a specific header must be included midway for macro magic or specialized conditional compilation (e.g., `#include "x-macros.def"`), it must be surrounded by an explicit comment like `// NOLINT(include-placement)` or explicitly allowed in the `.clang-tidy` config.

2. **Naming Convention Rule (Snake Case)**:
   - All variables, functions, and types must use `snake_case`. CamelCase or PascalCase is strictly prohibited for standard definitions.
   - *Exception Handling*: For interacting with third-party libraries (e.g., USB standard descriptors, LVGL, NanoVG) that mandate CamelCase names (e.g., `bDescriptorType`), the file or block must be explicitly ignored by the linter using `// NOLINT(readability-identifier-naming)` or configured in `.clang-tidy`.

3. **Comment Formatting Rule**:
   - Comments must contain descriptive text. Empty comments like `//` or `/* */` should be flagged and removed.
   - *Exception Handling*: If used for visual spacing, standard empty lines should be preferred instead of empty comment markers.

4. **Integration with `clang-tidy`**:
   - Update `.clang-tidy` to enforce these rules.
   - For naming conventions, add:
     ```yaml
     CheckOptions:
       - key:             readability-identifier-naming.VariableCase
         value:           lower_case
       - key:             readability-identifier-naming.FunctionCase
         value:           lower_case
     ```
   - Enforce include ordering and placement by ensuring `clang-format` or a dedicated custom check is run in the pre-commit phase.

## Linting rules implementation

Add to `.clang-tidy` file or run periodically.
Exceptions can be handled by `// NOLINT` lines.
