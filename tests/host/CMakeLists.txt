project(bharat_host_tests C)

add_executable(test_tlb_shootdown test_tlb_shootdown.c ../../kernel/src/mm/tlb/tlb_coordinator.c ../benchmark_stubs.c mock_panic.c ../../lib/msg/wire.c)
target_include_directories(test_tlb_shootdown PRIVATE ../../kernel/include ../../services/monitor/generated ../../subsys/include)
target_link_libraries(test_tlb_shootdown PRIVATE c pthread)
add_test(NAME test_tlb_shootdown COMMAND test_tlb_shootdown)

add_executable(test_tlb_stress test_tlb_stress.c ../../kernel/src/mm/tlb/tlb_coordinator.c ../benchmark_stubs.c mock_panic.c ../../lib/msg/wire.c)
target_include_directories(test_tlb_stress PRIVATE ../../kernel/include ../../services/monitor/generated ../../subsys/include)
target_link_libraries(test_tlb_stress PRIVATE c pthread)
add_test(NAME test_tlb_stress COMMAND test_tlb_stress)

add_executable(test_console console/test_console.c
               ../../kernel/src/console/console_format.c
               ../../kernel/src/console/console_buffer.c
               ../../kernel/src/console/console_router.c
               ../../kernel/src/console/console_policy.c
               ../../kernel/src/console/console_core.c
               ../../kernel/src/console/console_panic.c)
target_include_directories(test_console PRIVATE ../../kernel/include ../../kernel/src)
target_compile_definitions(test_console PRIVATE BHARAT_HOST_TEST)
target_link_libraries(test_console PRIVATE c)
add_test(NAME test_console COMMAND test_console)
