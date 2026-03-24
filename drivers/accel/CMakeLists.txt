add_library(bharatos_driver_accel OBJECT virt_accel.c)
target_include_directories(bharatos_driver_accel PRIVATE ../../include ../../kernel/include)
target_link_libraries(bharatos_driver_accel PRIVATE bharat_kernel_includes)
target_compile_options(bharatos_driver_accel PRIVATE $<$<COMPILE_LANGUAGE:C>:-ffreestanding> $<$<COMPILE_LANGUAGE:C>:-fno-builtin>)
