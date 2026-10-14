cmake_minimum_required(VERSION 3.16)
project(NameSvcTests)

# Get the path to the root of the repo
set(REPO_ROOT "${CMAKE_SOURCE_DIR}")

add_executable(test_namesvc_registry_contract
    ${REPO_ROOT}/quality/tests/host/ipc_contracts/test_namesvc_registry_contract.c
    ${REPO_ROOT}/core/kernel/src/ipc/contract_validate.c
    ${REPO_ROOT}/core/services/namesvc/src/registry.c
    ${REPO_ROOT}/core/services/namesvc/src/ipc_dispatch.c
    ${REPO_ROOT}/core/services/namesvc/src/service_manifest.c
)
target_include_directories(test_namesvc_registry_contract PRIVATE
    ${REPO_ROOT}/core/kernel/include
    ${REPO_ROOT}/interface/include
    ${REPO_ROOT}/core/services/namesvc/include
    ${REPO_ROOT}/core/services/namesvc/src
    ${REPO_ROOT}/core/lib/ipc/include
    ${REPO_ROOT}/core/lib/cap/include
    ${REPO_ROOT}/core/lib/runtime/include
)
add_test(NAME test_namesvc_registry_contract COMMAND test_namesvc_registry_contract)

add_executable(test_namesvc_ipc_caps
    ${REPO_ROOT}/quality/tests/host/ipc_contracts/test_namesvc_ipc_caps.c
    ${REPO_ROOT}/core/kernel/src/ipc/contract_validate.c
    ${REPO_ROOT}/core/services/namesvc/src/registry.c
    ${REPO_ROOT}/core/services/namesvc/src/ipc_dispatch.c
    ${REPO_ROOT}/core/services/namesvc/src/ipc_auth.c
    ${REPO_ROOT}/core/lib/cap/src/cap_validate.c
)
target_include_directories(test_namesvc_ipc_caps PRIVATE
    ${REPO_ROOT}/core/kernel/include
    ${REPO_ROOT}/interface/include
    ${REPO_ROOT}/core/services/namesvc/include
    ${REPO_ROOT}/core/services/namesvc/src
    ${REPO_ROOT}/core/lib/ipc/include
    ${REPO_ROOT}/core/lib/cap/include
)
add_test(NAME test_namesvc_ipc_caps COMMAND test_namesvc_ipc_caps)
