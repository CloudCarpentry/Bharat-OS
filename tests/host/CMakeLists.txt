cmake_minimum_required(VERSION 3.20)
include(cmake/test_component_policy.cmake)

# ... preserve existing host tests logic, append ours ...
add_executable(test_can_filter_match drivers/test_can_filter_match.c ../../drivers/class/can/can_filter_compile.c)
target_include_directories(test_can_filter_match PRIVATE ../../drivers/include)

add_executable(test_can_filter_compile drivers/test_can_filter_compile.c)
target_include_directories(test_can_filter_compile PRIVATE ../../drivers/include)

add_executable(test_can_controller_core drivers/test_can_controller_core.c ../../drivers/class/can/can_controller_core.c)
target_include_directories(test_can_controller_core PRIVATE ../../drivers/include)

add_executable(test_can_router services/test_can_router.c)
target_include_directories(test_can_router PRIVATE ../../services/include)

add_executable(test_can_subscriptions services/test_can_subscriptions.c)
target_include_directories(test_can_subscriptions PRIVATE ../../services/include)

add_executable(test_can_gateway services/test_can_gateway.c)
target_include_directories(test_can_gateway PRIVATE ../../services/include)

add_executable(test_can_busoff_recovery services/test_can_busoff_recovery.c ../../drivers/class/can/can_controller_core.c ../../drivers/devices/virt_can/virt_can.c)
target_include_directories(test_can_busoff_recovery PRIVATE ../../drivers/include)

add_test(NAME host_test_can_filter_match COMMAND test_can_filter_match)
add_test(NAME host_test_can_filter_compile COMMAND test_can_filter_compile)
add_test(NAME host_test_can_controller_core COMMAND test_can_controller_core)
add_test(NAME host_test_can_router COMMAND test_can_router)
add_test(NAME host_test_can_subscriptions COMMAND test_can_subscriptions)
add_test(NAME host_test_can_gateway COMMAND test_can_gateway)
add_test(NAME host_test_can_busoff_recovery COMMAND test_can_busoff_recovery)

# Note: The test targets will be built automatically with 'make check' or 'ctest'.

# Phase 1: Power Mode Tests
add_executable(test_power_mode_state_machine services/test_power_mode_state_machine.c ../../services/power_mode/state_machine.c)
target_include_directories(test_power_mode_state_machine PRIVATE ../../services/include)

add_executable(test_power_mode_callbacks services/test_power_mode_callbacks.c ../../services/power_mode/state_machine.c)
target_include_directories(test_power_mode_callbacks PRIVATE ../../services/include)

add_executable(test_power_mode_limp_home services/test_power_mode_limp_home.c ../../services/power_mode/state_machine.c)
target_include_directories(test_power_mode_limp_home PRIVATE ../../services/include)

add_test(NAME host_test_power_mode_state_machine COMMAND test_power_mode_state_machine)
add_test(NAME host_test_power_mode_callbacks COMMAND test_power_mode_callbacks)
add_test(NAME host_test_power_mode_limp_home COMMAND test_power_mode_limp_home)

# Phase 1: Diag Tests
add_executable(test_diag_store services/test_diag_store.c ../../services/diag/store.c)
target_include_directories(test_diag_store PRIVATE ../../services/include)

add_executable(test_diag_debounce services/test_diag_debounce.c ../../services/diag/debounce.c ../../services/diag/store.c)
target_include_directories(test_diag_debounce PRIVATE ../../services/include)

add_test(NAME host_test_diag_store COMMAND test_diag_store)
add_test(NAME host_test_diag_debounce COMMAND test_diag_debounce)

# Phase 2: Sensor Tests
add_executable(test_sensor_core drivers/test_sensor_core.c ../../drivers/class/sensor/sensor_core.c)
target_include_directories(test_sensor_core PRIVATE ../../drivers/include)

add_executable(test_sensor_core_bench drivers/test_sensor_core_bench.c ../../drivers/class/sensor/sensor_core.c)
target_include_directories(test_sensor_core_bench PRIVATE ../../drivers/include)

add_executable(test_sensor_freshness services/test_sensor_freshness.c ../../services/sensor_hub/freshness.c)
target_include_directories(test_sensor_freshness PRIVATE ../../drivers/include)

add_executable(test_sensor_plausibility services/test_sensor_plausibility.c ../../services/sensor_hub/plausibility.c)
target_include_directories(test_sensor_plausibility PRIVATE ../../drivers/include)
target_link_libraries(test_sensor_plausibility PRIVATE m)

add_test(NAME host_test_sensor_core COMMAND test_sensor_core)
add_test(NAME host_test_sensor_core_bench COMMAND test_sensor_core_bench)
add_test(NAME host_test_sensor_freshness COMMAND test_sensor_freshness)
add_test(NAME host_test_sensor_plausibility COMMAND test_sensor_plausibility)

# Phase 2: Actuator Tests
add_executable(test_actuator_core drivers/test_actuator_core.c ../../drivers/class/actuator/actuator_core.c)
target_include_directories(test_actuator_core PRIVATE ../../drivers/include)

add_executable(test_actuator_safe_state services/test_actuator_safe_state.c ../../services/actuator_mgr/safe_state.c)
target_include_directories(test_actuator_safe_state PRIVATE ../../drivers/include)

add_executable(test_actuator_deadman services/test_actuator_deadman.c)

