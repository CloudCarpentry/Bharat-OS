cmake_minimum_required(VERSION 3.20)
project(BharatDeviceManagerTests C)

enable_testing()

get_filename_component(
    BHARAT_REPO_ROOT
    "${CMAKE_CURRENT_LIST_DIR}/../../../.."
    ABSOLUTE
)

add_executable(test_devmgr_msg
    test_devmgr_msg.c
    ../devmgr_core.c
)

target_include_directories(test_devmgr_msg PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/.."
    "${BHARAT_REPO_ROOT}/interface/include"
    "${BHARAT_REPO_ROOT}/interface"
    "${BHARAT_REPO_ROOT}/interface/uapi"
    "${BHARAT_REPO_ROOT}/interface/sdk/include"
    "${BHARAT_REPO_ROOT}/core/services/common/runtime/include"
    "${BHARAT_REPO_ROOT}/core/lib/ipc/include"
    "${BHARAT_REPO_ROOT}/core/lib/cap/include"
    "${BHARAT_REPO_ROOT}/core/lib/runtime/include"
)

target_compile_features(test_devmgr_msg PRIVATE c_std_11)

add_test(NAME devmgr_msg COMMAND test_devmgr_msg)