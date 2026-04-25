---
title: Constraint-Aware Execution Substrate
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - core
see_also:
  - README.md
---
# Constraint-Aware Execution Substrate

This document outlines the core architecture for Bharat-OS constraint-aware execution.

Constraints provide a unified mechanism for exposing workload intent, memory semantic hints, and dataflow execution constraints to the kernel.

The kernel is strictly a consumer of these constraints. The AI heuristic scheduler, memory allocator, and other sub-systems must prioritize admissibility defined by constraints.

**Rules:**
- Kernel remains mechanism only. Policy is pushed to services.
- Constraints restrict the scheduler.
- No global constraint registry. They are attached to threads, VMAs, etc.
