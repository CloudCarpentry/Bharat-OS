add_library(subsys_manager STATIC
    manager.c
    subsys_test_runner.c
    ../../../stacks/network/skb.c
    ../../../personalities/common/version_registry.c
)
target_include_directories(subsys_manager PUBLIC
    ${CMAKE_SOURCE_DIR}/services/core/subsysmgr/include
    ${CMAKE_SOURCE_DIR}/kernel/include
    ${CMAKE_SOURCE_DIR}/lib/packet/include
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/stacks/network/include
)
target_link_libraries(subsys_manager PUBLIC
    bharat_freestanding
    bharat_generated_includes
    bharat_libpacket
)
