# Bharat-OS POSIX Portability Layer

This directory contains the core POSIX abstraction layer for Bharat-OS.
It is designed to provide minimal, capability-aware POSIX equivalents for basic C/C++ runtimes.

This is a **portability layer** meant to map standard C library functions (like open/read/write) down to Bharat-OS `uapi/` and `uapi/runtime/` calls. It is distinct from the full `linux_compat` personality layer, though the personality layer may rely heavily on this.
