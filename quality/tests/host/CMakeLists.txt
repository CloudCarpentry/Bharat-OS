# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.20)

add_executable(host_test_cap_generation_production test_cap_generation_policy.c)
target_include_directories(host_test_cap_generation_production PRIVATE
    "${CMAKE_SOURCE_DIR}/core/kernel/include")
add_test(NAME host_test_cap_generation_production COMMAND host_test_cap_generation_production)
set_tests_properties(host_test_cap_generation_production PROPERTIES LABELS "capability")

add_executable(host_test_cap_generation_legacy test_cap_generation_policy.c)
target_include_directories(host_test_cap_generation_legacy PRIVATE
    "${CMAKE_SOURCE_DIR}/core/kernel/include")
target_compile_definitions(host_test_cap_generation_legacy PRIVATE
    BHARAT_ENABLE_LEGACY_CAP_TESTS=1)
add_test(NAME host_test_cap_generation_legacy COMMAND host_test_cap_generation_legacy)
set_tests_properties(host_test_cap_generation_legacy PROPERTIES LABELS "capability")

add_executable(test_servicemgr
    "${CMAKE_CURRENT_SOURCE_DIR}/test_servicemgr.c"
    "${CMAKE_SOURCE_DIR}/core/services/servicemgr/servicemgr.c"
    "${CMAKE_SOURCE_DIR}/core/lib/ipc/src/ipc.c"
)

target_include_directories(test_servicemgr PRIVATE
    "${CMAKE_SOURCE_DIR}/interface/include"
    "${CMAKE_SOURCE_DIR}/interface"
    "${CMAKE_BINARY_DIR}/generated/include"
    "${CMAKE_SOURCE_DIR}/core/lib/ipc/include"
    "${CMAKE_SOURCE_DIR}/core/lib/cap/include"
    "${CMAKE_SOURCE_DIR}/core/kernel/include"
    "${CMAKE_SOURCE_DIR}/core/lib/include"
    "${CMAKE_SOURCE_DIR}/core/lib/runtime/include"
    "${CMAKE_SOURCE_DIR}/core/lib/namesvc/include"
    "${CMAKE_SOURCE_DIR}/core/services/servicemgr"
)

target_compile_definitions(test_servicemgr PRIVATE BHARAT_HOST_TEST)

add_test(NAME host_test_servicemgr COMMAND test_servicemgr)

add_executable(host_test_sched_selection
    sched/test_sched_selection.c
    ${CMAKE_SOURCE_DIR}/core/kernel/src/sched/constraint_apply.c)
target_include_directories(host_test_sched_selection PRIVATE
    ${CMAKE_SOURCE_DIR}/core/kernel/include
    ${CMAKE_SOURCE_DIR}/core/kernel/src/sched
    ${CMAKE_SOURCE_DIR}/core/hal/include
    ${CMAKE_SOURCE_DIR}/core/lib/include
    ${CMAKE_SOURCE_DIR}/core/boot/include
    ${CMAKE_SOURCE_DIR}/core/personalities/runtime/include
    ${CMAKE_SOURCE_DIR}/interface/include
    ${CMAKE_BINARY_DIR}/generated/include)
target_compile_definitions(host_test_sched_selection PRIVATE TESTING=1)
add_test(NAME host_test_sched_selection COMMAND host_test_sched_selection)
set_tests_properties(host_test_sched_selection PROPERTIES LABELS "scheduler")

add_executable(host_test_sched_diag
    sched/test_sched_diag.c
    ${CMAKE_SOURCE_DIR}/core/kernel/src/sched/sched_diag.c)
target_include_directories(host_test_sched_diag PRIVATE
    ${CMAKE_SOURCE_DIR}/core/kernel/include
    ${CMAKE_SOURCE_DIR}/core/hal/include
    ${CMAKE_SOURCE_DIR}/interface/include
    ${CMAKE_BINARY_DIR}/generated/include)
add_test(NAME host_test_sched_diag COMMAND host_test_sched_diag)
set_tests_properties(host_test_sched_diag PROPERTIES LABELS "scheduler")

add_executable(host_test_cpu_partition
    sched/test_cpu_partition.c
    ${CMAKE_SOURCE_DIR}/core/kernel/src/sched/cpu_partition.c
    ${CMAKE_SOURCE_DIR}/core/kernel/src/sched/cpu_partition_validate.c)
target_include_directories(host_test_cpu_partition PRIVATE
    ${CMAKE_SOURCE_DIR}/core/kernel/include
    ${CMAKE_SOURCE_DIR}/interface/include)
add_test(NAME host_test_cpu_partition COMMAND host_test_cpu_partition)
set_tests_properties(host_test_cpu_partition PROPERTIES LABELS "scheduler")

# Phase 3: Core IPC/Profile Tests
add_executable(test_ipc_profile test_ipc_profile.c ../../kernel/src/profile/profile.c ../../kernel/src/ipc/ipc_traffic.c)
target_include_directories(test_ipc_profile PRIVATE ../../kernel/include ../../include ../../core/lib/include)
add_test(NAME host_test_ipc_profile COMMAND test_ipc_profile)
set_tests_properties(host_test_ipc_profile PROPERTIES LABELS "ipc")

add_executable(test_ipc_urpc_reliability test_ipc_urpc_reliability.c ../../kernel/src/ipc/contract_validate.c ../../kernel/src/ipc/ipc_status.c ../../kernel/src/ipc/ipc_traffic.c ../../kernel/src/urpc/urpc_channel.c ../../kernel/src/ipc/ipc_profile_policy.c ../../kernel/src/mm/mem_model.c ../../kernel/src/profile/profile.c)
target_include_directories(test_ipc_urpc_reliability PRIVATE ../../kernel/include ../../include ../../core/lib/include)
add_test(NAME host_test_ipc_urpc_reliability COMMAND test_ipc_urpc_reliability)
set_tests_properties(host_test_ipc_urpc_reliability PROPERTIES LABELS "ipc")

