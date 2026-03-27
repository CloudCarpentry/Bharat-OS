cmake_minimum_required(VERSION 3.20)
if(TARGET kernel.elf)
    target_sources(kernel.elf PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/mon_vm_service.c
        ${CMAKE_CURRENT_SOURCE_DIR}/mon_vm_dispatch.c
        ${CMAKE_CURRENT_SOURCE_DIR}/mon_vm_state.c
    )
endif()
