# Legacy boot shim.
# Canonical boot tree now lives under core/boot.
if(EXISTS "${CMAKE_SOURCE_DIR}/core/boot/CMakeLists.txt")
    message(STATUS "[migration] boot/ is a compatibility shim; using core/boot")
endif()
