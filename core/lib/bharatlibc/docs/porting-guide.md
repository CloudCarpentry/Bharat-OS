# BharatLibC Porting Guide

Porting BharatLibC to a new processor architecture or hardware platform requires two major tasks:

## 1. CRT (C Runtime Initialization) & Architecture Support
- Implement the standard startup sequence in `src/crt/arch/<arch>/crt0.S` or `crt0.c`.
- Implement highly-optimized string and memory functions if needed under `src/arch/<arch>/`. If omitted, standard generic implementations under `src/arch/generic/` are automatically used.

## 2. Implementing the BSys Backend
- Define and registers a implementation of the `bh_bsys_backend_ops_t` interface.
- Provide functions for `write`, `read`, `clock_gettime`, `sleep_until`, `heap_region`, and `process_exit`.
- If a system call or device capability is absent, return `K_ERR_UNSUPPORTED` rather than simulating false successes.
