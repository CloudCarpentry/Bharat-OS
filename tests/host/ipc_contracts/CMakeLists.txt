cmake_minimum_required(VERSION 3.16)
project(IPCContractsTests)

# Get the path to the root of the repo
set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

add_executable(test_ipc_contract_validation
    test_ipc_contract_validation.c
    ${REPO_ROOT}/kernel/src/ipc/contract_validate.c
)
target_include_directories(test_ipc_contract_validation PRIVATE
    ${REPO_ROOT}/kernel/include
    ${REPO_ROOT}/include
)

add_executable(test_netmgr_ipc_contract
    test_netmgr_ipc_contract.c
    ${REPO_ROOT}/kernel/src/ipc/contract_validate.c
    ${REPO_ROOT}/services/netmgr/src/ipc_dispatch.c
    ${REPO_ROOT}/services/netmgr/src/ipc_ops.c
    ${REPO_ROOT}/services/netmgr/src/ipc_auth.c
    ${REPO_ROOT}/services/netmgr/src/ipc_contract.c
    ${REPO_ROOT}/services/netmgr/src/service_manifest.c
)
target_include_directories(test_netmgr_ipc_contract PRIVATE
    ${REPO_ROOT}/kernel/include
    ${REPO_ROOT}/include
    ${REPO_ROOT}/services/netmgr/include
    ${REPO_ROOT}/lib/ipc/include
    ${REPO_ROOT}/lib/net/include
    ${REPO_ROOT}/lib/runtime/include
    ${REPO_ROOT}/services/core/subsysmgr/include
    ${REPO_ROOT}/services/netmgr/src
    ${REPO_ROOT}/lib/cap/include
)

add_executable(test_namesvc_registry_contract
    test_namesvc_registry_contract.c
    ${REPO_ROOT}/kernel/src/ipc/contract_validate.c
    ${REPO_ROOT}/services/namesvc/src/registry.c
    ${REPO_ROOT}/services/namesvc/src/ipc_dispatch.c
    ${REPO_ROOT}/services/namesvc/src/service_manifest.c
)
target_include_directories(test_namesvc_registry_contract PRIVATE
    ${REPO_ROOT}/kernel/include
    ${REPO_ROOT}/include
    ${REPO_ROOT}/services/namesvc/include
    ${REPO_ROOT}/services/namesvc/src
    ${REPO_ROOT}/lib/ipc/include
    ${REPO_ROOT}/lib/cap/include
    ${REPO_ROOT}/lib/runtime/include
)