cmake_minimum_required(VERSION 3.20)
project(BharatOS_Net_Service C ASM)

add_executable(net_service
    main.c
    control_plane.c
    data_plane.c
)

target_include_directories(net_service PRIVATE
    ../../kernel/include
    ../../subsys/include
    ../../lib/bharat_libtransport
)



# Link against subsystem and URPC
target_link_libraries(net_service PRIVATE bharat_libtransport subsys_manager bharat_libcmin bharat_freestanding bharat_libruntime)
