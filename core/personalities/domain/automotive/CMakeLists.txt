add_library(bharatos_personality_automotive OBJECT
    automotive.c
    auto_watchdog.c
    auto_safe_state.c
    autos_task_demo.c
)

target_include_directories(bharatos_personality_automotive PRIVATE
    ${BHARAT_KERNEL_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/core/kernel/include
    ${CMAKE_SOURCE_DIR}/core/kernel/include/trace
    ${BHARAT_GENERATED_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/interface/include
    ${CMAKE_SOURCE_DIR}/core/stacks/vehicle/include
    ${CMAKE_SOURCE_DIR}/core/services/core/subsysmgr/include
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(bharatos_personality_automotive PRIVATE bharat_kernel_buildopts)
