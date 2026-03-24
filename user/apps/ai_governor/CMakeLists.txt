option(BHARAT_BUILD_AI_GOVERNOR "Build ai_governor user-space demo executable" ON)
if(BHARAT_BUILD_AI_GOVERNOR)
    add_executable(ai_governor
        ai_governor.c
        ${CMAKE_SOURCE_DIR}/kernel/src/sched/ai_sched.c
        ${CMAKE_SOURCE_DIR}/kernel/src/sched/sched_deg.c
        ${CMAKE_SOURCE_DIR}/kernel/src/sched_stub.c
    )
    target_link_libraries(ai_governor PRIVATE
        subsys_manager
        bharat_libtransport
        bharat_libcmin
        bharat_user_buildopts
        bharat_freestanding
    )
    target_include_directories(ai_governor PRIVATE
        ${CMAKE_SOURCE_DIR}/kernel/include
        ${CMAKE_SOURCE_DIR}/services/core/subsysmgr/include
    )
    bharat_configure_userspace_binary(ai_governor)
endif()
