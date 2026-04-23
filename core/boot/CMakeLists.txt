# Canonical boot tree (Phase D1b)
# Sources are consumed from kernel/CMakeLists via BHARAT_BOOT_ROOT.
add_library(bharat_boot_headers INTERFACE)
target_include_directories(bharat_boot_headers INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
