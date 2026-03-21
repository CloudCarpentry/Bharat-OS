project(bharat_host_tests C)

add_executable(test_tlb_shootdown test_tlb_shootdown.c ../../kernel/src/mm/tlb/tlb_coordinator.c ../benchmark_stubs.c mock_panic.c ../../lib/msg/wire.c)
target_include_directories(test_tlb_shootdown PRIVATE ../../kernel/include ../../services/monitor/generated ../../subsys/include)
target_link_libraries(test_tlb_shootdown PRIVATE c pthread)
add_test(NAME test_tlb_shootdown COMMAND test_tlb_shootdown)

add_executable(test_tlb_stress test_tlb_stress.c ../../kernel/src/mm/tlb/tlb_coordinator.c ../benchmark_stubs.c mock_panic.c ../../lib/msg/wire.c)
target_include_directories(test_tlb_stress PRIVATE ../../kernel/include ../../services/monitor/generated ../../subsys/include)
target_link_libraries(test_tlb_stress PRIVATE c pthread)
add_test(NAME test_tlb_stress COMMAND test_tlb_stress)

add_executable(test_checksum net/test_checksum.c ../../services/netstack/src/checksum.c mock_panic.c)
target_include_directories(test_checksum PRIVATE ../../services/netstack/src)
target_link_libraries(test_checksum PRIVATE c)
add_test(NAME test_checksum COMMAND test_checksum)
add_executable(test_console console/test_console.c
               ../../kernel/src/console/console_format.c
               ../../kernel/src/console/console_buffer.c
               ../../kernel/src/console/console_router.c
               ../../kernel/src/console/console_policy.c
               ../../kernel/src/console/console_core.c
               ../../kernel/src/console/console_panic.c
               ../../kernel/src/console/console_discovery.c
               ../../kernel/src/console/memlog_console.c
               ../../kernel/src/console/console_render_fb.c
               ../../kernel/src/arch/common/memops_scalar.c)
target_include_directories(test_console PRIVATE ../../kernel/include ../../kernel/src)
target_compile_definitions(test_console PRIVATE BHARAT_HOST_TEST)
target_link_libraries(test_console PRIVATE c)
add_test(NAME test_console COMMAND test_console)

add_test(
    NAME test_component_policy_matrix
    COMMAND ${CMAKE_COMMAND}
        -DREPO_ROOT=${CMAKE_SOURCE_DIR}
        -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/policy_matrix
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_component_policy.cmake
)

add_executable(test_virt_can services/can/test_virt_can.c
               ../../drivers/virt_can/virt_can.c
               ../../drivers/can_core/can_controller_core.c)
target_include_directories(test_virt_can PRIVATE ../../drivers/include)
target_compile_definitions(test_virt_can PRIVATE BHARAT_HOST_TEST)
target_link_libraries(test_virt_can PRIVATE c)
add_test(NAME test_virt_can COMMAND test_virt_can)

add_executable(test_interrupt_common hal/test_interrupt_common.c
               ../../kernel/src/hal/interrupt_common.c)
target_include_directories(test_interrupt_common PRIVATE ../../kernel/include ../../include)
target_compile_definitions(test_interrupt_common PRIVATE BHARAT_HOST_TEST BHARAT_IRQ_DISPATCH_MIXED)
target_link_libraries(test_interrupt_common PRIVATE c)
add_test(NAME test_interrupt_common COMMAND test_interrupt_common)