add_executable(test_ipc_urpc_policy_rt_mpu test_ipc_urpc_policy_matrix.c ../../kernel/src/ipc/ipc_profile_policy.c ../../kernel/src/mm/mem_model.c ../../kernel/src/profile/profile.c)
target_include_directories(test_ipc_urpc_policy_rt_mpu PRIVATE ../../kernel/include ../../include ../../core/lib/include)
target_compile_definitions(test_ipc_urpc_policy_rt_mpu PRIVATE CONFIG_PROFILE_AUTOMOBILE=1 CONFIG_MEM_MODEL_MPU=1 BHARAT_KERNEL_PROFILE_RT=1 EXPECT_PREFER_URPC=1 EXPECT_ALLOW_BULK_IPC=0 EXPECT_URPC_MAX=32 EXPECT_CONTROL_TRANSPORT=IPC_TRANSPORT_URPC EXPECT_BULK_TRANSPORT=IPC_TRANSPORT_URPC)
add_test(NAME host_test_ipc_urpc_policy_rt_mpu COMMAND test_ipc_urpc_policy_rt_mpu)
set_tests_properties(host_test_ipc_urpc_policy_rt_mpu PROPERTIES LABELS "ipc")

add_executable(test_ipc_urpc_policy_gp_mmu_lite test_ipc_urpc_policy_matrix.c ../../kernel/src/ipc/ipc_profile_policy.c ../../kernel/src/mm/mem_model.c ../../kernel/src/profile/profile.c)
target_include_directories(test_ipc_urpc_policy_gp_mmu_lite PRIVATE ../../kernel/include ../../include ../../core/lib/include)
target_compile_definitions(test_ipc_urpc_policy_gp_mmu_lite PRIVATE CONFIG_MEM_MODEL_MMU_LITE=1 EXPECT_PREFER_URPC=0 EXPECT_ALLOW_BULK_IPC=1 EXPECT_URPC_MAX=64 EXPECT_CONTROL_TRANSPORT=IPC_TRANSPORT_ENDPOINT EXPECT_BULK_TRANSPORT=IPC_TRANSPORT_ENDPOINT)
add_test(NAME host_test_ipc_urpc_policy_gp_mmu_lite COMMAND test_ipc_urpc_policy_gp_mmu_lite)
set_tests_properties(host_test_ipc_urpc_policy_gp_mmu_lite PROPERTIES LABELS "ipc")

add_executable(test_ipc_urpc_policy_mix_mmu_full test_ipc_urpc_policy_matrix.c ../../kernel/src/ipc/ipc_profile_policy.c ../../kernel/src/mm/mem_model.c ../../kernel/src/profile/profile.c)
target_include_directories(test_ipc_urpc_policy_mix_mmu_full PRIVATE ../../kernel/include ../../include ../../core/lib/include)
target_compile_definitions(test_ipc_urpc_policy_mix_mmu_full PRIVATE BHARAT_KERNEL_PROFILE_MIX=1 EXPECT_PREFER_URPC=1 EXPECT_ALLOW_BULK_IPC=1 EXPECT_URPC_MAX=256 EXPECT_CONTROL_TRANSPORT=IPC_TRANSPORT_URPC EXPECT_BULK_TRANSPORT=IPC_TRANSPORT_ENDPOINT)
add_test(NAME host_test_ipc_urpc_policy_mix_mmu_full COMMAND test_ipc_urpc_policy_mix_mmu_full)
set_tests_properties(host_test_ipc_urpc_policy_mix_mmu_full PROPERTIES LABELS "ipc")

# Phase 3: Crypto Tests
add_executable(test_secure_mem test_secure_mem.c)
target_include_directories(test_secure_mem PRIVATE ../../kernel/include)

add_test(NAME host_test_secure_mem COMMAND test_secure_mem)

add_executable(test_memops_conformance
    test_memops_conformance.c
    ../../hal/common/memops/mem_scalar.c
    ../../hal/common/memops/mem_dispatch.c)
target_include_directories(test_memops_conformance PRIVATE ../../hal/include)
target_compile_options(test_memops_conformance PRIVATE
    -fno-builtin-memcpy -fno-builtin-memset -fno-builtin-memmove)
add_test(NAME host_test_memops_conformance COMMAND test_memops_conformance)

add_executable(test_crypto_protocol_dispatch services/test_crypto_protocol_dispatch.c ../../core/services/security/crypto/protocol.c ../../core/services/security/crypto/dispatch.c)
target_include_directories(test_crypto_protocol_dispatch PRIVATE ../../core/services/security/crypto ../../kernel/include)
add_test(NAME host_test_crypto_protocol_dispatch COMMAND test_crypto_protocol_dispatch)

# Phase 3: NetMgr Tests
add_executable(test_netmgr_address_table services/test_netmgr_address_table.c ../../core/services/netmgr/src/address_table.c ../../core/lib/runtime/src/freestanding_string.c)
target_include_directories(test_netmgr_address_table PRIVATE ../../core/services/netmgr/src ../../core/services/netmgr/include ../../core/services/core/subsysmgr/include ../../core/lib/net/include ../../core/lib/runtime/include)
add_test(NAME host_test_netmgr_address_table COMMAND test_netmgr_address_table)

add_executable(test_pmm_pcache test_pmm_pcache.c ../../kernel/src/mm/pmm/pmm_init.c ../../kernel/src/mm/pmm/pmm_buddy.c ../../kernel/src/mm/pmm/pmm_fault.c ../../kernel/src/mm/pmm/pmm_pcache.c ../../kernel/src/mm/pmm/numa.c)
target_include_directories(test_pmm_pcache PRIVATE ${CMAKE_SOURCE_DIR}/core/kernel/include ${CMAKE_SOURCE_DIR}/core/kernel/src ${CMAKE_SOURCE_DIR}/interface/include ${CMAKE_SOURCE_DIR}/core/lib/include ${CMAKE_SOURCE_DIR}/core/boot/include)
add_test(NAME host_test_pmm_pcache COMMAND test_pmm_pcache)
set_tests_properties(host_test_pmm_pcache PROPERTIES LABELS "pmm")

