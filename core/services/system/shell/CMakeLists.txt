cmake_minimum_required(VERSION 3.20)
project(BharatOS_System_Shell C)

option(CONFIG_SHELL_MINI "Enable mini shell profile" OFF)
option(CONFIG_SHELL_ADMIN "Enable admin shell profile" ON)
option(CONFIG_SHELL_REMOTE "Enable remote shell endpoint" OFF)
option(CONFIG_SHELL_RECOVERY "Enable recovery shell mode" ON)
option(CONFIG_SHELL_SCRIPT "Enable shell scripting mode" OFF)
option(CONFIG_SHELL_COMPAT_POSIX "Enable compatibility POSIX shell (deferred)" OFF)

set(SHELL_SOURCES
    src/shell_service.c
    src/shell_session.c
    src/shell_parser.c
    src/shell_dispatch.c
    src/shell_auth.c
    src/shell_output.c
    src/shell_backend_urpc.c
    ../../../lib/urpc/urpc.c
    src/commands/shell_commands.c
)

add_executable(system_shell ${SHELL_SOURCES})
target_link_libraries(system_shell PRIVATE
    bharat_user_buildopts
    bharat_freestanding
)
target_include_directories(system_shell PRIVATE
    include
    ../../../lib
    ../../../lib/runtime/include
    ${CMAKE_SOURCE_DIR}/interface
)

if(CONFIG_SHELL_MINI)
    target_compile_definitions(system_shell PRIVATE CONFIG_SHELL_MINI=1)
endif()
if(CONFIG_SHELL_ADMIN)
    target_compile_definitions(system_shell PRIVATE CONFIG_SHELL_ADMIN=1)
endif()
if(CONFIG_SHELL_REMOTE)
    target_compile_definitions(system_shell PRIVATE CONFIG_SHELL_REMOTE=1)
endif()
if(CONFIG_SHELL_RECOVERY)
    target_compile_definitions(system_shell PRIVATE CONFIG_SHELL_RECOVERY=1)
endif()
if(CONFIG_SHELL_SCRIPT)
    target_compile_definitions(system_shell PRIVATE CONFIG_SHELL_SCRIPT=1)
endif()
if(CONFIG_SHELL_COMPAT_POSIX)
    target_compile_definitions(system_shell PRIVATE CONFIG_SHELL_COMPAT_POSIX=1)
endif()


if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

bharat_configure_userspace_binary(system_shell)
