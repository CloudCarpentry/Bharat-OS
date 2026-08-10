cmake_minimum_required(VERSION 3.20)

set(FABRIC_TEST_INCLUDES
    ${CMAKE_SOURCE_DIR}/core/kernel/include
    ${CMAKE_SOURCE_DIR}/interface/include
    ${CMAKE_SOURCE_DIR}/core/lib/include
    ${CMAKE_SOURCE_DIR}/quality/tests/host/ipc/fabric
)

set(FABRIC_SRCS
    ${CMAKE_SOURCE_DIR}/core/kernel/src/ipc/fabric/mk_mpsc_ring.c
    ${CMAKE_SOURCE_DIR}/core/kernel/src/ipc/fabric/mk_endpoint.c
    ${CMAKE_SOURCE_DIR}/core/kernel/src/ipc/fabric/mk_transaction.c
    ${CMAKE_SOURCE_DIR}/core/kernel/src/ipc/fabric/mk_replay.c
    ${CMAKE_SOURCE_DIR}/core/kernel/src/ipc/fabric/mk_fabric.c
    fabric/fake_hal.c
)

add_executable(host_test_mk_wire fabric/test_mk_wire.c ${FABRIC_SRCS})
target_include_directories(host_test_mk_wire PRIVATE ${FABRIC_TEST_INCLUDES})
add_test(NAME host_test_mk_wire COMMAND host_test_mk_wire)

add_executable(host_test_test_mk_mpsc_ring fabric/test_mk_mpsc_ring.c ${FABRIC_SRCS})
target_include_directories(host_test_test_mk_mpsc_ring PRIVATE ${FABRIC_TEST_INCLUDES})
add_test(NAME host_test_test_mk_mpsc_ring COMMAND host_test_test_mk_mpsc_ring)

add_executable(host_test_test_mk_credits fabric/test_mk_credits.c ${FABRIC_SRCS})
target_include_directories(host_test_test_mk_credits PRIVATE ${FABRIC_TEST_INCLUDES})
add_test(NAME host_test_test_mk_credits COMMAND host_test_test_mk_credits)

add_executable(host_test_test_mk_transactions fabric/test_mk_transactions.c ${FABRIC_SRCS})
target_include_directories(host_test_test_mk_transactions PRIVATE ${FABRIC_TEST_INCLUDES})
add_test(NAME host_test_test_mk_transactions COMMAND host_test_test_mk_transactions)

add_executable(host_test_test_mk_replay fabric/test_mk_replay.c ${FABRIC_SRCS})
target_include_directories(host_test_test_mk_replay PRIVATE ${FABRIC_TEST_INCLUDES})
add_test(NAME host_test_test_mk_replay COMMAND host_test_test_mk_replay)

add_executable(host_test_test_mk_endpoints fabric/test_mk_endpoints.c ${FABRIC_SRCS})
target_include_directories(host_test_test_mk_endpoints PRIVATE ${FABRIC_TEST_INCLUDES})
add_test(NAME host_test_test_mk_endpoints COMMAND host_test_test_mk_endpoints)

add_executable(host_test_test_mk_dispatch fabric/test_mk_dispatch.c ${FABRIC_SRCS})
target_include_directories(host_test_test_mk_dispatch PRIVATE ${FABRIC_TEST_INCLUDES})
add_test(NAME host_test_test_mk_dispatch COMMAND host_test_test_mk_dispatch)

add_executable(host_test_test_mk_faults fabric/test_mk_faults.c ${FABRIC_SRCS})
target_include_directories(host_test_test_mk_faults PRIVATE ${FABRIC_TEST_INCLUDES})
add_test(NAME host_test_test_mk_faults COMMAND host_test_test_mk_faults)

add_executable(host_test_test_mk_fabric_stress fabric/test_mk_fabric_stress.c ${FABRIC_SRCS})
target_include_directories(host_test_test_mk_fabric_stress PRIVATE ${FABRIC_TEST_INCLUDES})
target_link_libraries(host_test_test_mk_fabric_stress PRIVATE pthread)
add_test(NAME host_test_test_mk_fabric_stress COMMAND host_test_test_mk_fabric_stress)