add_executable(test_irq_state test_irq_state.c)
target_include_directories(test_irq_state PRIVATE ${CMAKE_SOURCE_DIR}/core/kernel/include ${CMAKE_SOURCE_DIR}/core/hal/include)
add_test(NAME host_test_irq_state COMMAND test_irq_state)
add_executable(test_netmgr_interface_table services/test_netmgr_interface_table.c ../../core/services/netmgr/src/interface_table.c ../../core/lib/runtime/src/freestanding_string.c)
target_include_directories(test_netmgr_interface_table PRIVATE ../../core/services/netmgr/src ../../core/services/netmgr/include ../../core/services/core/subsysmgr/include ../../core/lib/net/include ../../core/lib/runtime/include)
add_test(NAME host_test_netmgr_interface_table COMMAND test_netmgr_interface_table)

add_executable(test_smp_tlb_active_cores test_smp_tlb_active_cores.c ../../kernel/src/mm/tlb/tlb_shootdown.c ../../kernel/src/mm/tlb/tlb_pending.c ../../kernel/src/mm/tlb/tlb_stats.c host_stubs.c ../../kernel/src/lib/base/string.c)
target_include_directories(test_smp_tlb_active_cores PRIVATE ${CMAKE_SOURCE_DIR}/core/kernel/include ${CMAKE_SOURCE_DIR}/core/kernel/src ${CMAKE_SOURCE_DIR}/interface/include ${CMAKE_SOURCE_DIR}/core/boot/include ${CMAKE_SOURCE_DIR}/core/lib/include ${CMAKE_BINARY_DIR}/generated/include ${CMAKE_SOURCE_DIR}/core/services/monitor/generated)
add_test(NAME host_test_smp_tlb_active_cores COMMAND test_smp_tlb_active_cores)

# Memory host conformance expansion (2026-03-23)
# HAL contracts + generic PT/TLB contracts
add_executable(host_test_hal_pt_common ../../tests/mm/pt_common_tests.c ../../hal/hal_pt.c)
target_include_directories(host_test_hal_pt_common PRIVATE ${CMAKE_SOURCE_DIR}/core/kernel/include ${CMAKE_SOURCE_DIR}/interface/include ${CMAKE_SOURCE_DIR}/core/boot/include)
add_test(NAME host_test_hal_pt_common COMMAND host_test_hal_pt_common)
set_tests_properties(host_test_hal_pt_common PROPERTIES LABELS "vm")

add_executable(host_test_hal_pt_fallbacks ../../tests/test_hal_pt_fallbacks.c ../../hal/hal_pt.c)
target_include_directories(host_test_hal_pt_fallbacks PRIVATE ${CMAKE_SOURCE_DIR}/core/kernel/include ${CMAKE_SOURCE_DIR}/interface/include ${CMAKE_SOURCE_DIR}/core/boot/include)
add_test(NAME host_test_hal_pt_fallbacks COMMAND host_test_hal_pt_fallbacks)
set_tests_properties(host_test_hal_pt_fallbacks PROPERTIES LABELS "vm")

add_executable(host_test_hal_caps_consistency ../../tests/test_hal_caps_consistency.c ../../hal/hal_pt.c)
target_include_directories(host_test_hal_caps_consistency PRIVATE ${CMAKE_SOURCE_DIR}/core/kernel/include ${CMAKE_SOURCE_DIR}/interface/include ${CMAKE_SOURCE_DIR}/core/boot/include)
add_test(NAME host_test_hal_caps_consistency COMMAND host_test_hal_caps_consistency)

add_executable(host_test_mon_vm test_mon_vm.c host_stubs.c ../../kernel/src/monitor/mon_vm_dispatch.c ../../kernel/src/monitor/mon_vm_service.c ../../kernel/src/monitor/mon_vm_state.c)
target_include_directories(host_test_mon_vm PRIVATE ${CMAKE_SOURCE_DIR}/core/kernel/include ${CMAKE_SOURCE_DIR}/core/kernel/src ${CMAKE_SOURCE_DIR}/interface/include ${CMAKE_SOURCE_DIR}/core/lib/include)
add_test(NAME host_test_mon_vm COMMAND host_test_mon_vm)
set_tests_properties(host_test_mon_vm PROPERTIES LABELS "vm")

# VMM / ASpace / VM base-object conformance currently remains in top-level tests
# (`test_vmm_aspace`) due additional prot-domain and arch-cap dependencies.

add_executable(host_test_vmm_fault ../test_vmm_fault.c host_stubs.c ../../kernel/src/mm/vm/fault/fault.c ../../kernel/src/mm/vm/objects/vm_object.c ../../kernel/src/mm/vm/objects/vm_object_refcount.c ../../kernel/src/mm/vm/aspace/vm_space.c ../../kernel/src/mm/vm/aspace/aspace.c ../../kernel/src/mm/pmm/early_alloc.c ../../kernel/src/mm/pmm/pmm_init.c ../../kernel/src/mm/pmm/pmm_buddy.c ../../kernel/src/mm/pmm/pmm_fault.c ../../kernel/src/mm/pmm/pt_pool.c ../../kernel/src/mm/tlb/tlb_shootdown.c ../../kernel/src/cap/cap_cspace.c ../../kernel/src/cap/cap_dispatch.c ../../kernel/src/cap/cap_revoke.c ../../kernel/src/cap/cap_policy.c ../benchmark_stubs.c ../../kernel/src/lib/base/string.c ../../hal/common/memops/mem_scalar.c mock_panic.c)
target_include_directories(host_test_vmm_fault PRIVATE ${CMAKE_SOURCE_DIR}/core/kernel/include ${CMAKE_SOURCE_DIR}/core/kernel/src ${CMAKE_SOURCE_DIR}/interface/include ${CMAKE_SOURCE_DIR}/core/boot/include ${CMAKE_SOURCE_DIR}/core/lib/include ${CMAKE_BINARY_DIR}/generated/include)
target_compile_definitions(host_test_vmm_fault PRIVATE TESTING=1)
add_test(NAME host_test_vmm_fault COMMAND host_test_vmm_fault)
set_tests_properties(host_test_vmm_fault PROPERTIES LABELS "vm")

