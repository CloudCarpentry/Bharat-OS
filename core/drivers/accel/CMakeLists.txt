add_library(bharatos_driver_accel OBJECT virt_accel.c)
target_include_directories(bharatos_driver_accel PRIVATE ../../include ../../kernel/include)
target_link_libraries(bharatos_driver_accel PRIVATE bharat_kernel_includes bharat_freestanding)
