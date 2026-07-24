# Bharat-OS Linux Compatibility Library Layer

This directory (`lib/linux_compat/`) contains the user-space portability library for the Linux Compatibility Personality.

## Purpose

It implements the libc shims, ELF loading stubs, and syscall translation mechanisms required to host unaltered Linux binaries.

## Scope
* Syscall trap handlers.
* Translation from Linux file descriptors (FDs) to Bharat-OS Capabilities.
* Signal translation logic.
* Threading/pthread emulation backed by `lib/runtime/host/`.

## Boundary
Code in this folder must *not* leak into `lib/posix/` or core `lib/`. It is strictly tied to the `BHARAT_ENABLE_COMPAT_LINUX` personality path.