add_executable(test_uapi_abi test_uapi_abi.c)
target_include_directories(test_uapi_abi PRIVATE ${CMAKE_SOURCE_DIR}/interface/include)

add_executable(host_test_syscall_return_x86_64
    syscall/test_syscall_extract_x86_64.c
    ${CMAKE_SOURCE_DIR}/core/arch/x86/x86_64/syscall_extract.c)
target_include_directories(host_test_syscall_return_x86_64 PRIVATE
    ${CMAKE_SOURCE_DIR}/core/kernel/include
    ${CMAKE_SOURCE_DIR}/core/arch/x86/x86_64/include
    ${CMAKE_SOURCE_DIR}/core/personalities/runtime/include
    ${CMAKE_BINARY_DIR}/generated/include
    ${CMAKE_SOURCE_DIR}/interface/include)
add_test(NAME host_test_syscall_return_x86_64 COMMAND host_test_syscall_return_x86_64)
add_test(NAME host_test_uapi_abi COMMAND test_uapi_abi)

add_executable(host_test_robotics_demo
    robotics/test_robotics_demo.c
    ${CMAKE_SOURCE_DIR}/core/drivers/sensor/virtual/virtual_sensors.c
    ${CMAKE_SOURCE_DIR}/core/drivers/actuator/virtual/virtual_motors.c
    ${CMAKE_SOURCE_DIR}/core/services/device/sensormgr/sensormgr.c
    ${CMAKE_SOURCE_DIR}/core/services/device/actuator_mgr/actuator_mgr.c
    ${CMAKE_SOURCE_DIR}/core/platform/qemu/robot/robot_platform.c
    ${CMAKE_SOURCE_DIR}/core/stacks/robotics/robotics_demo.c)
target_include_directories(host_test_robotics_demo PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/interface/include)
add_test(NAME host_test_robotics_demo COMMAND host_test_robotics_demo)

add_executable(host_test_libmsg_crc
    test_libmsg_crc.c
    ${CMAKE_SOURCE_DIR}/core/lib/msg/crc.c
    ${CMAKE_SOURCE_DIR}/core/lib/msg/crc_generic.c)
target_include_directories(host_test_libmsg_crc PRIVATE
    ${CMAKE_SOURCE_DIR}/core/lib/msg)
add_test(NAME host_test_libmsg_crc COMMAND host_test_libmsg_crc)

add_executable(host_test_framebuffer gfx/test_framebuffer.c ../../core/lib/gfx/framebuffer.c)
target_include_directories(host_test_framebuffer PRIVATE ../../core/lib/gfx ../../)
add_test(NAME host_test_framebuffer COMMAND host_test_framebuffer)

add_executable(host_test_boot_display_bridge gfx/test_boot_display_bridge.c)
target_include_directories(host_test_boot_display_bridge PRIVATE ../../core/kernel/include ../../interface/include ../../core/boot/include ../../core/hal/include ../../)
add_test(NAME host_test_boot_display_bridge COMMAND host_test_boot_display_bridge)

add_executable(host_test_region gfx/test_region.c ../../core/lib/gfx/region.c)
target_include_directories(host_test_region PRIVATE ../../core/lib/gfx ../../)
add_test(NAME host_test_region COMMAND host_test_region)

add_executable(host_test_input_layout input/test_input_layout.c)
target_include_directories(host_test_input_layout PRIVATE ../../interface/include)
add_test(NAME host_test_input_layout COMMAND host_test_input_layout)

add_executable(test_hal_cpu_features
    test_hal_cpu_features.c
    ../../arch/common/cpu_caps_state.c
    ../../hal/common/cpu_features.c
    ../../hal/common/memops/mem_scalar.c
    ../../hal/common/memops/mem_dispatch.c)
target_include_directories(test_hal_cpu_features PRIVATE
    ${CMAKE_SOURCE_DIR}/core/kernel/include
    ${CMAKE_SOURCE_DIR}/core/hal/include
    ${CMAKE_SOURCE_DIR}/interface/include
    ${CMAKE_SOURCE_DIR}/core/lib/include
    ${CMAKE_SOURCE_DIR})
add_test(NAME host_test_hal_cpu_features COMMAND test_hal_cpu_features)

add_executable(test_hal_capability_lifecycle
    test_hal_capability_lifecycle.c
    ../../arch/common/cpu_caps_state.c
    ../../hal/common/cpu_features.c
    ../../hal/common/memops/mem_scalar.c
    ../../hal/common/memops/mem_dispatch.c
    ../../hal/common/hw_caps.c
    ../../kernel/src/core/primitive_registry.c)
target_include_directories(test_hal_capability_lifecycle PRIVATE
    ${CMAKE_SOURCE_DIR}/core/kernel/include
    ${CMAKE_SOURCE_DIR}/core/hal/include
    ${CMAKE_SOURCE_DIR}/interface/include
    ${CMAKE_SOURCE_DIR}/core/lib/include
    ${CMAKE_SOURCE_DIR})
add_test(NAME host_test_hal_capability_lifecycle COMMAND test_hal_capability_lifecycle)
set_tests_properties(host_test_hal_capability_lifecycle PROPERTIES LABELS "capability")

