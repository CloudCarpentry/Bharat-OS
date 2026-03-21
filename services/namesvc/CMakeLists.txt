cmake_minimum_required(VERSION 3.20)
project(BharatOS_Services_NameSvc C ASM)

# Add the executable target for the namesvc daemon
add_executable(namesvc main.c src/registry.c)
target_include_directories(namesvc PRIVATE include ../../include)
target_link_libraries(namesvc PRIVATE bharat_user_buildopts bharat_freestanding)

bharat_register_component(
    TARGET namesvc
    KIND "service"
    VERSION "${BHARAT_SVC_NAMESVC_VERSION}"
    IFACE_VERSION "${BHARAT_SVC_NAMESVC_IFACE}"
    CHANNEL "${BHARAT_SVC_NAMESVC_CHANNEL}"
)

# Link against the foundational Phase 1 user-space libraries
target_link_libraries(namesvc PUBLIC runtime ipc cap bharat_version_api)

# Keep the build freestanding and warning-clean

bharat_configure_userspace_binary(namesvc)
