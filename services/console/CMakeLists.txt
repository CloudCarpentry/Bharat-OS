cmake_minimum_required(VERSION 3.20)
project(BharatOS_Console_Service C ASM)

add_executable(console main.c)
    target_link_libraries(console PRIVATE bharat_user_buildopts bharat_freestanding)

target_include_directories(console PRIVATE
    ../../kernel/include
    ../../subsys/include
    ../../lib/urpc
)



# Link against the subsystem manager and URPC libraries
target_link_libraries(console PRIVATE urpc subsys_manager)
