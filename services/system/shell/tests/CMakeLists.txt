set(SHELL_TEST_SRCS
    ../src/shell_service.c
    ../src/shell_session.c
    ../src/shell_parser.c
    ../src/shell_dispatch.c
    ../src/shell_auth.c
    ../src/shell_output.c
    ../src/shell_backend_stub.c
    ../src/commands/shell_commands.c
)

function(add_shell_test name source)
    add_executable(${name} ${source} ${SHELL_TEST_SRCS})
    target_include_directories(${name} PRIVATE ../include)
    target_compile_definitions(${name} PRIVATE SHELL_NO_MAIN=1)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_shell_test(test_shell_parser test_shell_parser.c)
add_shell_test(test_shell_registry test_shell_registry.c)
add_shell_test(test_shell_auth_and_denials test_shell_auth_and_denials.c)
add_shell_test(test_shell_malformed_and_unknown test_shell_malformed_and_unknown.c)
add_shell_test(test_shell_timeout_and_backend_failure test_shell_timeout_and_backend_failure.c)
add_shell_test(test_shell_integration_session test_shell_integration_session.c)
