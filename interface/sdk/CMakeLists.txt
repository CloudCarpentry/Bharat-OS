cmake_minimum_required(VERSION 3.20)
project(BharatSDK VERSION 0.1.0 LANGUAGES C)

include(GNUInstallDirs)
option(BHARAT_SDK_BUILD_EXAMPLES "Build Bharat SDK examples" ON)
option(BHARAT_SDK_BUILD_TESTS "Build Bharat SDK host tests" ON)
option(BHARAT_SDK_BUILD_BENCHMARKS "Build HMEM/Tensor SDK benchmarks" OFF)

add_library(bharat_core STATIC lib/runtime/core.c lib/runtime/host_backend.c)
add_library(Bharat::core ALIAS bharat_core)
set_target_properties(bharat_core PROPERTIES EXPORT_NAME core)
target_compile_features(bharat_core PUBLIC c_std_11)
target_include_directories(bharat_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../uapi>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_include_directories(bharat_core PRIVATE lib/internal)

add_library(bharat_process STATIC lib/process/process.c)
add_library(Bharat::process ALIAS bharat_process)
set_target_properties(bharat_process PROPERTIES EXPORT_NAME process)
add_library(bharat_ipc STATIC lib/ipc/ipc.c)
add_library(Bharat::ipc ALIAS bharat_ipc)
set_target_properties(bharat_ipc PROPERTIES EXPORT_NAME ipc)
add_library(bharat_device STATIC lib/device/device.c)
add_library(Bharat::device ALIAS bharat_device)
set_target_properties(bharat_device PROPERTIES EXPORT_NAME device)
add_library(bharat_cap STATIC lib/capability/capability.c)
add_library(Bharat::cap ALIAS bharat_cap)
set_target_properties(bharat_cap PROPERTIES EXPORT_NAME cap)
foreach(component process ipc device cap)
    target_compile_features(bharat_${component} PUBLIC c_std_11)
    target_include_directories(bharat_${component} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../uapi>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_include_directories(bharat_${component} PRIVATE lib/internal)
    target_link_libraries(bharat_${component} PUBLIC bharat_core)
endforeach()

add_library(bharat_compute STATIC lib/memory/hmem.c lib/compute/tensor.c)
add_library(Bharat::compute ALIAS bharat_compute)
set_target_properties(bharat_compute PROPERTIES EXPORT_NAME compute)
target_compile_features(bharat_compute PUBLIC c_std_11)
target_include_directories(bharat_compute PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../uapi>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

add_library(bharat_sdk INTERFACE)
add_library(Bharat::sdk ALIAS bharat_sdk)
set_target_properties(bharat_sdk PROPERTIES EXPORT_NAME sdk)
target_link_libraries(bharat_sdk INTERFACE
    bharat_core bharat_process bharat_ipc bharat_device bharat_cap bharat_compute)

if(BHARAT_SDK_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
if(BHARAT_SDK_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
if(BHARAT_SDK_BUILD_BENCHMARKS)
    add_subdirectory(../../experience/apps/bench_hmem_tensor
                     ${CMAKE_CURRENT_BINARY_DIR}/bench_hmem_tensor)
endif()

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ../uapi/bharat/memory/hmem.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/bharat/memory)
install(TARGETS bharat_core bharat_process bharat_ipc bharat_device bharat_cap bharat_compute bharat_sdk
    EXPORT BharatSDKTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(EXPORT BharatSDKTargets NAMESPACE Bharat:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/BharatSDK)
include(CMakePackageConfigHelpers)
configure_package_config_file(cmake/BharatSDKConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/BharatSDKConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/BharatSDK)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/BharatSDKConfigVersion.cmake
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/BharatSDKConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/BharatSDKConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/BharatSDK)
