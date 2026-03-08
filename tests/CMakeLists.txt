cmake_minimum_required(VERSION 3.20)
project(BharatOS-Tests C)

enable_testing()

add_executable(test_early_alloc test_early_alloc.c)
target_include_directories(test_early_alloc PRIVATE ../kernel/include)

add_executable(test_panic test_panic.c ../kernel/src/panic.c)
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
target_include_directories(test_urpc_ring PRIVATE ../kernel/include)
add_test(NAME test_urpc_ring COMMAND test_urpc_ring)

add_executable(test_scheduler test_scheduler.c ../kernel/src/sched.c ../kernel/src/ai_sched.c ../kernel/src/capability.c ../kernel/src/algo_matrix.c ../kernel/src/device/device_manager.c ../kernel/src/mm/numa.c)
target_include_directories(test_scheduler PRIVATE ../kernel/include)
target_compile_definitions(test_scheduler PRIVATE TESTING=1)
add_test(NAME test_scheduler COMMAND test_scheduler)

add_executable(test_trap_syscall test_trap_syscall.c ../kernel/src/trap/trap.c ../kernel/src/sched.c ../kernel/src/ai_sched.c ../kernel/src/capability.c ../kernel/src/ipc/endpoint_ipc.c ../kernel/src/algo_matrix.c ../kernel/src/device/device_manager.c ../kernel/src/mm/numa.c)
target_include_directories(test_trap_syscall PRIVATE ../kernel/include)
target_compile_definitions(test_trap_syscall PRIVATE TESTING=1)
add_test(NAME test_trap_syscall COMMAND test_trap_syscall)

add_executable(test_capability_ipc test_capability_ipc.c ../kernel/src/capability.c ../kernel/src/ipc/endpoint_ipc.c ../kernel/src/sched.c ../kernel/src/ai_sched.c ../kernel/src/algo_matrix.c ../kernel/src/device/device_manager.c ../kernel/src/mm/numa.c)
target_include_directories(test_capability_ipc PRIVATE ../kernel/include)
target_compile_definitions(test_capability_ipc PRIVATE TESTING=1)
add_test(NAME test_capability_ipc COMMAND test_capability_ipc)

add_executable(test_device_framework test_device_framework.c ../kernel/src/device/device_manager.c ../kernel/src/device/builtin_drivers.c ../kernel/src/device/pci.c ../kernel/src/ipc/zero_copy_io.c ../kernel/src/capability.c ../kernel/src/sched.c ../kernel/src/ai_sched.c ../kernel/src/algo_matrix.c ../kernel/src/mm/numa.c)
target_include_directories(test_device_framework PRIVATE ../kernel/include)
target_compile_definitions(test_device_framework PRIVATE TESTING=1)
add_test(NAME test_device_framework COMMAND test_device_framework)

add_executable(test_multikernel_topology test_multikernel_topology.c ../kernel/src/ipc/multikernel.c ../kernel/src/mm/numa.c)
target_include_directories(test_multikernel_topology PRIVATE ../kernel/include)
add_test(NAME test_multikernel_topology COMMAND test_multikernel_topology)

add_executable(test_capability_misuse test_capability_misuse.c ../kernel/src/capability.c ../kernel/src/sched.c ../kernel/src/ai_sched.c ../kernel/src/algo_matrix.c ../kernel/src/device/device_manager.c ../kernel/src/mm/numa.c)
target_include_directories(test_capability_misuse PRIVATE ../kernel/include)
target_compile_definitions(test_capability_misuse PRIVATE TESTING=1)
add_test(NAME test_capability_misuse COMMAND test_capability_misuse)

add_executable(test_memory_edgecases test_memory_edgecases.c ../kernel/src/mm/vmm.c ../kernel/src/capability.c ../kernel/src/sched.c ../kernel/src/ai_sched.c ../kernel/src/algo_matrix.c ../kernel/src/device/device_manager.c ../kernel/src/mm/numa.c ../kernel/src/mm/zswap.c)
target_include_directories(test_memory_edgecases PRIVATE ../kernel/include)
target_compile_definitions(test_memory_edgecases PRIVATE TESTING=1)
add_test(NAME test_memory_edgecases COMMAND test_memory_edgecases)

add_executable(test_tier_a_integration test_tier_a_integration.c ../kernel/src/trap/trap.c ../kernel/src/sched.c ../kernel/src/ai_sched.c ../kernel/src/capability.c ../kernel/src/ipc/endpoint_ipc.c ../kernel/src/algo_matrix.c ../kernel/src/device/device_manager.c ../kernel/src/mm/numa.c)
target_include_directories(test_tier_a_integration PRIVATE ../kernel/include)
target_compile_definitions(test_tier_a_integration PRIVATE TESTING=1)
add_test(NAME test_tier_a_integration COMMAND test_tier_a_integration)

