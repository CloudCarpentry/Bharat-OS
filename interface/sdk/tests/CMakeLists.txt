add_executable(bharat_sdk_core_test core_test.c)
target_link_libraries(bharat_sdk_core_test PRIVATE Bharat::sdk)
add_test(NAME bharat_sdk_core COMMAND bharat_sdk_core_test)
add_executable(bharat_sdk_hmem_tensor_test hmem_tensor_test.c)
target_link_libraries(bharat_sdk_hmem_tensor_test PRIVATE Bharat::sdk)
add_test(NAME bharat_sdk_hmem_tensor COMMAND bharat_sdk_hmem_tensor_test)
add_test(NAME bharat_sdk_public_headers COMMAND ${CMAKE_COMMAND}
    -DSDK_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/.. -DBUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}/header-check
    -DC_COMPILER=${CMAKE_C_COMPILER} -P ${CMAKE_CURRENT_SOURCE_DIR}/check_public_headers.cmake)
