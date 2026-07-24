---
title: CPU Partition Ownership Contract
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - kernel
see_also:
  - README.md
---
# CPU Partition Ownership Contract

## Overview
Defines how CPUs are partitioned into roles (SYSTEM, REALTIME, BEST_EFFORT) based on execution modes.

## Enforcement
- Scheduler checks `cpu_partition_allows_class` before placing a thread.
- RT-dedicated CPUs are isolated from FAIR/GP tasks.
- Housekeeping CPUs handle system-wide deferred work.

## Validation
- Partition configs are validated at boot.
- Inactive CPUs must have no roles or allowed classes.
- A system CPU must always exist.
