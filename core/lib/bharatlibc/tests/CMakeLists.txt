# tests/CMakeLists.txt

# Register Python script tests if python is available and meta-tests are enabled
if(BHARATLIBC_ENABLE_META_TESTS)
    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        add_test(
            NAME test_public_header_self_containment
            COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/../tools/check_public_headers.py" --build-dir "${CMAKE_CURRENT_BINARY_DIR}/.."
        )
        set_tests_properties(test_public_header_self_containment PROPERTIES
            TIMEOUT 60
            LABELS "architecture;static"
        )

        add_test(
            NAME test_no_private_kernel_headers
            COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/../tools/check_elf_isolation.py"
        )
        set_tests_properties(test_no_private_kernel_headers PROPERTIES
            TIMEOUT 30
            LABELS "architecture;static"
        )

        add_test(
            NAME test_standalone_relocation
            COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/relocation/test_standalone_relocation.py"
        )
        set_tests_properties(test_standalone_relocation PROPERTIES
            TIMEOUT 180
            LABELS "integration;relocation;slow"
        )
    endif()
endif()

# C logic tests helper macro
macro(add_bh_test name src)
    add_executable(${name} ${src})
    target_link_libraries(${name} PRIVATE
        BharatLibC::Core
        BharatLibC::BSys
        BharatLibC::Alloc
        BharatLibC::StdioMin
    )
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES
        TIMEOUT 10
        LABELS "unit;fast"
    )
endmacro()

add_bh_test(test_string_memory host/test_string_memory.c)
add_bh_test(test_errno_mapping host/test_errno_mapping.c)
add_bh_test(test_fixed_arena host/test_fixed_arena.c)
add_bh_test(test_fixed_arena_exhaustion host/test_fixed_arena_exhaustion.c)
add_bh_test(test_bounded_freelist host/test_bounded_freelist.c)
add_bh_test(test_bsys_backend host/test_bsys_backend.c)
add_bh_test(test_bsys_unsupported host/test_bsys_unsupported.c)
add_bh_test(test_runtime_capability_match host/test_runtime_capability_match.c)
add_bh_test(test_runtime_capability_rejection host/test_runtime_capability_rejection.c)
add_bh_test(test_build_info host/test_build_info.c)
