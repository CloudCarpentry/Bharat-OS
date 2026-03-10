cmake_minimum_required(VERSION 3.20)
project(BharatOS-Tests C)

enable_testing()

add_executable(test_early_alloc test_early_alloc.c)
target_include_directories(test_early_alloc PRIVATE ../kernel/include)

add_executable(test_panic test_panic.c ../kernel/src/panic.c)

add_executable(test_stress_concurrency test_stress_concurrency.c
    benchmark_stubs.c
    ../kernel/src/sched.c
    ../kernel/src/ai_sched.c
    ../kernel/src/capability.c
    ../kernel/src/ipc/endpoint_ipc.c
    ../kernel/src/algo_matrix.c
    ../kernel/src/device/device_manager.c
    ../kernel/src/mm/numa.c
)
target_include_directories(test_stress_concurrency PRIVATE ../kernel/include)
target_compile_definitions(test_stress_concurrency PRIVATE TESTING=1)
target_compile_options(test_stress_concurrency PRIVATE -g -O0)
target_link_libraries(test_stress_concurrency PRIVATE pthread)
add_test(NAME test_stress_concurrency COMMAND test_stress_concurrency)
target_include_directories(test_panic PRIVATE ../kernel/include)
add_test(NAME test_panic COMMAND test_panic)

add_test(NAME test_early_alloc COMMAND test_early_alloc)

add_executable(test_hal_interrupts test_hal_interrupts.c ../kernel/src/hal/interrupt_common.c)
target_include_directories(test_hal_interrupts PRIVATE ../kernel/include)
add_test(NAME test_hal_interrupts COMMAND test_hal_interrupts)

add_executable(test_timer_common test_timer_common.c ../kernel/src/hal/timer_common.c)
target_include_directories(test_timer_common PRIVATE ../kernel/include)
add_test(NAME test_timer_common COMMAND test_timer_common)

add_executable(test_urpc_ring test_urpc_ring.c ../kernel/src/ipc/multikernel.c)
add_executable(test_ipc_async_timeout test_ipc_async_timeout.c ../kernel/src/ipc/async_ipc.c ../kernel/src/ipc/ipc_timeout.c)
target_include_directories(test_ipc_async_timeout PRIVATE ../kernel/include)
add_test(NAME test_ipc_async_timeout COMMAND test_ipc_async_timeout)

add_executable(test_automotive_subsys test_automotive_subsys.c ../subsys/automotive/automotive.c)
target_include_directories(test_automotive_subsys PRIVATE ../subsys/include ../kernel/include)
add_test(NAME test_automotive_subsys COMMAND test_automotive_subsys)

target_include_directories(test_urpc_ring PRIVATE ../kernel/include)
add_test(NAME test_urpc_ring COMMAND test_urpc_ring)

add_executable(test_scheduler test_scheduler.c benchmark_stubs.c ../kernel/src/sched.c ../kernel/src/ai_sched.c ../kernel/src/capability.c ../kernel/src/algo_matrix.c ../kernel/src/device/device_manager.c ../kernel/src/mm/numa.c)
target_include_directories(test_scheduler PRIVATE ../kernel/include)
target_compile_definitions(test_scheduler PRIVATE TESTING=1)
add_test(NAME test_scheduler COMMAND test_scheduler)

add_executable(test_trap_syscall test_trap_syscall.c benchmark_stubs.c ../kernel/src/trap/trap.c ../kernel/src/sched.c ../kernel/src/ai_sched.c ../kernel/src/capability.c ../kernel/src/ipc/endpoint_ipc.c ../kernel/src/algo_matrix.c ../kernel/src/device/device_manager.c ../kernel/src/mm/numa.c ../subsys/linux/linux_compat.c)
target_include_directories(test_trap_syscall PRIVATE ../kernel/include ../subsys/include ../include)
target_compile_definitions(test_trap_syscall PRIVATE TESTING=1)
add_test(NAME test_trap_syscall COMMAND test_trap_syscall)

