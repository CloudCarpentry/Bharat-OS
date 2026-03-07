cmake_minimum_required(VERSION 3.20)
project(BharatOS-Tests C)

enable_testing()

add_executable(test_early_alloc test_early_alloc.c)
target_include_directories(test_early_alloc PRIVATE ../kernel/include)

add_test(NAME test_early_alloc COMMAND test_early_alloc)

add_executable(test_urpc_ring test_urpc_ring.c ../kernel/src/ipc/multikernel.c)
target_include_directories(test_urpc_ring PRIVATE ../kernel/include)
add_test(NAME test_urpc_ring COMMAND test_urpc_ring)

add_executable(test_scheduler test_scheduler.c ../kernel/src/sched_stub.c ../kernel/src/capability.c)
target_include_directories(test_scheduler PRIVATE ../kernel/include)
add_test(NAME test_scheduler COMMAND test_scheduler)

add_executable(test_trap_syscall test_trap_syscall.c ../kernel/src/trap/trap.c ../kernel/src/sched_stub.c ../kernel/src/capability.c ../kernel/src/ipc/endpoint_ipc.c)
target_include_directories(test_trap_syscall PRIVATE ../kernel/include)
add_test(NAME test_trap_syscall COMMAND test_trap_syscall)

add_executable(test_capability_ipc test_capability_ipc.c ../kernel/src/capability.c ../kernel/src/ipc/endpoint_ipc.c ../kernel/src/sched_stub.c)
target_include_directories(test_capability_ipc PRIVATE ../kernel/include)
add_test(NAME test_capability_ipc COMMAND test_capability_ipc)

add_executable(test_device_framework test_device_framework.c ../kernel/src/device/device_manager.c ../kernel/src/device/builtin_drivers.c ../kernel/src/ipc/zero_copy_io.c ../kernel/src/capability.c ../kernel/src/sched_stub.c)
target_include_directories(test_device_framework PRIVATE ../kernel/include)
add_test(NAME test_device_framework COMMAND test_device_framework)

add_executable(test_multikernel_topology test_multikernel_topology.c ../kernel/src/ipc/multikernel.c ../kernel/src/mm/numa.c)
target_include_directories(test_multikernel_topology PRIVATE ../kernel/include)
add_test(NAME test_multikernel_topology COMMAND test_multikernel_topology)

add_executable(test_capability_misuse test_capability_misuse.c ../kernel/src/capability.c ../kernel/src/sched_stub.c)
target_include_directories(test_capability_misuse PRIVATE ../kernel/include)
add_test(NAME test_capability_misuse COMMAND test_capability_misuse)

add_executable(test_memory_edgecases test_memory_edgecases.c ../kernel/src/mm/vmm.c ../kernel/src/capability.c ../kernel/src/sched_stub.c)
target_include_directories(test_memory_edgecases PRIVATE ../kernel/include)
add_test(NAME test_memory_edgecases COMMAND test_memory_edgecases)

add_executable(test_tier_a_integration test_tier_a_integration.c ../kernel/src/trap/trap.c ../kernel/src/sched_stub.c ../kernel/src/capability.c ../kernel/src/ipc/endpoint_ipc.c)
target_include_directories(test_tier_a_integration PRIVATE ../kernel/include)
add_test(NAME test_tier_a_integration COMMAND test_tier_a_integration)

add_executable(test_riscv_shakti_bsp test_riscv_shakti_bsp.c ../kernel/src/hal/riscv/shakti_bsp.c)
target_include_directories(test_riscv_shakti_bsp PRIVATE ../kernel/include)
add_test(NAME test_riscv_shakti_bsp COMMAND test_riscv_shakti_bsp)
