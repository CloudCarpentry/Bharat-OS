cmake_minimum_required(VERSION 3.20)
project(BharatOS_LibString C)

add_library(bharat_libstring STATIC string.c)
target_include_directories(bharat_libstring PUBLIC ../include)
target_link_libraries(bharat_libstring PUBLIC bharat_freestanding)
bharat_configure_userspace_library(bharat_libstring)

add_library(string ALIAS bharat_libstring)
