# BharatProfiles.cmake
# Device profile configuration for Bharat-OS

# This module handles device profile configuration
# Device profiles define the target environment (DESKTOP, MOBILE, EDGE, etc.)

# Initialize profile variables if not already set
if(NOT DEFINED BHARAT_DEVICE_PROFILE)
    set(BHARAT_DEVICE_PROFILE "DESKTOP" CACHE STRING "Target device profile")
endif()

message(STATUS "Device Profile: ${BHARAT_DEVICE_PROFILE}")