add_executable(test_capability_ipc test_capability_ipc.c benchmark_stubs.c ../kernel/src/capability.c ../kernel/src/ipc/endpoint_ipc.c ../kernel/src/sched.c ../kernel/src/ai_sched.c ../kernel/src/algo_matrix.c ../kernel/src/device/device_manager.c ../kernel/src/mm/numa.c)
target_include_directories(test_capability_ipc PRIVATE ../kernel/include)
target_compile_definitions(test_capability_ipc PRIVATE TESTING=1)
add_test(NAME test_capability_ipc COMMAND test_capability_ipc)

add_executable(test_device_framework test_device_framework.c benchmark_stubs.c ../kernel/src/device/device_manager.c ../kernel/src/device/builtin_drivers.c ../kernel/src/subsystem/profile.c ../kernel/src/device/pci.c ../kernel/src/ipc/zero_copy_io.c ../kernel/src/capability.c ../kernel/src/sched.c ../kernel/src/ai_sched.c ../kernel/src/algo_matrix.c ../kernel/src/mm/numa.c)
target_include_directories(test_device_framework PRIVATE ../kernel/include)
target_compile_definitions(test_device_framework PRIVATE TESTING=1)
add_test(NAME test_device_framework COMMAND test_device_framework)

add_executable(test_multikernel_topology test_multikernel_topology.c benchmark_stubs.c ../kernel/src/ipc/multikernel.c ../kernel/src/mm/numa.c)
target_include_directories(test_multikernel_topology PRIVATE ../kernel/include)
add_test(NAME test_multikernel_topology COMMAND test_multikernel_topology)

add_executable(test_capability_misuse test_capability_misuse.c benchmark_stubs.c ../kernel/src/capability.c ../kernel/src/sched.c ../kernel/src/ai_sched.c ../kernel/src/algo_matrix.c ../kernel/src/device/device_manager.c ../kernel/src/mm/numa.c)
target_include_directories(test_capability_misuse PRIVATE ../kernel/include)
target_compile_definitions(test_capability_misuse PRIVATE TESTING=1)
add_test(NAME test_capability_misuse COMMAND test_capability_misuse)

add_executable(test_memory_edgecases test_memory_edgecases.c benchmark_stubs.c ../kernel/src/mm/vmm.c ../kernel/src/capability.c ../kernel/src/sched.c ../kernel/src/ai_sched.c ../kernel/src/algo_matrix.c ../kernel/src/device/device_manager.c ../kernel/src/mm/numa.c ../kernel/src/mm/zswap.c)
target_include_directories(test_memory_edgecases PRIVATE ../kernel/include)
target_compile_definitions(test_memory_edgecases PRIVATE TESTING=1)
add_test(NAME test_memory_edgecases COMMAND test_memory_edgecases)

add_executable(test_tier_a_integration test_tier_a_integration.c benchmark_stubs.c ../kernel/src/trap/trap.c ../kernel/src/sched.c ../kernel/src/ai_sched.c ../kernel/src/capability.c ../kernel/src/ipc/endpoint_ipc.c ../kernel/src/algo_matrix.c ../kernel/src/device/device_manager.c ../kernel/src/mm/numa.c ../subsys/linux/linux_compat.c)
target_include_directories(test_tier_a_integration PRIVATE ../kernel/include ../subsys/include ../include)
target_compile_definitions(test_tier_a_integration PRIVATE TESTING=1)
add_test(NAME test_tier_a_integration COMMAND test_tier_a_integration)

add_executable(test_riscv_shakti_bsp test_riscv_shakti_bsp.c ../kernel/src/hal/riscv/shakti_bsp.c)
target_include_directories(test_riscv_shakti_bsp PRIVATE ../kernel/include)
add_test(NAME test_riscv_shakti_bsp COMMAND test_riscv_shakti_bsp)

add_executable(test_ai_governor test_ai_governor.c ../subsys/src/ai_governor.c ../kernel/src/ai_sched.c)
target_include_directories(test_ai_governor PRIVATE ../kernel/include)
target_compile_definitions(test_ai_governor PRIVATE TESTING=1)
add_test(NAME test_ai_governor COMMAND test_ai_governor)

