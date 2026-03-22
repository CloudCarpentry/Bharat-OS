add_library(k_ipc OBJECT
    ../../src/ipc/endpoint_ipc.c
    ../../src/ipc/async_ipc.c
    ../../src/ipc/ipc_timeout.c
    ../../src/ipc/multikernel.c
    ../../src/ipc/mk_dispatch.c
    ../../src/ipc/mk_proto.c
    ../../src/urpc/urpc_bootstrap.c
    ../../src/urpc/urpc_channel.c
)

target_include_directories(k_ipc PRIVATE ../../include ../../include/generated ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/services/core/subsysmgr/include)
target_link_libraries(k_ipc PRIVATE bharat_freestanding bharat_kernel_buildopts)
