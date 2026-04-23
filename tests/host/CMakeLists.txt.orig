cmake_minimum_required(VERSION 3.20)
include(cmake/test_component_policy.cmake)

if(CMAKE_SYSTEM_NAME STREQUAL "Generic")
    # This file shouldn't be processed if we are building the kernel (cross-compiling)
    return()
endif()

# ... preserve existing host tests logic, append ours ...
add_executable(test_can_filter stacks/can/test_can_filter.c ../../stacks/can/core/can_filter.c)
target_include_directories(test_can_filter PRIVATE ../../stacks/include)

add_executable(test_can_filter_compile drivers/test_can_filter_compile.c ../../drivers/class/can/can_filter_compile.c)
target_include_directories(test_can_filter_compile PRIVATE ../../drivers/include ../../stacks/include)

add_executable(test_can_controller_core drivers/test_can_controller_core.c ../../drivers/class/can/can_controller_core.c ../../stacks/can/core/can_filter.c)
target_include_directories(test_can_controller_core PRIVATE ../../drivers/include ../../stacks/include)

add_executable(test_can_controller_state_machine drivers/test_can_controller_state_machine.c ../../drivers/class/can/can_controller_core.c ../../stacks/can/core/can_filter.c)
target_include_directories(test_can_controller_state_machine PRIVATE ../../drivers/include ../../stacks/include)

add_executable(test_net_driver_core drivers/test_net_driver_core.c ../../drivers/net/core/net_driver_core.c ../../lib/packet/src/packet.c)
target_include_directories(test_net_driver_core PRIVATE ../../drivers/include ../../stacks/include ../../lib/packet/include)

add_executable(test_can_route stacks/can/test_can_route.c)
target_include_directories(test_can_route PRIVATE ../../stacks/include)

add_executable(test_can_subscriptions services/test_can_subscriptions.c)
target_include_directories(test_can_subscriptions PRIVATE ../../services/include)

add_executable(test_can_gateway services/test_can_gateway.c)
target_include_directories(test_can_gateway PRIVATE ../../services/include)

add_executable(test_can_busoff_recovery services/test_can_busoff_recovery.c ../../drivers/class/can/can_controller_core.c ../../drivers/devices/virt_can/virt_can.c ../../stacks/can/core/can_filter.c)
target_include_directories(test_can_busoff_recovery PRIVATE ../../drivers/include ../../stacks/include)

add_test(NAME host_test_can_filter COMMAND test_can_filter)
add_test(NAME host_test_can_filter_compile COMMAND test_can_filter_compile)
add_test(NAME host_test_can_controller_core COMMAND test_can_controller_core)
add_test(NAME host_test_can_controller_state_machine COMMAND test_can_controller_state_machine)
add_test(NAME host_test_net_driver_core COMMAND test_net_driver_core)
add_test(NAME host_test_can_route COMMAND test_can_route)
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
add_executable(test_diag_store services/test_diag_store.c ../../services/system/diag/store.c)
target_include_directories(test_diag_store PRIVATE ../../services/include)

add_executable(test_diag_debounce services/test_diag_debounce.c ../../services/system/diag/debounce.c ../../services/system/diag/store.c)
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

add_executable(test_actuator_safe_state services/test_actuator_safe_state.c ../../services/device/actuator_mgr/safe_state.c)
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

add_executable(test_ramfs_add_child fs/test_ramfs_add_child.c)
target_include_directories(test_ramfs_add_child PRIVATE ../../kernel/include)
add_test(NAME host_test_ramfs_add_child COMMAND test_ramfs_add_child)

add_executable(benchmark_actuator drivers/benchmark_actuator.c ../../drivers/class/actuator/actuator_core.c)
target_include_directories(benchmark_actuator PRIVATE ../../drivers/include)
add_test(NAME host_benchmark_actuator COMMAND benchmark_actuator)

# Phase 3: Core Scheduler Tests
add_executable(test_sched test_sched.c host_stubs.c ../../lib/rbtree/rbtree.c)
target_include_directories(test_sched PRIVATE ../../kernel/include ../../include ../../lib/include)

add_test(NAME host_test_sched COMMAND test_sched)

add_executable(test_lib_string test_lib_string.c ../../lib/string/string.c)
add_test(NAME host_test_lib_string COMMAND test_lib_string)

# Phase 3: Core IPC/Profile Tests
add_executable(test_ipc_profile test_ipc_profile.c ../../kernel/src/profile/profile.c ../../kernel/src/ipc/ipc_traffic.c)
target_include_directories(test_ipc_profile PRIVATE ../../kernel/include ../../include ../../lib/include)
add_test(NAME host_test_ipc_profile COMMAND test_ipc_profile)

