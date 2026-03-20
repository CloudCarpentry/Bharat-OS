add_executable(test_early_alloc ../test_early_alloc.c)
add_executable(test_payload_codec ../test_payload_codec.c)
add_executable(test_capwire ../test_capwire.c)
add_executable(test_ool ../test_ool.c)

# We link to the host libraries to enable testing of these core components
target_link_libraries(test_early_alloc PRIVATE bharat_kernel_buildopts)
target_link_libraries(test_payload_codec PRIVATE bharat_libmsg bharat_hosted bharat_host_buildopts)
target_link_libraries(test_capwire PRIVATE bharat_libcap bharat_hosted bharat_host_buildopts)
target_link_libraries(test_ool PRIVATE bharat_libdiag bharat_hosted bharat_host_buildopts)

add_test(NAME test_early_alloc COMMAND test_early_alloc)
add_test(NAME test_payload_codec COMMAND test_payload_codec)
add_test(NAME test_capwire COMMAND test_capwire)
add_test(NAME test_ool COMMAND test_ool)

# Name Service Registry Test
add_executable(test_namesvc_registry ../test_namesvc_registry.c ../../services/namesvc/src/registry.c)
target_include_directories(test_namesvc_registry PRIVATE ../../services/namesvc/include ../../services/namesvc ../../lib/cap/include)
add_test(NAME test_namesvc_registry COMMAND test_namesvc_registry)
