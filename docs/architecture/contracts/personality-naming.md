---
title: Personality Naming and Vocabulary
status: Proposed
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - contracts
see_also:
  - README.md
---
# Personality Naming and Vocabulary

## 1. Overview
This document defines the definitive enumeration and vocabulary for Bharat-OS personalities. It establishes the "NATIVE" personality as the standard and strictly depreciates the bootstrap "NONE" alias.

## 2. Definitive Enumerations

The following constants must be used across architecture, build configuration, and codebase:

*   `BHARAT_PERSONALITY_NATIVE`: The preferred, long-term first-class model for Bharat-OS.
*   `BHARAT_PERSONALITY_LINUX`: A compatibility personality translating Linux ABI semantics to Bharat contracts.
*   `BHARAT_PERSONALITY_ANDROID`: A compatibility personality supporting Android user-space and Binder constructs.
*   `BHARAT_PERSONALITY_AUTOMOTIVE`: A domain personality tailoring policy and service visibility for automotive environments.
*   `BHARAT_PERSONALITY_NONE`: **DEPRECATED**. Only permissible as a temporary bootstrap alias during initial migration. All long-term, user-facing nomenclature must utilize "NATIVE".

## 3. Implementation Rules

1.  Build systems (e.g., `build_config.json`, CMake) should default to "NATIVE".
2.  Tests testing component policy should verify `BHARAT_PERSONALITY_NATIVE`.
3.  Any reference to "NONE" must be aggressively phased out of all architecture documentation and standard build paths.
