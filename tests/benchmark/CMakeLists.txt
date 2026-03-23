cmake_minimum_required(VERSION 3.20)
project(BharatOS-Benchmark-Framework C)

# Benchmark core library
add_library(bharat_bench_core STATIC
    core/bench_runner.c
    core/bench_clock.c
    core/bench_registry.c
)
target_include_directories(bharat_bench_core PUBLIC include)

# Host runner executable linking all suites
add_executable(bench_runner
    core/bench_main.c
    suites/memory/bench_pmm.c
    suites/memory/bench_vmm.c
    suites/vfs/bench_vfs.c
    suites/scheduler/bench_sched.c
    suites/network/bench_net.c
)

target_link_libraries(bench_runner PRIVATE bharat_bench_core)

# If compiling on Linux/Unix, might need to link rt for clock_gettime
if(UNIX AND NOT APPLE)
    target_link_libraries(bench_runner PRIVATE rt)
endif()

# We need the benchmarks section to be properly preserved.
# Depending on compiler/linker, we may need specific flags.
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    if(APPLE)
        # MacOS / Mach-O doesn't easily drop sections but ensures __DATA,benchmarks
        target_compile_options(bench_runner PRIVATE -O2)
    else()
        # Linux / ELF
        target_link_options(bench_runner PRIVATE "-Wl,--no-as-needed")
        # To prevent GC of the benchmarks section if using --gc-sections
        # Though the variables are statically defined and we use &__start_benchmarks
    endif()
endif()

add_test(NAME run_all_benchmarks COMMAND bench_runner)