# ---------------------------------------------------------
# Phase 2: Runtime completeness Tests
# ---------------------------------------------------------
add_executable(test_process_manager test_process_manager.c ../../core/services/process_manager/process_manager.c ../../core/lib/ipc/src/ipc.c)
target_include_directories(test_process_manager PRIVATE ../../include ../../core/lib/ipc/include ../../core/lib/cap/include ../../kernel/include)
add_test(NAME host_test_process_manager COMMAND test_process_manager)

add_executable(test_vm_manager test_vm_manager.c ../../core/services/vm_manager/vm_manager.c ../../core/lib/ipc/src/ipc.c)
target_include_directories(test_vm_manager PRIVATE ../../include ../../core/lib/ipc/include ../../core/lib/cap/include ../../kernel/include)
add_test(NAME host_test_vm_manager COMMAND test_vm_manager)
set_tests_properties(host_test_vm_manager PROPERTIES LABELS "vm")

# Modern PROCVM-P0-001 Vertical Slice Tests
add_executable(test_elf_load_plan
    process_vm/test_elf_load_plan.c
    ../../core/lib/elf/elf_load_plan.c
    ../../core/lib/elf/elf_parser.c
)
target_include_directories(test_elf_load_plan PRIVATE
    ../../include
    ../../interface/include
)
target_link_libraries(test_elf_load_plan PRIVATE bharat_freestanding)
add_test(NAME host_test_elf_load_plan COMMAND test_elf_load_plan)

add_executable(test_pm_handle_lifecycle
    process_vm/test_pm_handle_lifecycle.c
    ../../core/lib/handle/handle_table.c
)
target_include_directories(test_pm_handle_lifecycle PRIVATE
    ../../include
    ../../interface/include
    ../../core/lib/handle
)
target_link_libraries(test_pm_handle_lifecycle PRIVATE bharat_freestanding)
add_test(NAME host_test_pm_handle_lifecycle COMMAND test_pm_handle_lifecycle)

add_executable(test_vm_handle_lifecycle
    process_vm/test_vm_handle_lifecycle.c
    ../../core/lib/handle/handle_table.c
)
target_include_directories(test_vm_handle_lifecycle PRIVATE
    ../../include
    ../../interface/include
    ../../core/lib/handle
)
target_link_libraries(test_vm_handle_lifecycle PRIVATE bharat_freestanding)
add_test(NAME host_test_vm_handle_lifecycle COMMAND test_vm_handle_lifecycle)
set_tests_properties(host_test_vm_handle_lifecycle PROPERTIES LABELS "vm")

add_executable(test_pm_contract_v1
    process_vm/test_pm_contract_v1.c
)
target_include_directories(test_pm_contract_v1 PRIVATE
    ../../include
    ../../interface/include
)
target_link_libraries(test_pm_contract_v1 PRIVATE bharat_freestanding)
add_test(NAME host_test_pm_contract_v1 COMMAND test_pm_contract_v1)

add_executable(test_pm_spawn_transaction
    process_vm/test_pm_spawn_transaction.c
    ../../core/services/process_manager/process_manager.c
    ../../core/lib/handle/handle_table.c
    ../../core/lib/elf/elf_load_plan.c
    ../../core/lib/elf/elf_parser.c
)
target_include_directories(test_pm_spawn_transaction PRIVATE
    ../../include
    ../../interface/include
    ../../kernel/include
    ../../lib/handle
)
target_link_libraries(test_pm_spawn_transaction PRIVATE bharat_freestanding)
add_test(NAME host_test_pm_spawn_transaction COMMAND test_pm_spawn_transaction)

add_executable(test_pm_wait_reap
    process_vm/test_pm_wait_reap.c
    ../../core/services/process_manager/process_manager.c
    ../../core/lib/handle/handle_table.c
    ../../core/lib/elf/elf_load_plan.c
    ../../core/lib/elf/elf_parser.c
)
target_include_directories(test_pm_wait_reap PRIVATE
    ../../include
    ../../interface/include
    ../../kernel/include
    ../../lib/handle
)
target_link_libraries(test_pm_wait_reap PRIVATE bharat_freestanding)
add_test(NAME host_test_pm_wait_reap COMMAND test_pm_wait_reap)

add_executable(test_vm_authority_integration
    process_vm/test_vm_authority_integration.c
    ../../core/services/vm_manager/vm_manager.c
    ../../core/lib/handle/handle_table.c
)
target_include_directories(test_vm_authority_integration PRIVATE
    ../../include
    ../../interface/include
    ../../kernel/include
    ../../lib/handle
)
target_link_libraries(test_vm_authority_integration PRIVATE bharat_freestanding)
add_test(NAME host_test_vm_authority_integration COMMAND test_vm_authority_integration)
set_tests_properties(host_test_vm_authority_integration PROPERTIES LABELS "vm")

add_executable(test_procvm_e2e
    process_vm/test_procvm_e2e.c
    ../../core/services/process_manager/process_manager.c
    ../../core/lib/handle/handle_table.c
    ../../core/lib/elf/elf_load_plan.c
    ../../core/lib/elf/elf_parser.c
)
target_include_directories(test_procvm_e2e PRIVATE
    ../../include
    ../../interface/include
    ../../kernel/include
    ../../lib/handle
)
target_link_libraries(test_procvm_e2e PRIVATE bharat_freestanding)
add_test(NAME host_test_procvm_e2e COMMAND test_procvm_e2e)
set_tests_properties(host_test_procvm_e2e PROPERTIES LABELS "vm")

add_executable(test_procvm_stress
    process_vm/test_procvm_stress.c
    ../../core/services/process_manager/process_manager.c
    ../../core/lib/handle/handle_table.c
    ../../core/lib/elf/elf_load_plan.c
    ../../core/lib/elf/elf_parser.c
)
target_include_directories(test_procvm_stress PRIVATE
    ../../include
    ../../interface/include
    ../../kernel/include
    ../../lib/handle
)
target_link_libraries(test_procvm_stress PRIVATE bharat_freestanding)
add_test(NAME host_test_procvm_stress COMMAND test_procvm_stress)
set_tests_properties(host_test_procvm_stress PROPERTIES LABELS "vm")

