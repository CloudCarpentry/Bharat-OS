cmake_minimum_required(VERSION 3.20)
project(BharatOS-Tests C)

enable_testing()

if(EXISTS "${CMAKE_SOURCE_DIR}/core/kernel" AND EXISTS "${CMAKE_SOURCE_DIR}/core/tests")
    set(REPO_ROOT "${CMAKE_SOURCE_DIR}")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/../kernel")
    get_filename_component(REPO_ROOT "${CMAKE_SOURCE_DIR}/.." ABSOLUTE)
else()
    set(REPO_ROOT "${CMAKE_SOURCE_DIR}")
endif()
set(WORK_DIR "${CMAKE_BINARY_DIR}/tests/policy_matrix")

add_subdirectory(host)
add_subdirectory(benchmark)

# ---------------------------------------------------------
# Shared Scheduler Test Mocks
# ---------------------------------------------------------
if(CMAKE_SYSTEM_NAME STREQUAL "Generic")
    # This file shouldn't be processed if we are building the kernel (cross-compiling)
    # The top level CMakeLists.txt guards it, but just in case:
    return()
endif()

add_library(bharat_test_sched_mocks OBJECT
    ../kernel/src/sched_stub.c
    ../core/lib/rbtree/rbtree.c
    ../kernel/src/sched/ai_sched.c
    ../kernel/src/sched/sched_deg.c
    ../kernel/src/sched/algo_matrix.c
    ../kernel/src/capability.c
    ../kernel/src/cap/cap_policy.c
    ../kernel/src/device/device_manager.c
    ../kernel/src/mm/pmm/numa.c
    benchmark_stubs.c
    host/mock_panic.c
)
target_include_directories(bharat_test_sched_mocks PRIVATE
    ../kernel/include
    ../include
    ../core/boot/include
    ../core/lib/include
)
target_compile_definitions(bharat_test_sched_mocks PRIVATE TESTING=1)


# Memory HAL portability tests
add_executable(test_hal_pt_common
    mm/pt_common_tests.c
    ../hal/hal_pt.c
)
target_include_directories(test_hal_pt_common PRIVATE ../kernel/include ../include ../core/boot/include)
add_test(NAME test_hal_pt_common COMMAND test_hal_pt_common)

add_executable(test_hal_pt_fallbacks
    test_hal_pt_fallbacks.c
    ../hal/hal_pt.c
)
target_include_directories(test_hal_pt_fallbacks PRIVATE ../kernel/include ../include ../core/boot/include)
add_test(NAME test_hal_pt_fallbacks COMMAND test_hal_pt_fallbacks)

add_executable(test_hal_caps_consistency
    test_hal_caps_consistency.c
    ../hal/hal_pt.c
)
target_include_directories(test_hal_caps_consistency PRIVATE ../kernel/include ../include ../core/boot/include)
add_test(NAME test_hal_caps_consistency COMMAND test_hal_caps_consistency)

add_executable(test_arch_syscall
    test_arch_syscall.c
    ../core/arch/x86/x86_64/syscall_extract.c
)
target_include_directories(test_arch_syscall PRIVATE
    ../kernel/include
    ../include
    ../core/lib/include
)
target_compile_definitions(test_arch_syscall PRIVATE TRAP_FRAME_DEFINED=1)
add_test(NAME test_arch_syscall COMMAND test_arch_syscall)

# The following legacy tests are preserved here but should eventually
# migrate to kernel/src/test or tests/host as appropriate.
add_executable(test_panic test_panic.c ../kernel/src/panic.c ../kernel/src/fault_diag.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)

add_executable(test_stress_concurrency test_stress_concurrency.c
    ../kernel/src/ipc/endpoint_ipc.c
    $<TARGET_OBJECTS:bharat_test_sched_mocks>
)
target_include_directories(test_stress_concurrency PRIVATE ../kernel/include ../include ../core/lib/include)
target_compile_definitions(test_stress_concurrency PRIVATE TESTING=1)
target_compile_options(test_stress_concurrency PRIVATE -g -O0)
target_link_libraries(test_stress_concurrency PRIVATE pthread)
add_test(NAME test_stress_concurrency COMMAND test_stress_concurrency)
target_include_directories(test_panic PRIVATE ../kernel/include ../include ../core/lib/include ../core/boot/include)
add_test(NAME test_panic COMMAND test_panic)

add_executable(test_hal_interrupts test_hal_interrupts.c ../hal/interrupt_common.c)
target_include_directories(test_hal_interrupts PRIVATE ../kernel/include ../include)
add_test(NAME test_hal_interrupts COMMAND test_hal_interrupts)


add_executable(test_urpc_init_channel test_urpc_init_channel.c ../core/lib/urpc/urpc.c)
target_include_directories(test_urpc_init_channel PRIVATE ../core/lib/urpc)
add_test(NAME test_urpc_init_channel COMMAND test_urpc_init_channel)