add_executable(test_ipc_urpc_policy_rt_mpu test_ipc_urpc_policy_matrix.c ../../kernel/src/ipc/ipc_profile_policy.c ../../kernel/src/mm/mem_model.c ../../kernel/src/profile/profile.c)
target_include_directories(test_ipc_urpc_policy_rt_mpu PRIVATE ../../kernel/include ../../include ../../lib/include)
target_compile_definitions(test_ipc_urpc_policy_rt_mpu PRIVATE CONFIG_PROFILE_AUTOMOBILE=1 CONFIG_MEM_MODEL_MPU=1 BHARAT_KERNEL_PROFILE_RT=1 EXPECT_PREFER_URPC=1 EXPECT_ALLOW_BULK_IPC=0 EXPECT_URPC_MAX=32 EXPECT_CONTROL_TRANSPORT=IPC_TRANSPORT_URPC EXPECT_BULK_TRANSPORT=IPC_TRANSPORT_URPC)
add_test(NAME host_test_ipc_urpc_policy_rt_mpu COMMAND test_ipc_urpc_policy_rt_mpu)

add_executable(test_ipc_urpc_policy_gp_mmu_lite test_ipc_urpc_policy_matrix.c ../../kernel/src/ipc/ipc_profile_policy.c ../../kernel/src/mm/mem_model.c ../../kernel/src/profile/profile.c)
target_include_directories(test_ipc_urpc_policy_gp_mmu_lite PRIVATE ../../kernel/include ../../include ../../lib/include)
target_compile_definitions(test_ipc_urpc_policy_gp_mmu_lite PRIVATE CONFIG_MEM_MODEL_MMU_LITE=1 EXPECT_PREFER_URPC=0 EXPECT_ALLOW_BULK_IPC=1 EXPECT_URPC_MAX=64 EXPECT_CONTROL_TRANSPORT=IPC_TRANSPORT_ENDPOINT EXPECT_BULK_TRANSPORT=IPC_TRANSPORT_ENDPOINT)
add_test(NAME host_test_ipc_urpc_policy_gp_mmu_lite COMMAND test_ipc_urpc_policy_gp_mmu_lite)

add_executable(test_ipc_urpc_policy_mix_mmu_full test_ipc_urpc_policy_matrix.c ../../kernel/src/ipc/ipc_profile_policy.c ../../kernel/src/mm/mem_model.c ../../kernel/src/profile/profile.c)
target_include_directories(test_ipc_urpc_policy_mix_mmu_full PRIVATE ../../kernel/include ../../include ../../lib/include)
target_compile_definitions(test_ipc_urpc_policy_mix_mmu_full PRIVATE BHARAT_KERNEL_PROFILE_MIX=1 EXPECT_PREFER_URPC=1 EXPECT_ALLOW_BULK_IPC=1 EXPECT_URPC_MAX=256 EXPECT_CONTROL_TRANSPORT=IPC_TRANSPORT_URPC EXPECT_BULK_TRANSPORT=IPC_TRANSPORT_ENDPOINT)
add_test(NAME host_test_ipc_urpc_policy_mix_mmu_full COMMAND test_ipc_urpc_policy_mix_mmu_full)

# Phase 3: Crypto Tests
add_executable(test_secure_mem test_secure_mem.c)
target_include_directories(test_secure_mem PRIVATE ../../kernel/include)

add_test(NAME host_test_secure_mem COMMAND test_secure_mem)

add_executable(test_crypto_protocol_dispatch services/test_crypto_protocol_dispatch.c ../../services/security/crypto/protocol.c ../../services/security/crypto/dispatch.c)
target_include_directories(test_crypto_protocol_dispatch PRIVATE ../../services/security/crypto ../../kernel/include)
add_test(NAME host_test_crypto_protocol_dispatch COMMAND test_crypto_protocol_dispatch)

# Phase 3: NetMgr Tests
add_executable(test_netmgr_address_table services/test_netmgr_address_table.c ../../services/netmgr/src/address_table.c ../../lib/runtime/src/freestanding_string.c)
target_include_directories(test_netmgr_address_table PRIVATE ../../services/netmgr/src ../../services/netmgr/include ../../services/core/subsysmgr/include ../../lib/net/include ../../lib/runtime/include)
add_test(NAME host_test_netmgr_address_table COMMAND test_netmgr_address_table)

