file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dummy_storage.c "")
add_library(bharatos_stack_storage STATIC ${CMAKE_CURRENT_BINARY_DIR}/dummy_storage.c)
