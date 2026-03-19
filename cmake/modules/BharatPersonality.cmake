# BharatPersonality.cmake
# Personality profile configuration for Bharat-OS

# This module handles personality profile configuration
# Personalities define compatibility layers (NONE, LINUX, WINDOWS, MAC)

# Initialize personality variables if not already set
if(NOT DEFINED BHARAT_PERSONALITY_PROFILE)
    set(BHARAT_PERSONALITY_PROFILE "NONE" CACHE STRING "Target personality profile")
endif()

message(STATUS "Personality Profile: ${BHARAT_PERSONALITY_PROFILE}")
