cmake_minimum_required(VERSION 3.20)
project(BharatOS_Network_Subsys C)

# For now, this is purely an interface layer placeholder.
add_custom_target(subsys_network_stub ALL
    COMMAND ${CMAKE_COMMAND} -E echo "Building subsys/network stub"
)