add_executable(test_servicemgr_dup test_servicemgr.c ../../core/services/servicemgr/servicemgr.c ../../core/lib/ipc/src/ipc.c)
target_include_directories(test_servicemgr_dup PRIVATE ../../include ../../core/lib/ipc/include ../../core/lib/cap/include ../../kernel/include)
add_test(NAME host_test_servicemgr_dup COMMAND test_servicemgr_dup)

add_executable(test_init test_init.c ../../core/services/core/init/init_runtime.c ../../core/services/core/init/init_status.c ../../core/services/core/init/init_handoff.c ../../core/services/core/init/init_rollback.c)
target_include_directories(test_init PRIVATE ../../include ../../kernel/include ../../core/lib/runtime/include ../../core/lib/cap/include ../../core/lib/ipc/include)
add_test(NAME host_test_init COMMAND test_init)

add_executable(test_init_rollback test_init_rollback.c ../../core/services/core/init/init_rollback.c)
target_include_directories(test_init_rollback PRIVATE ../../interface/include ../../core/services/core/init)
add_test(NAME host_test_init_rollback COMMAND test_init_rollback)

add_executable(test_ktime time/test_ktime.c ../../core/kernel/src/time/ktime.c ../../core/hal/common/hal_timer_common.c ../../core/kernel/src/core/cpu_local.c)

target_include_directories(test_ktime PRIVATE ../../core/kernel/include ../../interface/include ../../core/hal/include ../../core/boot/include)

add_test(NAME host_test_ktime COMMAND test_ktime)


add_executable(test_e2e_service_runtime test_e2e_service_runtime.c ../../core/services/process_manager/process_manager.c ../../core/services/vm_manager/vm_manager.c ../../core/services/servicemgr/servicemgr.c ../../core/lib/ipc/src/ipc.c)
target_include_directories(test_e2e_service_runtime PRIVATE ../../include ../../core/lib/ipc/include ../../core/lib/cap/include ../../kernel/include)
add_test(NAME host_test_e2e_service_runtime COMMAND test_e2e_service_runtime)

add_executable(test_runtime_startup
    test_runtime_startup.c
    ../../core/lib/runtime/src/runtime.c
)
target_include_directories(test_runtime_startup PRIVATE
    ../../interface/include
    ../../core/lib/runtime/include
)
add_test(NAME host_test_runtime_startup COMMAND test_runtime_startup)

