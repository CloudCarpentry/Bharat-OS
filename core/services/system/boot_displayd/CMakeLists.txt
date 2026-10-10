cmake_minimum_required(VERSION 3.20)
project(boot_displayd C)

add_executable(boot_displayd
    main.c
    ${CMAKE_SOURCE_DIR}/core/stacks/ui/lcd/tiny_ui.c
)
target_link_libraries(boot_displayd PRIVATE bharat_user_buildopts bharat_freestanding)

target_include_directories(boot_displayd PRIVATE
    ${CMAKE_SOURCE_DIR}/interface/include
    ${CMAKE_SOURCE_DIR}/core/kernel/include
    ${CMAKE_SOURCE_DIR}/core/lib/ipc/include
    ${CMAKE_SOURCE_DIR}/core/lib/cap/include
    ${CMAKE_SOURCE_DIR}/core/lib/runtime/include
)

if(BHARAT_UI_LVGL)
    target_compile_definitions(boot_displayd PRIVATE BHARAT_UI_LVGL)
    target_link_libraries(boot_displayd PRIVATE bharat_ui_lvgl_core)
endif()

# Use PIC to fix linker issues since the environment defaults to PIE
target_compile_options(boot_displayd PRIVATE -fPIC)
target_link_options(boot_displayd PRIVATE "LINKER:-no-pie" "-fno-pie")

# In real scenarios we would link against the capability and IPC libraries
target_link_libraries(boot_displayd PRIVATE subsys_manager bharat_libipc bharat_libcap bharat_libruntime bharat_libnamesvc bharat_syscall)

# Add display_client from gui_showcase (stub for broker connection)
target_sources(boot_displayd PRIVATE display_client.c)
target_include_directories(boot_displayd PRIVATE ${CMAKE_SOURCE_DIR}/core/lib/namesvc/include)

bharat_configure_userspace_binary(boot_displayd)

# Host tests must use the host C library, not a bare-metal cross toolchain.
if(NOT CMAKE_CROSSCOMPILING)
    add_executable(test_boot_displayd
        tests/test_boot_displayd.c
        main.c
    )

    target_compile_definitions(test_boot_displayd PRIVATE BHARAT_TESTING)
    target_include_directories(test_boot_displayd PRIVATE
        ${CMAKE_SOURCE_DIR}/interface/include
        ${CMAKE_SOURCE_DIR}/core/kernel/include
        ${CMAKE_SOURCE_DIR}/core/lib/ipc/include
        ${CMAKE_SOURCE_DIR}/core/lib/cap/include
        ${CMAKE_SOURCE_DIR}/core/lib/runtime/include
    )
    add_test(NAME test_boot_displayd COMMAND test_boot_displayd)
endif()