add_executable(test_pmm_pcache test_pmm_pcache.c ../../kernel/src/mm/pmm/pmm.c ../../kernel/src/mm/pmm/pmm_pcache.c ../../kernel/src/mm/pmm/numa.c)
target_include_directories(test_pmm_pcache PRIVATE ${CMAKE_SOURCE_DIR}/kernel/include ${CMAKE_SOURCE_DIR}/kernel/src ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/lib/include ${CMAKE_SOURCE_DIR}/boot/include)
add_test(NAME host_test_pmm_pcache COMMAND test_pmm_pcache)
add_executable(test_netmgr_interface_table services/test_netmgr_interface_table.c ../../services/netmgr/src/interface_table.c ../../lib/runtime/src/freestanding_string.c)
target_include_directories(test_netmgr_interface_table PRIVATE ../../services/netmgr/src ../../services/netmgr/include ../../services/core/subsysmgr/include ../../lib/net/include ../../lib/runtime/include)
add_test(NAME host_test_netmgr_interface_table COMMAND test_netmgr_interface_table)

add_executable(test_smp_tlb_active_cores test_smp_tlb_active_cores.c ../../kernel/src/mm/tlb/tlb_shootdown.c ../../kernel/src/mm/tlb/tlb_pending.c ../../kernel/src/mm/tlb/tlb_stats.c host_stubs.c ../../kernel/src/lib/base/string.c)
target_include_directories(test_smp_tlb_active_cores PRIVATE ${CMAKE_SOURCE_DIR}/kernel/include ${CMAKE_SOURCE_DIR}/kernel/src ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/boot/include ${CMAKE_SOURCE_DIR}/lib/include ${CMAKE_BINARY_DIR}/generated/include ${CMAKE_SOURCE_DIR}/services/monitor/generated)
add_test(NAME host_test_smp_tlb_active_cores COMMAND test_smp_tlb_active_cores)

# Memory host conformance expansion (2026-03-23)
# HAL contracts + generic PT/TLB contracts
add_executable(host_test_hal_pt_common ../../tests/mm/pt_common_tests.c ../../hal/hal_pt.c)
target_include_directories(host_test_hal_pt_common PRIVATE ${CMAKE_SOURCE_DIR}/kernel/include ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/boot/include)
add_test(NAME host_test_hal_pt_common COMMAND host_test_hal_pt_common)

add_executable(host_test_hal_pt_fallbacks ../../tests/test_hal_pt_fallbacks.c ../../hal/hal_pt.c)
target_include_directories(host_test_hal_pt_fallbacks PRIVATE ${CMAKE_SOURCE_DIR}/kernel/include ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/boot/include)
add_test(NAME host_test_hal_pt_fallbacks COMMAND host_test_hal_pt_fallbacks)

add_executable(host_test_hal_caps_consistency ../../tests/test_hal_caps_consistency.c ../../hal/hal_pt.c)
target_include_directories(host_test_hal_caps_consistency PRIVATE ${CMAKE_SOURCE_DIR}/kernel/include ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/boot/include)
add_test(NAME host_test_hal_caps_consistency COMMAND host_test_hal_caps_consistency)

add_executable(host_test_mon_vm test_mon_vm.c host_stubs.c ../../kernel/src/monitor/mon_vm_dispatch.c ../../kernel/src/monitor/mon_vm_service.c ../../kernel/src/monitor/mon_vm_state.c)
target_include_directories(host_test_mon_vm PRIVATE ${CMAKE_SOURCE_DIR}/kernel/include ${CMAKE_SOURCE_DIR}/kernel/src ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/lib/include)
add_test(NAME host_test_mon_vm COMMAND host_test_mon_vm)

# VMM / ASpace / VM base-object conformance currently remains in top-level tests
# (`test_vmm_aspace`) due additional prot-domain and arch-cap dependencies.

add_executable(host_test_vmm_fault ../test_vmm_fault.c host_stubs.c ../../kernel/src/mm/vm/fault/fault.c ../../kernel/src/mm/vm/objects/vm_object.c ../../kernel/src/mm/vm/objects/vm_object_refcount.c ../../kernel/src/mm/vm/aspace/vm_space.c ../../kernel/src/mm/vm/aspace/aspace.c ../../kernel/src/mm/pmm/early_alloc.c ../../kernel/src/mm/pmm/pmm.c ../../kernel/src/mm/pmm/pt_pool.c ../../kernel/src/mm/tlb/tlb_shootdown.c ../../kernel/src/capability.c ../../kernel/src/cap/cap_policy.c ../benchmark_stubs.c ../../kernel/src/lib/base/string.c ../../arch/common/memops_scalar.c mock_panic.c)
target_include_directories(host_test_vmm_fault PRIVATE ${CMAKE_SOURCE_DIR}/kernel/include ${CMAKE_SOURCE_DIR}/kernel/src ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/boot/include ${CMAKE_SOURCE_DIR}/lib/include ${CMAKE_BINARY_DIR}/generated/include)
target_compile_definitions(host_test_vmm_fault PRIVATE TESTING=1)
add_test(NAME host_test_vmm_fault COMMAND host_test_vmm_fault)

