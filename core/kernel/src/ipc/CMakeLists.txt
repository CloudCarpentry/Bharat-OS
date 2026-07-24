add_library(k_ipc OBJECT
    ../../src/ipc/endpoint_ipc.c
    ../../src/ipc/async_ipc.c
    ../../src/ipc/ipc_timeout.c
    ../../src/ipc/multikernel.c
    ../../src/ipc/mk_dispatch.c
    ../../src/ipc/mk_proto.c
    ../../src/ipc/contract_validate.c
    ../../src/ipc/bh_core_msgq.c
    ../../src/urpc/urpc_bootstrap.c
    ../../src/urpc/urpc_channel.c
)

target_include_directories(k_ipc PRIVATE ../../include ${BHARAT_GENERATED_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/core/personalities/runtime/include ${CMAKE_SOURCE_DIR}/interface/include ${CMAKE_SOURCE_DIR}/core/lib/include ${BHARAT_BOOT_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/core/services/core/subsysmgr/include)
target_link_libraries(k_ipc PRIVATE bharat_freestanding bharat_kernel_buildopts)
