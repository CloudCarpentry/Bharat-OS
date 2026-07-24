add_library(subsys_manager STATIC
    manager.c
    subsys_test_runner.c
    ../../../stacks/network/skb.c
    ../../../personalities/runtime/src/version_registry.c
)
target_include_directories(subsys_manager PUBLIC
    ${CMAKE_SOURCE_DIR}/core/services/core/subsysmgr/include
    ${CMAKE_SOURCE_DIR}/core/personalities/runtime/include
    ${CMAKE_SOURCE_DIR}/core/kernel/include
    ${CMAKE_SOURCE_DIR}/core/lib/include
    ${CMAKE_SOURCE_DIR}/core/lib/packet/include
    ${CMAKE_SOURCE_DIR}/core/hal/include
    ${CMAKE_SOURCE_DIR}/interface/include
    ${CMAKE_SOURCE_DIR}/core/stacks/network/include
)
target_link_libraries(subsys_manager PUBLIC
    bharat_freestanding
    bharat_generated_includes
    bharat_libpacket
)
