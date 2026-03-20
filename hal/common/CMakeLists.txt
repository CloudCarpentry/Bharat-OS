add_library(hal_common OBJECT
    ../../kernel/src/hal/common/fdt_parser.c
    ../../kernel/src/hal/common/discovery.c
    ../../kernel/src/hal/mmu_init.c
    ../../kernel/src/hal/iommu_stub.c
    ../../kernel/src/hal/interrupt_common.c
    ../../kernel/src/hal/timer_common.c
)

target_include_directories(hal_common PRIVATE
    ${CMAKE_SOURCE_DIR}/kernel/src
    ${CMAKE_SOURCE_DIR}/kernel/include
    ${CMAKE_SOURCE_DIR}/kernel/include/generated
    ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(hal_common PRIVATE bharat_freestanding bharat_kernel_buildopts)
