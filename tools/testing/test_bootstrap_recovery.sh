#!/usr/bin/env bash
# Focused BOOT-FLOW-P0-001 checks; build the canonical x86_64 target first.
set -euo pipefail
cd "$(dirname "$0")/../.."
compiler="${CC:-cc}"
generated="${1:-build/x86_64-qemu-desktop-release/generated/include}"
if [[ ! -f "$generated/bharat_config.h" ]]; then
    echo "BLOCKED: missing generated configuration at $generated; build x86_64 first" >&2
    exit 2
fi
out=build/boot-flow-regressions
mkdir -p "$out"
flags=(-std=gnu11 -ffunction-sections -fdata-sections -Wl,--gc-sections)
service_includes=(-Iinterface -Iinterface/include -Icore/lib/runtime/include
    -Icore/lib/cap/include -Icore/lib/ipc/include -Icore/lib/namesvc/include
    -Icore/services/common/runtime/include -Icore/lib/handle)
kernel_includes=(-I"$generated" -Icore/kernel/include -Icore/kernel/src
    -Icore/lib/include -Icore/hal/include -Icore/personalities/runtime/include -Iinterface/include)
pm_includes=(-Iinterface -Iinterface/include -Icore/lib/handle
    -Icore/services/common/runtime/include -Icore/lib/cap/include -Icore/lib/ipc/include)
init_sources=(core/services/core/init/{init_runtime,init_graph,init_events,init_rollback,init_status,init_profile}.c)
pm_sources=(core/services/process_manager/process_manager.c core/lib/handle/handle_table.c
    core/lib/elf/elf_load_plan.c core/lib/elf/elf_parser.c)
build_run() {
    local name="$1"
    shift
    echo "Building $name with $compiler"
    "$compiler" "${flags[@]}" "$@" -o "$out/$name"
    "$out/$name"
}
for name in test_init_failure_handoff test_init_readiness; do
    build_run "$name" "${service_includes[@]}" "quality/tests/init/$name.c" "${init_sources[@]}"
done
build_run test_init "${service_includes[@]}" quality/tests/host/test_init.c \
    core/services/core/init/init_manifest.c "${init_sources[@]}"
build_run test_native_status_boundary "${kernel_includes[@]}" -DBHARAT_PERSONALITY_NATIVE=1 \
    quality/tests/init/test_native_status_boundary.c core/kernel/src/trap/{syscall_gate,syscall_status}.c
build_run test_endpoint_usercopy "${kernel_includes[@]}" quality/tests/init/test_endpoint_usercopy.c \
    core/kernel/src/personality/native/native_syscall_handlers.c core/kernel/src/trap/syscall_status.c
build_run test_x86_process_isolation "${kernel_includes[@]}" quality/tests/arch/test_x86_process_isolation.c
build_run test_boot_cpu_inventory "${kernel_includes[@]}" -Icore/boot/include -DBHARAT_KERNEL_PROFILE_RT=1 \
    quality/tests/init/test_boot_cpu_inventory.c core/boot/discovery/smp_boot.c core/hal/common/discovery.c
build_run test_boot_reservation_overlap "${kernel_includes[@]}" -Icore/boot/include -DBHARAT_HOST_TEST \
    quality/tests/init/test_boot_reservation_overlap.c core/kernel/src/mm/pmm/early_alloc.c core/kernel/src/mm/pmm/pmm_init.c
build_run test_boot_progress -Iinterface/include quality/tests/ui/test_boot_progress.c \
    experience/user/ui/fbui/widgets/fb_widgets.c
for name in test_pm_spawn_transaction test_procvm_stress; do
    build_run "$name" "${pm_includes[@]}" "quality/tests/host/process_vm/$name.c" "${pm_sources[@]}"
done
printf '%s\n' 'void hal_serial_write(const char *s) { (void)s; }' > "$out/serial_stub.c"
build_run test_boot_validate -DBHARAT_HOST_TEST -Icore/boot/include \
    quality/tests/host/boot/test_boot_validate.c core/boot/src/{boot_info,boot_validate}.c "$out/serial_stub.c"
cmake -P quality/tests/init/test_headless_component_policy.cmake
bash tools/testing/test_check_boot_log.sh
echo "PASS: bootstrap recovery regressions"
