add_library(hal_common OBJECT
    ../common/cpu_features.c
    hmem.c
    ../common/memops/mem_scalar.c
    ../common/memops/mem_dispatch.c
)

target_include_directories(hal_common PRIVATE
    ${CMAKE_SOURCE_DIR}/core/hal/include
    ${CMAKE_SOURCE_DIR}/interface/include)
target_link_libraries(hal_common PRIVATE bharat_freestanding)

# Apply targeted builtin suppression to the memory helper
set_source_files_properties(../common/memops/mem_scalar.c PROPERTIES COMPILE_OPTIONS "-fno-builtin-memset;-fno-builtin-memcpy;-fno-builtin-memmove;-fno-builtin-strcmp;-fno-builtin-strncmp;-fno-builtin-strlen;-fno-builtin-bzero")
