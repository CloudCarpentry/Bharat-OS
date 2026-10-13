cmake_minimum_required(VERSION 3.20)

project(faultmgr C ASM)

# Add source files
set(SOURCES main.c state.c)

# Create executable
add_executable(faultmgr ${SOURCES})
    target_link_libraries(faultmgr PRIVATE bharat_user_buildopts bharat_freestanding)

# Target include directories
target_include_directories(faultmgr PRIVATE
    ${CMAKE_SOURCE_DIR}/interface/include
    ${CMAKE_SOURCE_DIR}/core/services/include
)

# Link against common runtime library and IPC library
target_link_libraries(faultmgr PRIVATE
    bharat_service_runtime
    bharat_libipc
    bharat_libruntime
    bharat_libnamesvc
)

bharat_configure_userspace_binary(faultmgr)

if(NOT CMAKE_CROSSCOMPILING)
    add_executable(test_faultmgr tests/test_faultmgr.c state.c)
    target_include_directories(test_faultmgr PRIVATE
        ${CMAKE_SOURCE_DIR}/interface/include
    )
    add_test(NAME test_faultmgr COMMAND test_faultmgr)

    add_executable(test_faultmgr_msg tests/test_faultmgr_msg.c main.c state.c)
    target_include_directories(test_faultmgr_msg PRIVATE
        ${CMAKE_SOURCE_DIR}/interface/include
        ${CMAKE_SOURCE_DIR}/core/services/include
    )
    target_compile_definitions(test_faultmgr_msg PRIVATE FAULTMGR_UNIT_TEST)
    target_link_libraries(test_faultmgr_msg PRIVATE
        bharat_service_runtime
        bharat_libipc
        bharat_libruntime
        bharat_libnamesvc
    )
    add_test(NAME test_faultmgr_msg COMMAND test_faultmgr_msg)
endif()
