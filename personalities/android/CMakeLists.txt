if(BHARAT_ENABLE_SUBSYS_ANDROID)
    add_library(bharatos_personality_android STATIC
        android_personality.c
        android_process.c
        android_boot.c
        android_security.c
        android_binder_dist.c
    )
    target_include_directories(bharatos_personality_android PUBLIC ${CMAKE_SOURCE_DIR}/kernel/include ${CMAKE_SOURCE_DIR}/lib/include ${CMAKE_SOURCE_DIR}/services/core/subsysmgr/include)
    target_link_libraries(bharatos_personality_android PUBLIC bharat_user_buildopts bharat_freestanding)
endif()
