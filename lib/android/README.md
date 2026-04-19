# Bharat-OS Android Compatibility Library Layer

This directory (`lib/android/`) contains the user-space portability library for the Android Compatibility Personality.

## Purpose

It implements the Binder adapters, HAL translation layers (Gralloc, AudioFlinger shims), and ART runtime expectations required to host the Android application framework.

## Scope
* Bionic libc ABI compatibility stubs.
* User-space Binder to Bharat-OS URPC translation.
* Graphic and sensor adapter libraries.

## Boundary
Code here is strictly an adapter. It must map down into the `uapi/runtime/` hosting layer or communicate via IPC to native Bharat-OS display/sensor services. It does not introduce Android-specific code into the core kernel or driver model.