add_executable(test_lib_ipc_contract
    test_lib_ipc_contract.c
    ../core/lib/ipc/src/ipc.c
)
target_include_directories(test_lib_ipc_contract PRIVATE
    ../include
    ../core/lib/include
    ../core/lib/cap/include
    ../core/lib/ipc/include
)
add_test(NAME test_lib_ipc_contract COMMAND test_lib_ipc_contract)

add_executable(test_urpc_ring test_urpc_ring.c ../kernel/src/ipc/multikernel.c ../kernel/src/ipc/mk_dispatch.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
add_executable(test_ipc_async_timeout test_ipc_async_timeout.c ../kernel/src/ipc/async_ipc.c ../kernel/src/ipc/ipc_timeout.c)
target_include_directories(test_ipc_async_timeout PRIVATE ../kernel/include ../core/lib/include)
add_test(NAME test_ipc_async_timeout COMMAND test_ipc_async_timeout)

if (EXISTS "${CMAKE_SOURCE_DIR}/core/personalities/domain/automotive/automotive.c")
    add_executable(test_automotive_subsys
        test_automotive_subsys.c
        ../core/personalities/domain/automotive/automotive.c
        ../core/drivers/devices/can_loopback/can_loopback.c
    )
    target_include_directories(test_automotive_subsys PRIVATE ../core/services/core/subsysmgr/include ../kernel/include ../core/drivers/include)
    add_test(NAME test_automotive_subsys COMMAND test_automotive_subsys)
endif()

target_include_directories(test_urpc_ring PRIVATE ../kernel/include ../core/lib/include ../kernel/include/generated)
add_test(NAME test_urpc_ring COMMAND test_urpc_ring)

add_executable(test_scheduler test_scheduler.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
target_include_directories(test_scheduler PRIVATE ../kernel/include ../core/lib/include)
target_compile_definitions(test_scheduler PRIVATE TESTING=1)
add_test(NAME test_scheduler COMMAND test_scheduler)

if (EXISTS "${CMAKE_SOURCE_DIR}/core/personalities/compat/linux/linux_compat.c")
    add_executable(test_trap_syscall test_trap_syscall.c ../kernel/src/trap/trap.c ../kernel/src/trap/syscall.c ../kernel/src/trap/syscall_gate.c ../kernel/src/trap/syscall_stats.c ../kernel/src/ipc/endpoint_ipc.c ../core/personalities/compat/linux/linux_compat.c ../core/personalities/common/personality_registry.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
    target_include_directories(test_trap_syscall PRIVATE ../kernel/include ../core/services/core/subsysmgr/include ../include ../core/lib/include ../core/boot/include ../subsys/include)
    target_compile_definitions(test_trap_syscall PRIVATE TESTING=1)
    add_test(NAME test_trap_syscall COMMAND test_trap_syscall)
    # Compatibility alias for common typo used in ad-hoc commands.
    add_custom_target(test_trap_syscal DEPENDS test_trap_syscall)
endif()

add_executable(test_capability_ipc test_capability_ipc.c ../kernel/src/ipc/endpoint_ipc.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
target_include_directories(test_capability_ipc PRIVATE ../kernel/include ../core/lib/include)
target_compile_definitions(test_capability_ipc PRIVATE TESTING=1)
add_test(NAME test_capability_ipc COMMAND test_capability_ipc)

add_executable(test_ipc_endpoint_profile_limits test_ipc_endpoint_profile_limits.c ../kernel/src/ipc/endpoint_ipc.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
target_include_directories(test_ipc_endpoint_profile_limits PRIVATE ../kernel/include ../core/lib/include)
target_compile_definitions(test_ipc_endpoint_profile_limits PRIVATE TESTING=1)
add_test(NAME test_ipc_endpoint_profile_limits COMMAND test_ipc_endpoint_profile_limits)

add_executable(test_device_framework test_device_framework.c ../kernel/src/device/builtin_drivers.c ../kernel/src/subsystem/profile.c ../kernel/src/device/pci.c ../kernel/src/ipc/zero_copy_io.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
target_include_directories(test_device_framework PRIVATE ../kernel/include ../core/lib/include)
target_compile_definitions(test_device_framework PRIVATE TESTING=1)
add_test(NAME test_device_framework COMMAND test_device_framework)

add_executable(test_multikernel_topology test_multikernel_topology.c ../kernel/src/ipc/multikernel.c ../kernel/src/ipc/mk_dispatch.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
target_include_directories(test_multikernel_topology PRIVATE ../kernel/include ../core/lib/include)
add_test(NAME test_multikernel_topology COMMAND test_multikernel_topology)

add_executable(test_capability_misuse test_capability_misuse.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
target_include_directories(test_capability_misuse PRIVATE ../kernel/include ../core/lib/include)
target_compile_definitions(test_capability_misuse PRIVATE TESTING=1)
add_test(NAME test_capability_misuse COMMAND test_capability_misuse)

add_executable(test_ui_device_class_contract test_ui_device_class_contract.c)
target_include_directories(test_ui_device_class_contract PRIVATE ../include)
add_test(NAME test_ui_device_class_contract COMMAND test_ui_device_class_contract)

add_executable(test_ui_abi test_ui_abi.c)
target_include_directories(test_ui_abi PRIVATE ../include)
add_test(NAME test_ui_abi COMMAND test_ui_abi)

add_executable(test_headless_compositor test_headless_compositor.c)
target_include_directories(test_headless_compositor PRIVATE ../include ../)
target_link_libraries(test_headless_compositor PRIVATE bharat_headless_compositor)
add_test(NAME test_headless_compositor COMMAND test_headless_compositor)

add_executable(test_tiny_ui_stack
    test_tiny_ui_stack.c
    ../core/stacks/ui/lcd/tiny_ui.c
)
target_include_directories(test_tiny_ui_stack PRIVATE ../include)
add_test(NAME test_tiny_ui_stack COMMAND test_tiny_ui_stack)

add_executable(test_memory_edgecases test_memory_edgecases.c ../kernel/src/mm/vm/aspace/vm_space.c ../kernel/src/mm/zswap.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
target_include_directories(test_memory_edgecases PRIVATE ../kernel/include ../core/lib/include)
target_compile_definitions(test_memory_edgecases PRIVATE TESTING=1)
add_test(NAME test_memory_edgecases COMMAND test_memory_edgecases)

if (EXISTS "${CMAKE_SOURCE_DIR}/core/personalities/compat/linux/linux_compat.c")
    add_executable(test_tier_a_integration test_tier_a_integration.c ../kernel/src/trap/trap.c ../kernel/src/trap/syscall.c ../kernel/src/ipc/endpoint_ipc.c ../core/personalities/compat/linux/linux_compat.c ../core/personalities/common/personality_registry.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
    target_include_directories(test_tier_a_integration PRIVATE ../kernel/include ../core/services/core/subsysmgr/include ../include ../subsys/include ../core/lib/include ../core/boot/include)
    target_compile_definitions(test_tier_a_integration PRIVATE TESTING=1)
    add_test(NAME test_tier_a_integration COMMAND test_tier_a_integration)
endif()

add_executable(test_riscv_shakti_bsp test_riscv_shakti_bsp.c ../arch/hal/riscv/shakti_bsp.c ../hal/common/fdt_parser.c)
target_include_directories(test_riscv_shakti_bsp PRIVATE ../kernel/include ../include ../core/boot/include)
add_test(NAME test_riscv_shakti_bsp COMMAND test_riscv_shakti_bsp)

add_executable(test_ai_governor test_ai_governor.c ../user/apps/ai_governor/ai_governor.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
target_include_directories(test_ai_governor PRIVATE ../kernel/include ../core/lib/include)
target_compile_definitions(test_ai_governor PRIVATE TESTING=1)
add_test(NAME test_ai_governor COMMAND test_ai_governor)

add_executable(test_ai_kernel_bridge test_ai_kernel_bridge.c ../kernel/src/mm/vm/aspace/vm_space.c ../kernel/src/ipc/endpoint_ipc.c ../hal/timer_common.c ../core/kernel/staging/ai/ai_kernel_bridge.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
target_include_directories(test_ai_kernel_bridge PRIVATE ../kernel/include ../core/lib/include)
target_compile_definitions(test_ai_kernel_bridge PRIVATE TESTING=1)
add_test(NAME test_ai_kernel_bridge COMMAND test_ai_kernel_bridge)

add_executable(test_secure_boot_policy test_secure_boot_policy.c ../kernel/src/security/secure_boot.c ../kernel/src/security/audit.c)
target_include_directories(test_secure_boot_policy PRIVATE ../kernel/include)
add_test(NAME test_secure_boot_policy COMMAND test_secure_boot_policy)

add_executable(test_capability_policy test_capability_policy.c benchmark_stubs.c host/mock_panic.c
    ../kernel/src/mm/vm/aspace/vm_space.c
    ../kernel/src/sched_stub.c
    ../kernel/src/capability.c
    ../kernel/src/cap/cap_policy.c
    ../kernel/src/ipc/endpoint_ipc.c
    ../kernel/src/device/device_manager.c
    ../kernel/src/mm/pmm/numa.c
    ../kernel/src/mm/zswap.c
    ../core/lib/rbtree/rbtree.c
    ../kernel/src/sched/ai_sched.c
    ../kernel/src/sched/sched_deg.c
    ../kernel/src/sched/algo_matrix.c
)
target_include_directories(test_capability_policy PRIVATE ../kernel/include ../core/lib/include)
add_test(NAME test_capability_policy COMMAND test_capability_policy)

if (EXISTS "${CMAKE_SOURCE_DIR}/core/personalities/compat/linux/linux_compat.c")
    add_executable(test_posix
        test_posix.c
        ../core/personalities/compat/linux/linux_compat.c
        ../core/personalities/common/translation_events.c
        subsys/test_personality_stubs.c
    )
    target_include_directories(test_posix PRIVATE ../kernel/include ../core/lib/include ../core/services/core/subsysmgr/include)
    add_test(NAME test_posix COMMAND test_posix)
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/core/personalities/compat/windows/win_compat.c")
    add_executable(test_windows_compat_init test_windows_compat_init.c ../core/personalities/compat/windows/win_compat.c)
    target_include_directories(test_windows_compat_init PRIVATE ../kernel/include ../core/lib/include ../core/services/core/subsysmgr/include)
    add_test(NAME test_windows_compat_init COMMAND test_windows_compat_init)
endif()

add_executable(test_subsys_manager test_subsys_manager.c ../core/services/core/subsysmgr/manager.c ../core/personalities/compat/linux/linux_compat.c ../core/personalities/compat/windows/win_compat.c ../platform/common/platform_cpu_topology.c)
target_include_directories(test_subsys_manager PRIVATE ../kernel/include ../core/lib/include ../core/services/core/subsysmgr/include ../hal/include)
add_test(NAME test_subsys_manager COMMAND test_subsys_manager)

if (EXISTS "${CMAKE_SOURCE_DIR}/core/personalities/compat/android/android_personality.c")
    add_executable(test_android_personality
        subsys/test_android_personality.c
        ../core/personalities/compat/android/android_personality.c
        ../core/personalities/compat/android/android_binder_dist.c
        ../core/personalities/compat/android/android_boot.c
        ../core/personalities/compat/android/android_process.c
        ../core/personalities/compat/android/android_security.c
        ../core/personalities/common/translation_events.c
        subsys/test_personality_stubs.c
    )
    target_include_directories(test_android_personality PRIVATE ${CMAKE_SOURCE_DIR}/core/kernel/include ${CMAKE_SOURCE_DIR}/core/lib/include ${CMAKE_SOURCE_DIR}/core/services/core/subsysmgr/include)
    add_test(NAME test_android_personality COMMAND test_android_personality)

    if (EXISTS "${CMAKE_SOURCE_DIR}/core/personalities/compat/linux/linux_compat.c")
        add_executable(test_personality_translation_guards
            subsys/test_personality_translation_guards.c
            ../core/personalities/compat/linux/linux_compat.c
            ../core/personalities/compat/android/android_binder_dist.c
            ../core/personalities/common/translation_events.c
        )
        target_include_directories(test_personality_translation_guards PRIVATE ${CMAKE_SOURCE_DIR}/core/kernel/include ${CMAKE_SOURCE_DIR}/core/lib/include ${CMAKE_SOURCE_DIR}/core/services/core/subsysmgr/include)
        add_test(NAME test_personality_translation_guards COMMAND test_personality_translation_guards)
    endif()
endif()

add_executable(test_vm_stress test_vm_stress.c benchmark_stubs.c host/mock_panic.c ../kernel/src/mm/vm/aspace/vm_space.c ../core/lib/rbtree/rbtree.c)
target_include_directories(test_vm_stress PRIVATE ../kernel/include ../core/lib/include)
add_test(NAME test_vm_stress COMMAND test_vm_stress)

add_executable(test_ipc_fuzz test_ipc_fuzz.c ../kernel/src/ipc/endpoint_ipc.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
target_include_directories(test_ipc_fuzz PRIVATE ../kernel/include ../core/lib/include)
add_test(NAME test_ipc_fuzz COMMAND test_ipc_fuzz)

add_executable(test_ipc_sync_sender_wakeup test_ipc_sync_sender_wakeup.c ../kernel/src/ipc/endpoint_ipc.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
target_include_directories(test_ipc_sync_sender_wakeup PRIVATE ../kernel/include ../core/lib/include)
add_test(NAME test_ipc_sync_sender_wakeup COMMAND test_ipc_sync_sender_wakeup)

add_executable(test_ipc_sync_receiver_wakeup test_ipc_sync_receiver_wakeup.c ../kernel/src/ipc/endpoint_ipc.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
target_include_directories(test_ipc_sync_receiver_wakeup PRIVATE ../kernel/include ../core/lib/include)
add_test(NAME test_ipc_sync_receiver_wakeup COMMAND test_ipc_sync_receiver_wakeup)

add_executable(test_ipc_multiple_waiters_fifo test_ipc_multiple_waiters_fifo.c ../kernel/src/ipc/endpoint_ipc.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
target_include_directories(test_ipc_multiple_waiters_fifo PRIVATE ../kernel/include ../core/lib/include)
add_test(NAME test_ipc_multiple_waiters_fifo COMMAND test_ipc_multiple_waiters_fifo)

add_executable(test_ipc_no_spurious_self_wakeup test_ipc_no_spurious_self_wakeup.c ../kernel/src/ipc/endpoint_ipc.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
target_include_directories(test_ipc_no_spurious_self_wakeup PRIVATE ../kernel/include ../core/lib/include)
add_test(NAME test_ipc_no_spurious_self_wakeup COMMAND test_ipc_no_spurious_self_wakeup)

add_executable(test_security_isolation test_security_isolation.c ../kernel/src/ipc/endpoint_ipc.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
target_include_directories(test_security_isolation PRIVATE ../kernel/include ../core/lib/include)
add_test(NAME test_security_isolation COMMAND test_security_isolation)

# Benchmarks

add_executable(test_bench_ipc test_bench_ipc.c ../kernel/src/benchmark/benchmark.c ../kernel/src/ipc/endpoint_ipc.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
target_include_directories(test_bench_ipc PRIVATE ../kernel/include ../core/lib/include)
target_compile_definitions(test_bench_ipc PRIVATE TESTING=1)
add_test(NAME test_bench_ipc COMMAND test_bench_ipc)

add_executable(test_bench_sched test_bench_sched.c ../kernel/src/benchmark/benchmark.c ../kernel/src/ipc/endpoint_ipc.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
target_include_directories(test_bench_sched PRIVATE ../kernel/include ../core/lib/include)
target_compile_definitions(test_bench_sched PRIVATE TESTING=1)
add_test(NAME test_bench_sched COMMAND test_bench_sched)

add_executable(test_bench_sched_timer_tick test_bench_sched_timer_tick.c ../kernel/src/benchmark/benchmark.c ../kernel/src/ipc/endpoint_ipc.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
target_include_directories(test_bench_sched_timer_tick PRIVATE ../kernel/include ../core/lib/include)
target_compile_definitions(test_bench_sched_timer_tick PRIVATE TESTING=1)
add_test(NAME test_bench_sched_timer_tick COMMAND test_bench_sched_timer_tick)

add_executable(test_bench_zswap test_bench_zswap.c ../kernel/src/benchmark/benchmark.c ../kernel/src/mm/zswap.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
target_include_directories(test_bench_zswap PRIVATE ../kernel/include ../core/lib/include)
target_compile_definitions(test_bench_zswap PRIVATE TESTING=1)
add_test(NAME test_bench_zswap COMMAND test_bench_zswap)


add_executable(test_vfs_storage test_vfs_storage.c ../core/lib/fs/fs_client.c ../core/services/system/filesystem/vfs_stub.c ../core/stacks/storage/fs/blob/blob_backend.c ../core/services/system/filesystem/mount_mgr.c ../core/services/system/filesystem/fd_mgr.c ../core/services/system/filesystem/namespace_mgr.c ../kernel/src/lib/base/string.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
target_include_directories(test_vfs_storage PRIVATE ../kernel/include ../include ../core/lib/include)
target_compile_definitions(test_vfs_storage PRIVATE TESTING=1)
add_test(NAME test_vfs_storage COMMAND test_vfs_storage)

add_executable(test_vfs_file_caps test_vfs_file_caps.c ../core/lib/fs/fs_client.c ../core/services/system/filesystem/fd_mgr.c ../core/services/system/filesystem/vfs_stub.c ../core/services/system/filesystem/mount_mgr.c ../core/services/system/filesystem/namespace_mgr.c ../kernel/src/lib/base/string.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
target_include_directories(test_vfs_file_caps PRIVATE ../kernel/include ../include ../core/lib/include)
target_compile_definitions(test_vfs_file_caps PRIVATE TESTING=1)
add_test(NAME test_vfs_file_caps COMMAND test_vfs_file_caps)

add_executable(test_object_store test_object_store.c ../kernel/src/fs/object_store.c)
target_include_directories(test_object_store PRIVATE ../kernel/include)
target_compile_definitions(test_object_store PRIVATE TESTING=1)
add_test(NAME test_object_store COMMAND test_object_store)

add_executable(test_subsystem_profile test_subsystem_profile.c ../kernel/src/subsystem/profile.c)
target_include_directories(test_subsystem_profile PRIVATE ../kernel/include)
target_compile_definitions(test_subsystem_profile PRIVATE BHARAT_PROFILE_DATACENTER=1 BHARAT_PERSONALITY_LINUX=1 BHARAT_ARCH_X86=1)
add_test(NAME test_subsystem_profile COMMAND test_subsystem_profile)

add_executable(test_builtin_drivers_profile test_builtin_drivers_profile.c ../kernel/src/device/builtin_drivers.c ../kernel/src/subsystem/profile.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
target_include_directories(test_builtin_drivers_profile PRIVATE ../kernel/include ../core/lib/include)
target_compile_definitions(test_builtin_drivers_profile PRIVATE TESTING=1 BHARAT_PROFILE_DATACENTER=1 BHARAT_PERSONALITY_LINUX=1 BHARAT_ARCH_X86=1)
add_test(NAME test_builtin_drivers_profile COMMAND test_builtin_drivers_profile)

# Integration Subsystem Tests
if (EXISTS "${CMAKE_SOURCE_DIR}/core/personalities/compat/linux/linux_compat.c" AND EXISTS "${CMAKE_SOURCE_DIR}/core/personalities/domain/automotive/automotive.c")
    add_executable(test_integration_core_subsys
        test_integration_core_subsys.c
        ../core/personalities/compat/linux/linux_compat.c
        ../core/personalities/domain/automotive/automotive.c
        ../user/ui/fbui/core/fb_render.c
        $<TARGET_OBJECTS:bharat_test_sched_mocks>
    )
    target_include_directories(test_integration_core_subsys PRIVATE ../kernel/include ../core/services/core/subsysmgr/include ../include ../core/lib/include ../core/boot/include)
    target_compile_definitions(test_integration_core_subsys PRIVATE TESTING=1 BHARAT_PROFILE_DATACENTER=1 BHARAT_PERSONALITY_LINUX=1 BHARAT_ARCH_X86=1)
    add_test(NAME test_integration_core_subsys COMMAND test_integration_core_subsys)
endif()

# Edge Profile Subsystem Tests
if (EXISTS "${CMAKE_SOURCE_DIR}/core/personalities/domain/automotive/automotive.c")
    add_executable(test_profile_edge
        test_profile_edge.c
        ../core/personalities/domain/automotive/automotive.c
        ../user/ui/fbui/core/fb_render.c
        $<TARGET_OBJECTS:bharat_test_sched_mocks>
    )
    target_include_directories(test_profile_edge PRIVATE ../kernel/include ../core/services/core/subsysmgr/include ../include ../core/lib/include ../core/boot/include)
    target_compile_definitions(test_profile_edge PRIVATE TESTING=1 BHARAT_PROFILE_EDGE=1 BHARAT_PERSONALITY_AUTOMOTIVE=1 BHARAT_ARCH_ARM=1)
    add_test(NAME test_profile_edge COMMAND test_profile_edge)
endif()


add_executable(test_scheduler_hello_world test_scheduler_hello_world.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
target_include_directories(test_scheduler_hello_world PRIVATE ../kernel/include ../core/lib/include)
target_compile_definitions(test_scheduler_hello_world PRIVATE TESTING=1)
add_test(NAME test_scheduler_hello_world COMMAND test_scheduler_hello_world)

add_executable(test_dma_iommu test_dma_iommu.c ../kernel/src/security/isolation.c ../kernel/src/security/vfio.c ../kernel/src/security/audit.c)
target_include_directories(test_dma_iommu PRIVATE ../kernel/include)

add_executable(test_iommu_stub test_iommu_stub.c ../kernel/src/mm/iommu/iommu_stub.c ../kernel/src/lib/base/status.c)
target_include_directories(test_iommu_stub PRIVATE ../kernel/include ../core/lib/include ../include)
add_test(NAME test_iommu_stub COMMAND test_iommu_stub)

add_executable(test_dma_lifecycle_host test_dma_lifecycle_host.c
    ../kernel/src/mm/dma/dma.c
    ../kernel/src/mm/iommu/iommu_domain.c
    ../kernel/src/capability.c
    ../hal/dma_coherency.c)
target_include_directories(test_dma_lifecycle_host PRIVATE ../kernel/include ../include ../core/lib/include)
add_test(NAME test_dma_lifecycle_host COMMAND test_dma_lifecycle_host)

add_executable(test_smp_hal test_smp_hal.c)
target_include_directories(test_smp_hal PRIVATE ../kernel/include)
add_test(NAME test_smp_hal COMMAND test_smp_hal)

add_executable(test_pmm_production test_pmm_production.c host/mock_panic.c ../kernel/src/mm/pmm/pmm.c ../kernel/src/mm/pmm/pmm_pcache.c)
target_include_directories(test_pmm_production PRIVATE ../kernel/include ../kernel/src ../include ../core/lib/include ../core/boot/include)
add_test(NAME test_pmm_production COMMAND test_pmm_production)

add_executable(test_pmm_zone_alloc test_pmm_zone_alloc.c host/mock_panic.c ../kernel/src/mm/pmm/pmm.c ../kernel/src/mm/pmm/pmm_pcache.c)
target_include_directories(test_pmm_zone_alloc PRIVATE ../kernel/include ../kernel/src ../include ../core/lib/include ../core/boot/include)
add_test(NAME test_pmm_zone_alloc COMMAND test_pmm_zone_alloc)

add_executable(test_pmm_refpin test_pmm_refpin.c host/mock_panic.c ../kernel/src/mm/pmm/pmm.c ../kernel/src/mm/pmm/pmm_pcache.c)
target_include_directories(test_pmm_refpin PRIVATE ../kernel/include ../kernel/src ../include ../core/lib/include ../core/boot/include)
add_test(NAME test_pmm_refpin COMMAND test_pmm_refpin)

add_executable(test_sched_reap test_sched_reap.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
target_include_directories(test_sched_reap PRIVATE ../kernel/include ../core/lib/include)
target_compile_definitions(test_sched_reap PRIVATE TESTING=1)
add_test(NAME test_sched_reap COMMAND test_sched_reap)

add_executable(test_benchmark_hw_caps test_benchmark_hw_caps.c ../kernel/src/benchmark/benchmark.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
target_include_directories(test_benchmark_hw_caps PRIVATE ../kernel/include ../core/lib/include)
target_compile_definitions(test_benchmark_hw_caps PRIVATE TESTING=1)
add_test(NAME test_benchmark_hw_caps COMMAND test_benchmark_hw_caps)

add_executable(test_syscall_abi test_syscall_abi.c)
target_include_directories(test_syscall_abi PRIVATE
    ${CMAKE_SOURCE_DIR}/interface/include
    ${CMAKE_SOURCE_DIR}/core/kernel/include
)
add_test(NAME SyscallABITest COMMAND test_syscall_abi)

add_executable(test_elf_parser test_elf_parser.c)
target_include_directories(test_elf_parser PRIVATE ${CMAKE_SOURCE_DIR}/interface/include)
target_link_libraries(test_elf_parser PRIVATE elf_parser)
add_test(NAME ELFParserTest COMMAND test_elf_parser)
add_executable(test_storage_profile test_storage_profile.c ../core/stacks/storage/profile.c)
target_include_directories(test_storage_profile PRIVATE ../include)
add_test(NAME test_storage_profile COMMAND test_storage_profile)

add_executable(test_vfs_ramfs test_vfs_ramfs.c ../core/lib/fs/fs_client.c ../core/stacks/storage/fs/ramfs/ramfs.c ../core/services/system/filesystem/vfs_stub.c ../core/services/system/filesystem/mount_mgr.c ../core/services/system/filesystem/fd_mgr.c ../core/services/system/filesystem/namespace_mgr.c ../kernel/src/lib/base/string.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
target_include_directories(test_vfs_ramfs PRIVATE ../kernel/include ../include ../core/lib/include)
target_compile_definitions(test_vfs_ramfs PRIVATE TESTING=1)
add_test(NAME test_vfs_ramfs COMMAND test_vfs_ramfs)



set(BHARAT_NOSTDLIB_TEST_TARGETS
    test_panic
    test_stress_concurrency
    test_hal_interrupts
    test_urpc_init_channel
    test_urpc_ring
    test_ipc_async_timeout
    test_automotive_subsys
    test_scheduler
    test_trap_syscall
    test_capability_ipc
    test_device_framework
    test_multikernel_topology
    test_capability_misuse
    test_memory_edgecases
    test_tier_a_integration
    test_riscv_shakti_bsp
    test_ai_governor
    test_ai_kernel_bridge
    test_secure_boot_policy
    test_capability_policy
    test_posix
    test_windows_compat_init
    test_subsys_manager
    test_vm_stress
    test_ipc_fuzz
    test_ipc_sync_sender_wakeup
    test_ipc_sync_receiver_wakeup
    test_ipc_multiple_waiters_fifo
    test_ipc_no_spurious_self_wakeup
    test_security_isolation
    test_bench_ipc
    test_bench_sched
    test_bench_sched_timer_tick
    test_bench_zswap
    test_vfs_storage
    test_vfs_file_caps
    test_object_store
    test_subsystem_profile
    test_builtin_drivers_profile
    test_integration_core_subsys
    test_profile_edge
    test_scheduler_hello_world
    test_dma_iommu
    test_iommu_stub
    test_dma_lifecycle_host
    test_smp_hal
    test_pmm_production
    test_pmm_zone_alloc
    test_pmm_refpin
    test_sched_reap
    test_benchmark_hw_caps
    test_syscall_abi
    test_elf_parser
    test_vfs_ramfs
    test_storage_profile
    test_boot_e2e
    test_android_personality
    test_arch_syscall
)

foreach(test_target IN LISTS BHARAT_NOSTDLIB_TEST_TARGETS)
    if(TARGET ${test_target})
        target_link_options(${test_target} PRIVATE "LINKER:-no-pie" "-fno-pie")
    endif()
endforeach()

add_executable(test_boot_e2e test_boot_e2e.c test_arch_caps.c ../kernel/src/kernel_boot.c ../kernel/src/lib/base/string.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
target_include_directories(test_boot_e2e PRIVATE ../kernel/include ../include ../core/lib/include ../core/boot/include)
add_test(NAME boot_e2e COMMAND test_boot_e2e)

add_executable(test_init_manifest host/test_init_manifest.c ../core/services/core/init/init_events.c)
target_include_directories(test_init_manifest PRIVATE ../include ../core/lib/runtime/include ../core/lib/cap/include ../core/lib/ipc/include)
add_test(NAME test_init_manifest COMMAND test_init_manifest)

add_executable(test_execution_mode_single_core host/test_execution_mode_single_core.c ../kernel/src/sched/cpu_partition.c ../kernel/src/sched/execution_mode.c ../kernel/src/sched/sched_class.c ../kernel/src/sched/sched_topology.c ../platform/common/platform_cpu_topology.c)
target_include_directories(test_execution_mode_single_core PRIVATE ../kernel/include ../include ../hal/include ../)
target_compile_definitions(test_execution_mode_single_core PRIVATE TESTING=1)
add_test(NAME test_execution_mode_single_core COMMAND test_execution_mode_single_core)

add_executable(test_execution_mode_dual_core host/test_execution_mode_dual_core.c ../kernel/src/sched/cpu_partition.c ../kernel/src/sched/execution_mode.c ../kernel/src/sched/sched_class.c ../kernel/src/sched/sched_topology.c ../platform/common/platform_cpu_topology.c)
target_include_directories(test_execution_mode_dual_core PRIVATE ../kernel/include ../include ../hal/include ../)
target_compile_definitions(test_execution_mode_dual_core PRIVATE TESTING=1)
add_test(NAME test_execution_mode_dual_core COMMAND test_execution_mode_dual_core)

add_executable(test_execution_mode_quad_core host/test_execution_mode_quad_core.c ../kernel/src/sched/cpu_partition.c ../kernel/src/sched/execution_mode.c ../kernel/src/sched/sched_class.c ../kernel/src/sched/sched_topology.c ../platform/common/platform_cpu_topology.c)
target_include_directories(test_execution_mode_quad_core PRIVATE ../kernel/include ../include ../hal/include ../)
target_compile_definitions(test_execution_mode_quad_core PRIVATE TESTING=1)
add_test(NAME test_execution_mode_quad_core COMMAND test_execution_mode_quad_core)

add_executable(test_irq_domain host/test_irq_domain_host.c ../kernel/src/tests/ktest_irq_domain.c ../kernel/src/device/irq_domain.c)
target_include_directories(test_irq_domain PRIVATE ../kernel/include ../include ../hal/include ../core/boot/include ../)
target_compile_definitions(test_irq_domain PRIVATE TESTING=1 HOST_TEST_BUILD)
add_test(NAME test_irq_domain COMMAND test_irq_domain)

add_executable(test_sched_partition_validation host/test_sched_partition_validation.c ../kernel/src/sched/cpu_partition.c ../kernel/src/sched/execution_mode.c ../kernel/src/sched/sched_class.c ../kernel/src/sched/sched_topology.c ../platform/common/platform_cpu_topology.c)
target_include_directories(test_sched_partition_validation PRIVATE ../kernel/include ../include ../hal/include ../)
target_compile_definitions(test_sched_partition_validation PRIVATE TESTING=1)
add_test(NAME test_sched_partition_validation COMMAND test_sched_partition_validation)

# Benchmarks
add_executable(test_bench_hal_pt_arm64 benchmark/test_bench_hal_pt_arm64.c ../kernel/src/benchmark/benchmark.c $<TARGET_OBJECTS:bharat_test_sched_mocks>)
target_include_directories(test_bench_hal_pt_arm64 PRIVATE ../kernel/include ../include ../core/lib/include)
target_compile_definitions(test_bench_hal_pt_arm64 PRIVATE TESTING=1)
add_test(NAME test_bench_hal_pt_arm64 COMMAND test_bench_hal_pt_arm64)

# Message/Wire Tests
add_executable(test_wire_format test_wire_format.c)
target_include_directories(test_wire_format PRIVATE ../core/services/core/subsysmgr/include)
target_link_libraries(test_wire_format PRIVATE msg)
add_test(NAME test_wire_format COMMAND test_wire_format)

add_executable(test_transport_loopback test_transport_loopback.c)
target_include_directories(test_transport_loopback PRIVATE ../core/services/core/subsysmgr/include)
target_link_libraries(test_transport_loopback PRIVATE transport msg)
add_test(NAME test_transport_loopback COMMAND test_transport_loopback)

add_executable(test_unmapped_fault
    test_unmapped_fault.c
)
target_include_directories(test_unmapped_fault PRIVATE ../kernel/include ../include)
add_test(NAME test_unmapped_fault COMMAND test_unmapped_fault)

add_subdirectory(drivers)
add_subdirectory(driver_core)
add_executable(test_constraint_validation
    core/test_constraint_validation.c
)
target_include_directories(test_constraint_validation PRIVATE
    ${CMAKE_SOURCE_DIR}/core/kernel/include
    ${CMAKE_SOURCE_DIR}/interface/uapi
)
target_sources(test_constraint_validation PRIVATE ${CMAKE_SOURCE_DIR}/core/kernel/src/core/constraints/constraints_common.c ${CMAKE_SOURCE_DIR}/core/kernel/src/core/constraints/constraint_validate.c)
add_test(NAME test_constraint_validation COMMAND test_constraint_validation)
