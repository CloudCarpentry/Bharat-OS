# Boot Display Service (`services/system/boot_displayd`)

## Overview

`boot_displayd` is a minimal early-boot graphical/visual service responsible for handling splash screens, boot progress indicators, and early recovery UI.

## Responsibilities

- **Early Rendering**: Utilizing the raw framebuffer or display capabilities provided by the kernel early in the boot sequence.
- **Progress Updates**: Displaying visual feedback during the initial phases of system startup.

## Architecture

This service is replaced or transitioned out once the full user-space display manager (`displayd`) or equivalent windowing system takes over.
