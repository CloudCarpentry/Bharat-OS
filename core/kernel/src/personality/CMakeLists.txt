add_library(bharatos_kernel_personality OBJECT
    native/native_syscall_handlers.c
    personality_registry_compat.c
)

target_include_directories(bharatos_kernel_personality PRIVATE ../include ${BHARAT_GENERATED_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/core/personalities/runtime/include ${CMAKE_SOURCE_DIR}/interface/include ${CMAKE_SOURCE_DIR}/core/lib/include ${BHARAT_BOOT_INCLUDE_DIR})
target_link_libraries(bharatos_kernel_personality PRIVATE bharat_kernel_includes bharat_kernel_buildopts)
