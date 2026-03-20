add_subdirectory(common)

if(BHARAT_ARCH_FAMILY STREQUAL "x86_64")
    add_subdirectory(x86_64)
elseif(BHARAT_ARCH_FAMILY STREQUAL "arm64")
    add_subdirectory(arm64)
elseif(BHARAT_ARCH_FAMILY MATCHES "^riscv")
    add_subdirectory(riscv64)
endif()

add_subdirectory(platforms/qemu)
