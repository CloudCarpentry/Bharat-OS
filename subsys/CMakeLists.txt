cmake_minimum_required(VERSION 3.20)
project(BharatOS_Subsys C ASM)

include_directories(include)

# Subsystem manager daemon/library
set(SUBSYS_SOURCES
    src/manager.c
    src/linux_compat.c
    src/win_compat.c
)

# AI Governor daemon (Predictive Resource Scheduling)
set(AI_GOVERNOR_SOURCES
    src/ai_governor.c
)

add_library(subsys_manager STATIC ${SUBSYS_SOURCES})
target_compile_options(subsys_manager PRIVATE -ffreestanding -nostdlib -Wall)

add_executable(ai_governor ${AI_GOVERNOR_SOURCES})
# Might need host headers if it's considered user-space mock,
# for now compiled natively or with suitable toolchain for user-space