if(BHARAT_BUILD_HOST_TESTS)
    # VirtIO transport mechanism tests use a synthetic register window. They
    # exercise no QEMU policy or service-layer device selection.
    add_executable(test_virtio_mmio
        drivers/test_virtio_mmio.c
        ${CMAKE_SOURCE_DIR}/core/drivers/virtio/mmio/virtio_mmio.c
    )
    target_include_directories(test_virtio_mmio PRIVATE
        ${CMAKE_SOURCE_DIR}/core/drivers/virtio/mmio
    )
    add_test(NAME host_test_virtio_mmio COMMAND test_virtio_mmio)

    # I2C Core Host Tests
    add_executable(test_i2c_registry ${CMAKE_SOURCE_DIR}/core/tests/host/drivers/test_i2c_registry.c ${CMAKE_SOURCE_DIR}/core/drivers/bus/i2c/i2c_core.c ${CMAKE_SOURCE_DIR}/core/drivers/bus/i2c/i2c_registry.c)
    target_include_directories(test_i2c_registry PRIVATE ${CMAKE_SOURCE_DIR}/core/drivers/bus/i2c)
    add_test(NAME host_test_i2c_registry COMMAND test_i2c_registry)

    add_executable(test_i2c_transfer_mock ${CMAKE_SOURCE_DIR}/core/tests/host/drivers/test_i2c_transfer_mock.c ${CMAKE_SOURCE_DIR}/core/drivers/bus/i2c/i2c_core.c ${CMAKE_SOURCE_DIR}/core/drivers/bus/i2c/i2c_registry.c)
    target_include_directories(test_i2c_transfer_mock PRIVATE ${CMAKE_SOURCE_DIR}/core/drivers/bus/i2c)
    add_test(NAME host_test_i2c_transfer_mock COMMAND test_i2c_transfer_mock)

    # SPI Core Host Tests
    add_executable(test_spi_registry ${CMAKE_SOURCE_DIR}/core/tests/host/drivers/test_spi_registry.c ${CMAKE_SOURCE_DIR}/core/drivers/bus/spi/spi_core.c ${CMAKE_SOURCE_DIR}/core/drivers/bus/spi/spi_registry.c)
    target_include_directories(test_spi_registry PRIVATE ${CMAKE_SOURCE_DIR}/core/drivers/bus/spi)
    add_test(NAME host_test_spi_registry COMMAND test_spi_registry)

    add_executable(test_spi_transfer_mock ${CMAKE_SOURCE_DIR}/core/tests/host/drivers/test_spi_transfer_mock.c ${CMAKE_SOURCE_DIR}/core/drivers/bus/spi/spi_core.c ${CMAKE_SOURCE_DIR}/core/drivers/bus/spi/spi_registry.c)
    target_include_directories(test_spi_transfer_mock PRIVATE ${CMAKE_SOURCE_DIR}/core/drivers/bus/spi)
    add_test(NAME host_test_spi_transfer_mock COMMAND test_spi_transfer_mock)

    # DRM Core Host Tests
    add_executable(test_drm_registry ${CMAKE_SOURCE_DIR}/core/tests/host/drivers/test_drm_registry.c ${CMAKE_SOURCE_DIR}/core/drivers/display/drm/drm_core.c ${CMAKE_SOURCE_DIR}/core/drivers/display/drm/drm_registry.c)
    target_include_directories(test_drm_registry PRIVATE ${CMAKE_SOURCE_DIR}/core/drivers/display/drm)
    add_test(NAME host_test_drm_registry COMMAND test_drm_registry)

    add_executable(test_drm_mode_validation ${CMAKE_SOURCE_DIR}/core/tests/host/drivers/test_drm_mode_validation.c ${CMAKE_SOURCE_DIR}/core/drivers/display/drm/drm_core.c ${CMAKE_SOURCE_DIR}/core/drivers/display/drm/drm_registry.c)
    target_include_directories(test_drm_mode_validation PRIVATE ${CMAKE_SOURCE_DIR}/core/drivers/display/drm)
    add_test(NAME host_test_drm_mode_validation COMMAND test_drm_mode_validation)

    # NVMe Core Host Tests
    add_executable(test_nvme_init_mock ${CMAKE_SOURCE_DIR}/core/tests/host/drivers/test_nvme_init_mock.c ${CMAKE_SOURCE_DIR}/core/drivers/storage/nvme/nvme_core.c ${CMAKE_SOURCE_DIR}/core/drivers/storage/nvme/nvme_admin.c ${CMAKE_SOURCE_DIR}/core/drivers/storage/nvme/nvme_queue.c)
    target_include_directories(test_nvme_init_mock PRIVATE ${CMAKE_SOURCE_DIR}/core/drivers/storage/nvme)
    add_test(NAME host_test_nvme_init_mock COMMAND test_nvme_init_mock)

    # I2C HID Host Tests
    add_executable(test_i2c_hid_probe ${CMAKE_SOURCE_DIR}/core/tests/host/drivers/test_i2c_hid_probe.c ${CMAKE_SOURCE_DIR}/core/drivers/input/i2c_hid/i2c_hid.c ${CMAKE_SOURCE_DIR}/core/drivers/bus/i2c/i2c_core.c ${CMAKE_SOURCE_DIR}/core/drivers/bus/i2c/i2c_registry.c)
    target_include_directories(test_i2c_hid_probe PRIVATE ${CMAKE_SOURCE_DIR}/core/drivers/input/i2c_hid ${CMAKE_SOURCE_DIR}/core/drivers/bus/i2c)
    add_test(NAME host_test_i2c_hid_probe COMMAND test_i2c_hid_probe)

    add_executable(test_i2c_hid_failures ${CMAKE_SOURCE_DIR}/core/tests/host/drivers/test_i2c_hid_failures.c ${CMAKE_SOURCE_DIR}/core/drivers/input/i2c_hid/i2c_hid.c ${CMAKE_SOURCE_DIR}/core/drivers/bus/i2c/i2c_core.c ${CMAKE_SOURCE_DIR}/core/drivers/bus/i2c/i2c_registry.c)
    target_include_directories(test_i2c_hid_failures PRIVATE ${CMAKE_SOURCE_DIR}/core/drivers/input/i2c_hid ${CMAKE_SOURCE_DIR}/core/drivers/bus/i2c)
    add_test(NAME host_test_i2c_hid_failures COMMAND test_i2c_hid_failures)

    # VirtIO Input Host Tests
    add_executable(test_virtio_input
        ${CMAKE_SOURCE_DIR}/core/tests/host/drivers/test_virtio_input.c
        ${CMAKE_SOURCE_DIR}/core/drivers/input/virtio_input/virtio_input.c
        ${CMAKE_SOURCE_DIR}/core/drivers/virtio/core/virtqueue.c
        ${CMAKE_SOURCE_DIR}/core/drivers/virtio/pci/virtio_pci.c
        ../../core/services/device/inputmgr/inputmgr.c
        ../../experience/user/ui/fbui/input/fbui_input_bridge.c
    )
    target_include_directories(test_virtio_input PRIVATE
        ${CMAKE_SOURCE_DIR}/core/drivers/input/virtio_input
        ${CMAKE_SOURCE_DIR}/core/drivers/input/include
        ${CMAKE_SOURCE_DIR}/interface/uapi
        ${CMAKE_SOURCE_DIR}/interface/include
        ${CMAKE_SOURCE_DIR}/core/drivers/virtio/core
        ${CMAKE_SOURCE_DIR}/core/drivers/virtio/pci
        ${CMAKE_SOURCE_DIR}/core/kernel/include
        ${CMAKE_SOURCE_DIR}/core/services/device/inputmgr/include
    )
    add_test(NAME host_test_virtio_input COMMAND test_virtio_input)

    # Memory Model AI-adjacent Primitives Host Test
    add_executable(test_mem_model_ai test_mem_model_ai.c ../../kernel/src/mm/mem_class.c)
    target_include_directories(test_mem_model_ai PRIVATE ${CMAKE_SOURCE_DIR}/core/kernel/include ${CMAKE_SOURCE_DIR}/interface/include ${CMAKE_SOURCE_DIR}/core/lib/include)
    add_test(NAME host_test_mem_model_ai COMMAND test_mem_model_ai)
endif()
add_executable(test_fdt_parser test_fdt_parser.c ../../hal/common/fdt_parser.c)
target_include_directories(test_fdt_parser PRIVATE ../../include ../../kernel/include ../../core/boot/include)

# Personality Compatibility Tests
if(BHARAT_BUILD_HOST_TESTS)
    add_executable(test_linux_syscalls compat/linux/test_linux_syscalls.c ../../core/personalities/compat/linux/linux_compat.c)
    target_include_directories(test_linux_syscalls PRIVATE ../../core/personalities/compat/linux ../../core/services/core/subsysmgr/include ../../include ../../kernel/include ../../core/lib/cap/include ../../core/lib/ipc/include ../../lib)
    add_test(NAME host_test_linux_syscalls COMMAND test_linux_syscalls)

    add_executable(test_android_binder compat/android/test_android_binder.c ../../core/personalities/compat/android/android_binder_dist.c ../../core/personalities/compat/android/android_personality.c)
    target_include_directories(test_android_binder PRIVATE ../../core/personalities/compat/android ../../include)
    add_test(NAME host_test_android_binder COMMAND test_android_binder)
