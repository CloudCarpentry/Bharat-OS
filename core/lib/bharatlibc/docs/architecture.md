# BharatLibC Architecture

BharatLibC is a next-generation, profile-driven, isolated standard library designed specifically for Bharat-OS userspace and standalone environments.

## Core Design Principles

1. **Strict Isolation**: BharatLibC relies only on stable, versioned public UAPI headers. It never touches kernel-private headers, HAL headers, or device-specific details.
2. **Profile-Driven Configuration**: Feature sets, sizes, and behaviors are selected via high-level presets (profiles) rather than arbitrary compiler flags.
3. **Backend Abstraction**: Platform-specific system calls and environment operations are accessed via a well-defined backend dispatch layer (`bh_bsys_backend_ops_t`).
4. **No Hidden State**: Avoids lazy global state, unsafe fallbacks, and non-deterministic locks, making it suitable for RT/MPU and safety-critical profiles.