add_executable(test_uapi_abi test_uapi_abi.c)
target_include_directories(test_uapi_abi PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME host_test_uapi_abi COMMAND test_uapi_abi)

# ---------------------------------------------------------
# Phase 2: Runtime completeness Tests
# ---------------------------------------------------------
add_executable(test_process_manager test_process_manager.c ../../services/process_manager/process_manager.c ../../lib/ipc/src/ipc.c)
target_include_directories(test_process_manager PRIVATE ../../include ../../lib/ipc/include ../../lib/cap/include ../../kernel/include)
add_test(NAME host_test_process_manager COMMAND test_process_manager)

add_executable(test_vm_manager test_vm_manager.c ../../services/vm_manager/vm_manager.c ../../lib/ipc/src/ipc.c)
target_include_directories(test_vm_manager PRIVATE ../../include ../../lib/ipc/include ../../lib/cap/include ../../kernel/include)
add_test(NAME host_test_vm_manager COMMAND test_vm_manager)

add_executable(test_servicemgr test_servicemgr.c ../../services/servicemgr/servicemgr.c ../../lib/ipc/src/ipc.c)
target_include_directories(test_servicemgr PRIVATE ../../include ../../lib/ipc/include ../../lib/cap/include ../../kernel/include)
add_test(NAME host_test_servicemgr COMMAND test_servicemgr)

add_executable(test_init test_init.c ../../services/core/init/init_runtime.c ../../services/core/init/init_status.c ../../services/core/init/init_handoff.c)
target_include_directories(test_init PRIVATE ../../include ../../kernel/include ../../lib/runtime/include ../../lib/cap/include ../../lib/ipc/include)
add_test(NAME host_test_init COMMAND test_init)

add_executable(test_e2e_service_runtime test_e2e_service_runtime.c ../../services/process_manager/process_manager.c ../../services/vm_manager/vm_manager.c ../../services/servicemgr/servicemgr.c ../../lib/ipc/src/ipc.c)
target_include_directories(test_e2e_service_runtime PRIVATE ../../include ../../lib/ipc/include ../../lib/cap/include ../../kernel/include)
add_test(NAME host_test_e2e_service_runtime COMMAND test_e2e_service_runtime)

