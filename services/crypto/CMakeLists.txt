add_executable(crypto
    main.c
    protocol.c
    dispatch.c
    key_store.c
    drbg.c
)
target_include_directories(crypto PRIVATE ../../kernel/include ../../subsys/include ../../lib/bharat_libtransport)

target_link_libraries(crypto PRIVATE bharat_libtransport subsys_manager bharat_libruntime)

bharat_configure_userspace_binary(crypto)
