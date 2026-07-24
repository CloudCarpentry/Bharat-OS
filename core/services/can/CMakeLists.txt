cmake_minimum_required(VERSION 3.20)

if (BHARAT_AUTOMOTIVE_PROFILE OR BHARAT_ENABLE_CAN)
    add_executable(canmgr_service
        canmgr/main.c
        canmgr/subscriptions.c
    )
    target_link_libraries(canmgr_service PRIVATE
        bharat_libruntime
        bharat_libipc
        bharat_libcap
        bharatos_stack_can
    )
    bharat_configure_userspace_binary(canmgr_service)
    target_include_directories(canmgr_service PRIVATE ${CMAKE_SOURCE_DIR}/core/services/include)

    add_executable(cangw_service
        cangw/gateway.c
    )
    target_link_libraries(cangw_service PRIVATE
        bharat_libruntime
        bharat_libipc
        bharat_libcap
        bharatos_stack_can
    )
    bharat_configure_userspace_binary(cangw_service)
    target_include_directories(cangw_service PRIVATE ${CMAKE_SOURCE_DIR}/core/services/include)

    add_executable(candiag_service
        candiag/diag_service.c
    )
    target_link_libraries(candiag_service PRIVATE
        bharat_libruntime
        bharat_libipc
        bharat_libcap
        bharatos_stack_can
    )
    bharat_configure_userspace_binary(candiag_service)
    target_include_directories(candiag_service PRIVATE ${CMAKE_SOURCE_DIR}/core/services/include)
endif()
