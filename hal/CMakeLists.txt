add_subdirectory(common)

if(BHARAT_ARCH_FAMILY STREQUAL "X86")
    add_subdirectory(x86_64)
elseif(BHARAT_ARCH_FAMILY STREQUAL "ARM")
    add_subdirectory(arm64)
elseif(BHARAT_ARCH_FAMILY STREQUAL "RISCV")
    add_subdirectory(riscv64)
endif()

add_subdirectory(platforms/qemu)