add_executable(test_ai_kernel_bridge test_ai_kernel_bridge.c benchmark_stubs.c ../kernel/src/ai_kernel_bridge.c ../kernel/src/sched.c ../kernel/src/mm/numa.c ../kernel/src/mm/vmm.c ../kernel/src/ipc/endpoint_ipc.c ../kernel/src/ai_sched.c ../kernel/src/hal/timer_common.c ../kernel/src/capability.c ../kernel/src/algo_matrix.c ../kernel/src/device/device_manager.c)
target_include_directories(test_ai_kernel_bridge PRIVATE ../kernel/include)
target_compile_definitions(test_ai_kernel_bridge PRIVATE TESTING=1)
add_test(NAME test_ai_kernel_bridge COMMAND test_ai_kernel_bridge)

add_executable(test_secure_boot_policy test_secure_boot_policy.c ../kernel/src/security/secure_boot.c ../kernel/src/security/audit.c)
target_include_directories(test_secure_boot_policy PRIVATE ../kernel/include)
add_test(NAME test_secure_boot_policy COMMAND test_secure_boot_policy)

add_executable(test_capability_policy test_capability_policy.c benchmark_stubs.c
    ../kernel/src/mm/vmm.c
    ../kernel/src/sched.c
    ../kernel/src/ai_sched.c
    ../kernel/src/capability.c
    ../kernel/src/algo_matrix.c
    ../kernel/src/device/device_manager.c
    ../kernel/src/mm/numa.c
    ../kernel/src/mm/zswap.c
)
target_include_directories(test_capability_policy PRIVATE ../kernel/include)
add_test(NAME test_capability_policy COMMAND test_capability_policy)

add_executable(test_posix test_posix.c ../subsys/linux/linux_compat.c)
target_include_directories(test_posix PRIVATE ../kernel/include ../subsys/include)
add_test(NAME test_posix COMMAND test_posix)

add_executable(test_windows_compat_init test_windows_compat_init.c ../subsys/windows/win_compat.c)
target_include_directories(test_windows_compat_init PRIVATE ../kernel/include ../subsys/include)
add_test(NAME test_windows_compat_init COMMAND test_windows_compat_init)

add_executable(test_vm_stress test_vm_stress.c benchmark_stubs.c ../kernel/src/mm/vmm.c)
target_include_directories(test_vm_stress PRIVATE ../kernel/include)
add_test(NAME test_vm_stress COMMAND test_vm_stress)

add_executable(test_ipc_fuzz test_ipc_fuzz.c benchmark_stubs.c
    ../kernel/src/sched.c
    ../kernel/src/ai_sched.c
    ../kernel/src/capability.c
    ../kernel/src/ipc/endpoint_ipc.c
    ../kernel/src/algo_matrix.c
    ../kernel/src/device/device_manager.c
    ../kernel/src/mm/numa.c
)
target_include_directories(test_ipc_fuzz PRIVATE ../kernel/include)
add_test(NAME test_ipc_fuzz COMMAND test_ipc_fuzz)

add_executable(test_security_isolation test_security_isolation.c benchmark_stubs.c
    ../kernel/src/sched.c
    ../kernel/src/ai_sched.c
    ../kernel/src/capability.c
    ../kernel/src/ipc/endpoint_ipc.c
    ../kernel/src/algo_matrix.c
    ../kernel/src/device/device_manager.c
    ../kernel/src/mm/numa.c
)
target_include_directories(test_security_isolation PRIVATE ../kernel/include)
add_test(NAME test_security_isolation COMMAND test_security_isolation)

# Benchmarks

add_executable(test_bench_ipc test_bench_ipc.c benchmark_stubs.c
    ../kernel/src/benchmark/benchmark.c
    ../kernel/src/ipc/endpoint_ipc.c
    ../kernel/src/capability.c
    ../kernel/src/sched.c
    ../kernel/src/ai_sched.c
    ../kernel/src/algo_matrix.c
    ../kernel/src/device/device_manager.c
    ../kernel/src/mm/numa.c
)
target_include_directories(test_bench_ipc PRIVATE ../kernel/include)
target_compile_definitions(test_bench_ipc PRIVATE TESTING=1)
add_test(NAME test_bench_ipc COMMAND test_bench_ipc)

