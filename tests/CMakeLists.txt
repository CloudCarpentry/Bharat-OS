cmake_minimum_required(VERSION 3.20)
project(BharatOS-Tests C)

enable_testing()

add_executable(test_early_alloc test_early_alloc.c)
target_include_directories(test_early_alloc PRIVATE ../kernel/include)

add_test(NAME test_early_alloc COMMAND test_early_alloc)
