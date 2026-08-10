cmake_minimum_required(VERSION 3.20)
project(BharatOS_Apps C ASM)

# Leaf application targets
add_subdirectory(shell)
add_subdirectory(diag)
add_subdirectory(demo)
add_subdirectory(test)
add_subdirectory(ai_governor)
if(BHARAT_ENABLE_UI AND BHARAT_UI_LVGL)
    add_subdirectory(../../apps/gui_showcase gui_showcase_build)
endif()
add_subdirectory(user_smoke)
