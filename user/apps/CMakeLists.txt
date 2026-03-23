cmake_minimum_required(VERSION 3.20)
project(BharatOS_Apps C ASM)

# Leaf application targets
add_subdirectory(shell)
add_subdirectory(diag)
add_subdirectory(demo)
add_subdirectory(test)
add_subdirectory(ai_governor)
