project(bharat_host_tests C)

add_executable(test_tlb_shootdown test_tlb_shootdown.c ../../kernel/src/mm/tlb/tlb_coordinator.c ../benchmark_stubs.c mock_panic.c ../../lib/msg/wire.c)
target_include_directories(test_tlb_shootdown PRIVATE ../../kernel/include ../../services/monitor/generated ../../subsys/include)
target_link_libraries(test_tlb_shootdown PRIVATE c pthread)
add_test(NAME test_tlb_shootdown COMMAND test_tlb_shootdown)

add_executable(test_tlb_stress test_tlb_stress.c ../../kernel/src/mm/tlb/tlb_coordinator.c ../benchmark_stubs.c mock_panic.c ../../lib/msg/wire.c)
target_include_directories(test_tlb_stress PRIVATE ../../kernel/include ../../services/monitor/generated ../../subsys/include)
target_link_libraries(test_tlb_stress PRIVATE c pthread)
add_test(NAME test_tlb_stress COMMAND test_tlb_stress)