if(BHARAT_BUILD_HOST_TESTS)
    # I2C Core Host Tests
    add_executable(test_i2c_registry ${CMAKE_SOURCE_DIR}/tests/host/drivers/test_i2c_registry.c ${CMAKE_SOURCE_DIR}/drivers/bus/i2c/i2c_core.c ${CMAKE_SOURCE_DIR}/drivers/bus/i2c/i2c_registry.c)
    target_include_directories(test_i2c_registry PRIVATE ${CMAKE_SOURCE_DIR}/drivers/bus/i2c)
    add_test(NAME host_test_i2c_registry COMMAND test_i2c_registry)

    add_executable(test_i2c_transfer_mock ${CMAKE_SOURCE_DIR}/tests/host/drivers/test_i2c_transfer_mock.c ${CMAKE_SOURCE_DIR}/drivers/bus/i2c/i2c_core.c ${CMAKE_SOURCE_DIR}/drivers/bus/i2c/i2c_registry.c)
    target_include_directories(test_i2c_transfer_mock PRIVATE ${CMAKE_SOURCE_DIR}/drivers/bus/i2c)
    add_test(NAME host_test_i2c_transfer_mock COMMAND test_i2c_transfer_mock)

    # SPI Core Host Tests
    add_executable(test_spi_registry ${CMAKE_SOURCE_DIR}/tests/host/drivers/test_spi_registry.c ${CMAKE_SOURCE_DIR}/drivers/bus/spi/spi_core.c ${CMAKE_SOURCE_DIR}/drivers/bus/spi/spi_registry.c)
    target_include_directories(test_spi_registry PRIVATE ${CMAKE_SOURCE_DIR}/drivers/bus/spi)
    add_test(NAME host_test_spi_registry COMMAND test_spi_registry)

    add_executable(test_spi_transfer_mock ${CMAKE_SOURCE_DIR}/tests/host/drivers/test_spi_transfer_mock.c ${CMAKE_SOURCE_DIR}/drivers/bus/spi/spi_core.c ${CMAKE_SOURCE_DIR}/drivers/bus/spi/spi_registry.c)
    target_include_directories(test_spi_transfer_mock PRIVATE ${CMAKE_SOURCE_DIR}/drivers/bus/spi)
    add_test(NAME host_test_spi_transfer_mock COMMAND test_spi_transfer_mock)

    # DRM Core Host Tests
    add_executable(test_drm_registry ${CMAKE_SOURCE_DIR}/tests/host/drivers/test_drm_registry.c ${CMAKE_SOURCE_DIR}/drivers/display/drm/drm_core.c ${CMAKE_SOURCE_DIR}/drivers/display/drm/drm_registry.c)
    target_include_directories(test_drm_registry PRIVATE ${CMAKE_SOURCE_DIR}/drivers/display/drm)
    add_test(NAME host_test_drm_registry COMMAND test_drm_registry)

    add_executable(test_drm_mode_validation ${CMAKE_SOURCE_DIR}/tests/host/drivers/test_drm_mode_validation.c ${CMAKE_SOURCE_DIR}/drivers/display/drm/drm_core.c ${CMAKE_SOURCE_DIR}/drivers/display/drm/drm_registry.c)
    target_include_directories(test_drm_mode_validation PRIVATE ${CMAKE_SOURCE_DIR}/drivers/display/drm)
    add_test(NAME host_test_drm_mode_validation COMMAND test_drm_mode_validation)

    # NVMe Core Host Tests
    add_executable(test_nvme_init_mock ${CMAKE_SOURCE_DIR}/tests/host/drivers/test_nvme_init_mock.c ${CMAKE_SOURCE_DIR}/drivers/storage/nvme/nvme_core.c ${CMAKE_SOURCE_DIR}/drivers/storage/nvme/nvme_admin.c ${CMAKE_SOURCE_DIR}/drivers/storage/nvme/nvme_queue.c)
    target_include_directories(test_nvme_init_mock PRIVATE ${CMAKE_SOURCE_DIR}/drivers/storage/nvme)
    add_test(NAME host_test_nvme_init_mock COMMAND test_nvme_init_mock)

    # I2C HID Host Tests
    add_executable(test_i2c_hid_probe ${CMAKE_SOURCE_DIR}/tests/host/drivers/test_i2c_hid_probe.c ${CMAKE_SOURCE_DIR}/drivers/input/i2c_hid/i2c_hid.c ${CMAKE_SOURCE_DIR}/drivers/bus/i2c/i2c_core.c ${CMAKE_SOURCE_DIR}/drivers/bus/i2c/i2c_registry.c)
    target_include_directories(test_i2c_hid_probe PRIVATE ${CMAKE_SOURCE_DIR}/drivers/input/i2c_hid ${CMAKE_SOURCE_DIR}/drivers/bus/i2c)
    add_test(NAME host_test_i2c_hid_probe COMMAND test_i2c_hid_probe)

    add_executable(test_i2c_hid_failures ${CMAKE_SOURCE_DIR}/tests/host/drivers/test_i2c_hid_failures.c ${CMAKE_SOURCE_DIR}/drivers/input/i2c_hid/i2c_hid.c ${CMAKE_SOURCE_DIR}/drivers/bus/i2c/i2c_core.c ${CMAKE_SOURCE_DIR}/drivers/bus/i2c/i2c_registry.c)
    target_include_directories(test_i2c_hid_failures PRIVATE ${CMAKE_SOURCE_DIR}/drivers/input/i2c_hid ${CMAKE_SOURCE_DIR}/drivers/bus/i2c)
    add_test(NAME host_test_i2c_hid_failures COMMAND test_i2c_hid_failures)

    # VirtIO Input Host Tests
    add_executable(test_virtio_input ${CMAKE_SOURCE_DIR}/tests/host/drivers/test_virtio_input.c ${CMAKE_SOURCE_DIR}/drivers/input/virtio_input/virtio_input.c)
    target_include_directories(test_virtio_input PRIVATE ${CMAKE_SOURCE_DIR}/drivers/input/virtio_input ${CMAKE_SOURCE_DIR}/drivers/input/include ${CMAKE_SOURCE_DIR}/uapi)
    add_test(NAME host_test_virtio_input COMMAND test_virtio_input)
endif()
add_executable(test_fdt_parser test_fdt_parser.c ../../hal/common/fdt_parser.c)
target_include_directories(test_fdt_parser PRIVATE ../../include ../../kernel/include ../../boot/include)
