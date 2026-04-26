if(BHARAT_ENABLE_SUBSYS_WINDOWS)
    add_library(bharatos_personality_windows STATIC win_compat.c)
    target_include_directories(bharatos_personality_windows PUBLIC
        ${CMAKE_SOURCE_DIR}/core/personalities/runtime/include
        ${CMAKE_SOURCE_DIR}/core/kernel/include
        ${CMAKE_SOURCE_DIR}/core/lib/include
        ${CMAKE_SOURCE_DIR}/core/services/core/subsysmgr/include
        ${CMAKE_SOURCE_DIR}/interface/include
    )
    target_link_libraries(bharatos_personality_windows PUBLIC bharat_user_buildopts bharat_freestanding)
endif()
