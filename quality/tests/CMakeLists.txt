cmake_minimum_required(VERSION 3.20)
project(BharatOS-Tests C)

enable_testing()

add_subdirectory(host)

# Return early to skip legacy broken targets in this file during path migration
return()
