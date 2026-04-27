add_library(bharatos_stack_vehicle STATIC
    sim/bh_vehicle_sim.c
)

target_include_directories(bharatos_stack_vehicle PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(bharatos_stack_vehicle PRIVATE
    ${CMAKE_SOURCE_DIR}/core/kernel/include
    ${CMAKE_SOURCE_DIR}/interface/include
)

target_link_libraries(bharatos_stack_vehicle PRIVATE bharat_kernel_buildopts bharat_freestanding)
