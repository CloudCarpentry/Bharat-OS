file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dummy_can.c "")
add_library(bharatos_stack_can STATIC ${CMAKE_CURRENT_BINARY_DIR}/dummy_can.c)