endif()

# Heterogeneous Compute Host Tests
add_executable(test_accel_dispatch
    accel/test_accel_dispatch.c
    ../../core/lib/runtime/backend_dispatch/backend_registry.c
    ../../core/lib/runtime/accel/tensor_dispatch.c
    ../../core/drivers/accel/virt_accel.c
)
target_include_directories(test_accel_dispatch PRIVATE
    ../../core/kernel/include
    ../../interface/include
    ../../interface
    ../../core/lib/include
    ../../core/lib/runtime/include
    ../../
)
add_test(NAME host_test_accel_dispatch COMMAND test_accel_dispatch)

add_executable(test_accel_handle_security
    accel/test_accel_handle_security.c
    ../../core/services/device/accelmgr/accelmgr_broker.c
    ../../core/drivers/accel/virt_accel.c
    ../../core/drivers/accel/virt/virt_gpu.c
    ../../core/lib/cap/src/cap_validate.c
    accel/fake_accelmgr_ipc.c
)
target_include_directories(test_accel_handle_security PRIVATE
    ../../core/kernel/include
    ../../interface/include
    ../../interface
    ../../core/lib/include
    ../../core/lib/cap/include
    ../../core/drivers/accel
    ../../core/drivers/accel/virt
    ../../core/services/device/accelmgr
    ../../core/services/common/runtime/include
    ../../core/lib/ipc/include
    ${CMAKE_CURRENT_SOURCE_DIR}/accel
    ../../
)
target_compile_definitions(test_accel_handle_security PRIVATE ACCELMGR_NO_MAIN=1)
add_test(NAME host_test_accel_handle_security COMMAND test_accel_handle_security)

add_executable(test_accelmgr_broker
    accel/test_accelmgr_broker.c
    ../../core/services/device/accelmgr/accelmgr_broker.c
    ../../core/drivers/accel/virt_accel.c
    ../../core/drivers/accel/virt/virt_gpu.c
    ../../core/lib/cap/src/cap_validate.c
    accel/fake_accelmgr_ipc.c
)
target_include_directories(test_accelmgr_broker PRIVATE
    ../../core/kernel/include
    ../../interface/include
    ../../interface
    ../../core/lib/include
    ../../core/lib/cap/include
    ../../core/drivers/accel
    ../../core/drivers/accel/virt
    ../../core/services/device/accelmgr
    ../../core/services/common/runtime/include
    ../../core/lib/ipc/include
    ${CMAKE_CURRENT_SOURCE_DIR}/accel
    ../../
)
target_compile_definitions(test_accelmgr_broker PRIVATE ACCELMGR_NO_MAIN=1)
add_test(NAME host_test_accelmgr_broker COMMAND test_accelmgr_broker)

add_executable(test_accel_broker_e2e
    accel/test_accel_broker_e2e.c
    ../../core/services/device/accelmgr/accelmgr_broker.c
    ../../core/drivers/accel/virt_accel.c
    ../../core/drivers/accel/virt/virt_gpu.c
    ../../core/lib/cap/src/cap_validate.c
    accel/fake_accelmgr_ipc.c
)
target_include_directories(test_accel_broker_e2e PRIVATE
    ../../core/kernel/include
    ../../interface/include
    ../../interface
    ../../core/lib/include
    ../../core/lib/cap/include
    ../../core/drivers/accel
    ../../core/drivers/accel/virt
    ../../core/services/device/accelmgr
    ../../core/services/common/runtime/include
    ../../core/lib/ipc/include
    ${CMAKE_CURRENT_SOURCE_DIR}/accel
    ../../
)
target_compile_definitions(test_accel_broker_e2e PRIVATE ACCELMGR_NO_MAIN=1)
add_test(NAME host_test_accel_broker_e2e COMMAND test_accel_broker_e2e)

add_subdirectory(ipc)

# SPDX-License-Identifier: MIT
add_executable(host_test_diag_ring test_diag_ring.c ../../core/lib/diag/src/diag_ring.c ../../core/lib/diag/src/diag_emit.c)
target_include_directories(host_test_diag_ring PRIVATE ../../core/lib/diag/include ../../interface/uapi ../../interface/include)
target_compile_definitions(host_test_diag_ring PRIVATE BHARAT_DIAG_ENABLED=1 BHARAT_DIAG_MAX_PAYLOAD=256)
add_test(NAME host_test_diag_ring COMMAND host_test_diag_ring)
add_executable(host_test_diag_contract test_diag_contract.c)
target_include_directories(host_test_diag_contract PRIVATE ../../interface/uapi)
add_test(NAME host_test_diag_contract COMMAND host_test_diag_contract)
add_executable(host_test_diag_collector test_diag_collector.c ../../core/lib/diag/src/diag_ring.c ../../core/services/system/diag/src/diag_collector.c)
target_include_directories(host_test_diag_collector PRIVATE ../../core/lib/diag/include ../../core/services/system/diag/include ../../interface/uapi ../../interface/include)
add_test(NAME host_test_diag_collector COMMAND host_test_diag_collector)

# Seqlock Concurrency Test
add_executable(test_bh_seqlock_concurrency
    test_bh_seqlock_concurrency.c
    ../../../core/kernel/src/ds/bh_seqlock.c
    ../../../core/arch/hal/x86/x86_64/cpu_relax.c
)
target_include_directories(test_bh_seqlock_concurrency PRIVATE
    ../../../core/kernel/include
    ../../../interface/include
)
target_link_libraries(test_bh_seqlock_concurrency PRIVATE pthread)
add_test(NAME host_test_bh_seqlock_concurrency COMMAND test_bh_seqlock_concurrency)