add_executable(test_riscv_shakti_bsp test_riscv_shakti_bsp.c ../kernel/src/hal/riscv/shakti_bsp.c)
target_include_directories(test_riscv_shakti_bsp PRIVATE ../kernel/include)
add_test(NAME test_riscv_shakti_bsp COMMAND test_riscv_shakti_bsp)

add_executable(test_ai_governor test_ai_governor.c ../subsys/src/ai_governor.c ../kernel/src/ai_sched.c)
target_include_directories(test_ai_governor PRIVATE ../kernel/include)
target_compile_definitions(test_ai_governor PRIVATE TESTING=1)
add_test(NAME test_ai_governor COMMAND test_ai_governor)

add_executable(test_ai_kernel_bridge test_ai_kernel_bridge.c ../kernel/src/ai_kernel_bridge.c ../kernel/src/sched.c ../kernel/src/mm/numa.c ../kernel/src/mm/pmm.c ../kernel/src/mm/vmm.c ../kernel/src/mm/early_alloc.c ../kernel/src/hal/x86_64/hal_cpu.c ../kernel/src/ipc/endpoint_ipc.c ../kernel/src/ai_sched.c ../kernel/src/hal/timer_common.c ../kernel/src/capability.c ../kernel/src/algo_matrix.c ../kernel/src/device/device_manager.c)
target_include_directories(test_ai_kernel_bridge PRIVATE ../kernel/include)
target_compile_definitions(test_ai_kernel_bridge PRIVATE TESTING=1)
add_test(NAME test_ai_kernel_bridge COMMAND test_ai_kernel_bridge)

add_executable(test_secure_boot_policy test_secure_boot_policy.c ../kernel/src/security/secure_boot.c)
target_include_directories(test_secure_boot_policy PRIVATE ../kernel/include)
add_test(NAME test_secure_boot_policy COMMAND test_secure_boot_policy)

add_executable(test_capability_policy test_capability_policy.c
    ${CMAKE_SOURCE_DIR}/../kernel/src/mm/vmm.c
    ${CMAKE_SOURCE_DIR}/../kernel/src/sched.c
    ${CMAKE_SOURCE_DIR}/../kernel/src/capability.c
    ${CMAKE_SOURCE_DIR}/../kernel/src/algo_matrix.c
    ${CMAKE_SOURCE_DIR}/../kernel/src/device/device_manager.c
    ${CMAKE_SOURCE_DIR}/../kernel/src/mm/numa.c
    ${CMAKE_SOURCE_DIR}/../kernel/src/mm/zswap.c
)
target_include_directories(test_capability_policy PRIVATE ${CMAKE_SOURCE_DIR}/../kernel/include)
add_test(NAME test_capability_policy COMMAND test_capability_policy)

# Benchmarks

add_executable(test_bench_ipc test_bench_ipc.c benchmark_stubs.c
    ${CMAKE_SOURCE_DIR}/../kernel/src/benchmark/benchmark.c
    ${CMAKE_SOURCE_DIR}/../kernel/src/ipc/endpoint_ipc.c
    ${CMAKE_SOURCE_DIR}/../kernel/src/capability.c
    ${CMAKE_SOURCE_DIR}/../kernel/src/sched.c
    ${CMAKE_SOURCE_DIR}/../kernel/src/ai_sched.c
    ${CMAKE_SOURCE_DIR}/../kernel/src/algo_matrix.c
    ${CMAKE_SOURCE_DIR}/../kernel/src/device/device_manager.c
    ${CMAKE_SOURCE_DIR}/../kernel/src/mm/numa.c
)
target_include_directories(test_bench_ipc PRIVATE ${CMAKE_SOURCE_DIR}/../kernel/include)
target_compile_definitions(test_bench_ipc PRIVATE TESTING=1)
add_test(NAME test_bench_ipc COMMAND test_bench_ipc)

add_executable(test_bench_sched test_bench_sched.c benchmark_stubs.c
    ${CMAKE_SOURCE_DIR}/../kernel/src/benchmark/benchmark.c
    ${CMAKE_SOURCE_DIR}/../kernel/src/sched.c
    ${CMAKE_SOURCE_DIR}/../kernel/src/ai_sched.c
    ${CMAKE_SOURCE_DIR}/../kernel/src/capability.c
    ${CMAKE_SOURCE_DIR}/../kernel/src/algo_matrix.c
    ${CMAKE_SOURCE_DIR}/../kernel/src/device/device_manager.c
    ${CMAKE_SOURCE_DIR}/../kernel/src/mm/numa.c
)
target_include_directories(test_bench_sched PRIVATE ${CMAKE_SOURCE_DIR}/../kernel/include)
target_compile_definitions(test_bench_sched PRIVATE TESTING=1)
add_test(NAME test_bench_sched COMMAND test_bench_sched)