add_test(NAME host_test_actuator_core COMMAND test_actuator_core)
add_test(NAME host_test_actuator_safe_state COMMAND test_actuator_safe_state)
add_test(NAME host_test_actuator_deadman COMMAND test_actuator_deadman)

# Phase 2: Time Sync Tests
add_executable(test_time_sync_drift services/test_time_sync_drift.c ../../services/time_sync/drift.c)
target_include_directories(test_time_sync_drift PRIVATE ../../services/include)

add_executable(test_time_sync_quality services/test_time_sync_quality.c ../../services/time_sync/quality.c)

add_test(NAME host_test_time_sync_drift COMMAND test_time_sync_drift)
add_test(NAME host_test_time_sync_quality COMMAND test_time_sync_quality)

# Phase 3: Motor Control Tests
add_executable(test_motor_control_core drivers/test_motor_control_core.c ../../drivers/class/motor/motor_control_core.c)
target_include_directories(test_motor_control_core PRIVATE ../../drivers/include)

add_test(NAME host_test_motor_control_core COMMAND test_motor_control_core)

# VFS/RamFS Tests
add_executable(test_ramfs_streq fs/test_ramfs_streq.c)
target_include_directories(test_ramfs_streq PRIVATE ../../kernel/include)
add_test(NAME host_test_ramfs_streq COMMAND test_ramfs_streq)
add_executable(benchmark_actuator drivers/benchmark_actuator.c ../../drivers/class/actuator/actuator_core.c)
target_include_directories(benchmark_actuator PRIVATE ../../drivers/include)
add_test(NAME host_benchmark_actuator COMMAND benchmark_actuator)

# Phase 3: Core Scheduler Tests
add_executable(test_sched test_sched.c)
target_include_directories(test_sched PRIVATE ../../kernel/include)

add_test(NAME host_test_sched COMMAND test_sched)

# Phase 3: Core IPC/Profile Tests
add_executable(test_ipc_profile test_ipc_profile.c ../../kernel/src/profile/profile.c ../../kernel/src/ipc/ipc_traffic.c)
target_include_directories(test_ipc_profile PRIVATE ../../kernel/include)
add_test(NAME host_test_ipc_profile COMMAND test_ipc_profile)

# Phase 3: Crypto Tests
add_executable(test_secure_mem test_secure_mem.c)
target_include_directories(test_secure_mem PRIVATE ../../kernel/include)

add_test(NAME host_test_secure_mem COMMAND test_secure_mem)
# Phase 3: NetMgr Tests
add_executable(test_netmgr_address_table services/test_netmgr_address_table.c ../../services/netmgr/src/address_table.c ../../lib/runtime/src/freestanding_string.c)
target_include_directories(test_netmgr_address_table PRIVATE ../../services/netmgr/src ../../services/core/subsysmgr/include ../../lib/net/include ../../lib/runtime/include)
add_test(NAME host_test_netmgr_address_table COMMAND test_netmgr_address_table)

add_executable(test_pmm_pcache test_pmm_pcache.c ../../kernel/src/mm/pmm/pmm.c ../../kernel/src/mm/pmm/pmm_pcache.c ../../kernel/src/mm/pmm/numa.c)
target_include_directories(test_pmm_pcache PRIVATE ${CMAKE_SOURCE_DIR}/kernel/include ${CMAKE_SOURCE_DIR}/kernel/src ${CMAKE_SOURCE_DIR}/include)
add_test(NAME host_test_pmm_pcache COMMAND test_pmm_pcache)
add_executable(test_netmgr_interface_table services/test_netmgr_interface_table.c ../../services/netmgr/src/interface_table.c ../../lib/runtime/src/freestanding_string.c)
target_include_directories(test_netmgr_interface_table PRIVATE ../../services/netmgr/src ../../services/core/subsysmgr/include ../../lib/net/include ../../lib/runtime/include)
add_test(NAME host_test_netmgr_interface_table COMMAND test_netmgr_interface_table)

# Memory host conformance expansion (2026-03-23)
# HAL contracts + generic PT/TLB contracts
add_executable(host_test_hal_pt_common ../../tests/mm/pt_common_tests.c ../../hal/hal_pt.c)
target_include_directories(host_test_hal_pt_common PRIVATE ${CMAKE_SOURCE_DIR}/kernel/include ${CMAKE_SOURCE_DIR}/include)
add_test(NAME host_test_hal_pt_common COMMAND host_test_hal_pt_common)

add_executable(host_test_hal_pt_fallbacks ../../tests/test_hal_pt_fallbacks.c ../../hal/hal_pt.c)
target_include_directories(host_test_hal_pt_fallbacks PRIVATE ${CMAKE_SOURCE_DIR}/kernel/include ${CMAKE_SOURCE_DIR}/include)
add_test(NAME host_test_hal_pt_fallbacks COMMAND host_test_hal_pt_fallbacks)

add_executable(host_test_hal_caps_consistency ../../tests/test_hal_caps_consistency.c ../../hal/hal_pt.c)
target_include_directories(host_test_hal_caps_consistency PRIVATE ${CMAKE_SOURCE_DIR}/kernel/include ${CMAKE_SOURCE_DIR}/include)
add_test(NAME host_test_hal_caps_consistency COMMAND host_test_hal_caps_consistency)

# VMM / ASpace / VM base-object conformance currently remains in top-level tests
# (`test_vmm_aspace`) due additional prot-domain and arch-cap dependencies.
