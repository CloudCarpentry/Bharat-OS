# Bharat-OS Staging Area

This directory contains experimental, research, or not-yet-production-ready modules.

## Rules for Staging Code
1. **Not Production Runtime**: Staging code is not part of the production runtime and is intended for development and experimentation only.
2. **Default-Off**: Staging modules must not be linked into the kernel or system images by default. They are enabled via explicit build options.
3. **No Stable UAPI**: Staging modules must not define or depend on stable UAPIs.
4. **Promotion Requirements**: To be moved out of staging into a production directory (e.g., `core/services/`, `drivers/`), a module must have:
    - An assigned owner/maintainer.
    - Comprehensive unit and integration tests.
    - Architecture and design documentation.
    - Clear target placement in the project structure.

## Contents
- `ai/`: Experimental AI-based scheduler governors and heuristics.
- `distributed/`: Research modules for multi-kernel and distributed shared memory.
