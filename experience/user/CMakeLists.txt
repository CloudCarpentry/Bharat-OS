add_subdirectory(apps)
add_subdirectory(sdk)
add_subdirectory(shell)

if(BHARAT_ENABLE_UI)
    add_subdirectory(ui)
endif()
