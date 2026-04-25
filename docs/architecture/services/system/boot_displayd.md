# Boot Display Service (`core/services/system/boot_displayd`)

## Overview

`boot_displayd` is the minimal early-boot visual service for splash, diagnostics, and recovery-safe pages.

## Responsibilities

- **Early rendering** via a framebuffer-first contract.
- **Deterministic tiny pages** (`splash`, `diagnostics`, `recovery`) through `bharat_tiny_ui_state_t`.
- **Simple input mapping** for next/prev/select/back navigation without a compositor dependency.
- **Handoff readiness** so `displayd` can replace the boot path once richer UI services are online.

## Current Implementation Baseline

- Service entrypoint: `core/services/system/boot_displayd/main.c`
- Tiny renderer/runtime: `core/stacks/ui/lcd/tiny_ui.c`
- Public contract: `include/bharat/ui/tiny_ui.h`

The current baseline intentionally stays policy-light and profile-friendly: page-oriented rendering, low-memory compatible buffers, and no direct board-driver reach-around.
