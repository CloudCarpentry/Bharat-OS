cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED REPO_ROOT)
    message(FATAL_ERROR "REPO_ROOT is required")
endif()

if(NOT DEFINED WORK_DIR)
    set(WORK_DIR "${CMAKE_CURRENT_BINARY_DIR}/policy_matrix")
endif()

file(MAKE_DIRECTORY "${WORK_DIR}")

function(run_policy_case case_name expected_result)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs CMAKE_ARGS)
    cmake_parse_arguments(RPC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(case_build_dir "${WORK_DIR}/${case_name}")
    file(REMOVE_RECURSE "${case_build_dir}")

    execute_process(
        COMMAND "${CMAKE_COMMAND}"
            -S "${REPO_ROOT}"
            -B "${case_build_dir}"
            -DBHARAT_BUILD_HOST_TESTS=OFF
            ${RPC_CMAKE_ARGS}
        RESULT_VARIABLE case_result
        OUTPUT_VARIABLE case_stdout
        ERROR_VARIABLE case_stderr
    )

    if(expected_result STREQUAL "PASS")
        if(NOT case_result EQUAL 0)
            message(FATAL_ERROR
                "Case '${case_name}' expected PASS but failed.\n"
                "stdout:\n${case_stdout}\n"
                "stderr:\n${case_stderr}\n"
            )
        endif()
    elseif(expected_result STREQUAL "FAIL")
        if(case_result EQUAL 0)
            message(FATAL_ERROR "Case '${case_name}' expected FAIL but configure succeeded")
        endif()
    else()
        message(FATAL_ERROR "Unknown expected_result='${expected_result}'")
    endif()
endfunction()

run_policy_case(lowercase_profile_personality_normalized PASS
    CMAKE_ARGS
        -DBHARAT_DEVICE_PROFILE=automotive_ecu
        -DBHARAT_PERSONALITY_PROFILE=linux
        -DBHARAT_TARGET_BOARD=qemu-virt-riscv64
)

run_policy_case(automotive_infotainment_forces_required_network_service PASS
    CMAKE_ARGS
        -DBHARAT_DEVICE_PROFILE=AUTOMOTIVE_INFOTAINMENT
        -DBHARAT_PERSONALITY_PROFILE=NONE
        -DBHARAT_ENABLE_SERVICE_NETWORK=OFF
)

run_policy_case(invalid_personality_rejected FAIL
    CMAKE_ARGS
        -DBHARAT_DEVICE_PROFILE=DESKTOP
        -DBHARAT_PERSONALITY_PROFILE=legacy
)