add_executable(test_bench_sched test_bench_sched.c benchmark_stubs.c
    ../kernel/src/benchmark/benchmark.c
    ../kernel/src/sched.c
    ../kernel/src/ai_sched.c
    ../kernel/src/capability.c
    ../kernel/src/algo_matrix.c
    ../kernel/src/device/device_manager.c
    ../kernel/src/mm/numa.c
)
target_include_directories(test_bench_sched PRIVATE ../kernel/include)
target_compile_definitions(test_bench_sched PRIVATE TESTING=1)
add_test(NAME test_bench_sched COMMAND test_bench_sched)


add_executable(test_vfs_storage test_vfs_storage.c ../kernel/src/fs/vfs.c ../kernel/src/fs/blob_backend.c)
target_include_directories(test_vfs_storage PRIVATE ../kernel/include)
target_compile_definitions(test_vfs_storage PRIVATE TESTING=1)
add_test(NAME test_vfs_storage COMMAND test_vfs_storage)

add_executable(test_subsystem_profile test_subsystem_profile.c ../kernel/src/subsystem/profile.c)
target_include_directories(test_subsystem_profile PRIVATE ../kernel/include)
target_compile_definitions(test_subsystem_profile PRIVATE BHARAT_PROFILE_DATACENTER=1 BHARAT_PERSONALITY_LINUX=1 BHARAT_ARCH_X86=1)
add_test(NAME test_subsystem_profile COMMAND test_subsystem_profile)

add_executable(test_builtin_drivers_profile test_builtin_drivers_profile.c ../kernel/src/device/device_manager.c ../kernel/src/device/builtin_drivers.c ../kernel/src/subsystem/profile.c ../kernel/src/algo_matrix.c)
target_include_directories(test_builtin_drivers_profile PRIVATE ../kernel/include)
target_compile_definitions(test_builtin_drivers_profile PRIVATE TESTING=1 BHARAT_PROFILE_DATACENTER=1 BHARAT_PERSONALITY_LINUX=1 BHARAT_ARCH_X86=1)
add_test(NAME test_builtin_drivers_profile COMMAND test_builtin_drivers_profile)

# Integration Subsystem Tests
add_executable(test_integration_core_subsys
    test_integration_core_subsys.c
    benchmark_stubs.c
    ../kernel/src/device/device_manager.c
    ../kernel/src/algo_matrix.c
    ../subsys/linux/linux_compat.c
    ../subsys/automotive/automotive.c
    ../ui/fbui/core/fb_render.c
)
target_include_directories(test_integration_core_subsys PRIVATE ../kernel/include ../subsys/include ../include)
target_compile_definitions(test_integration_core_subsys PRIVATE TESTING=1 BHARAT_PROFILE_DATACENTER=1 BHARAT_PERSONALITY_LINUX=1 BHARAT_ARCH_X86=1)
add_test(NAME test_integration_core_subsys COMMAND test_integration_core_subsys)

# Edge Profile Subsystem Tests
add_executable(test_profile_edge
    test_profile_edge.c
    benchmark_stubs.c
    ../kernel/src/device/device_manager.c
    ../kernel/src/algo_matrix.c
    ../subsys/automotive/automotive.c
    ../ui/fbui/core/fb_render.c
)
target_include_directories(test_profile_edge PRIVATE ../kernel/include ../subsys/include ../include)
target_compile_definitions(test_profile_edge PRIVATE TESTING=1 BHARAT_PROFILE_EDGE=1)
add_test(NAME test_profile_edge COMMAND test_profile_edge)


add_executable(test_scheduler_hello_world test_scheduler_hello_world.c benchmark_stubs.c ../kernel/src/sched.c ../kernel/src/ai_sched.c ../kernel/src/capability.c ../kernel/src/algo_matrix.c ../kernel/src/device/device_manager.c ../kernel/src/mm/numa.c)
target_include_directories(test_scheduler_hello_world PRIVATE ../kernel/include)
target_compile_definitions(test_scheduler_hello_world PRIVATE TESTING=1)
add_test(NAME test_scheduler_hello_world COMMAND test_scheduler_hello_world)
