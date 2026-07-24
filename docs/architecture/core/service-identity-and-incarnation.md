---
title: Service Identity and Incarnation
status: Proposed
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - core
see_also:
  - README.md
---
# Service Identity and Incarnation

This document details the primitives governing service identity and restarting within the Bharat-OS multikernel.

## Problem Statement

When services restart in a distributed architecture, capabilities to their endpoints may remain in the system. To avoid "stale handle" bugs where old capabilities inadvertently grant access to a new service instance sharing the same object ID or memory address, Bharat-OS uses strict incarnation checking.

## Endpoint Generation (`endpoint_gen`)

Every service and its primary IPC endpoints have an associated `endpoint_gen`.

- **Initialization:** Starts at `ENDPOINT_GENERATION_INITIAL` (e.g., 1).
- **Restart/Reincarnation:** When a service restarts, its generation is strictly incremented.
- **Validation:** Every uRPC message destined for the service must carry the known `endpoint_gen` of the capability used. If the message's generation is strictly less than the service's current generation, it is rejected as stale.

## Service ID (`service_id`)

The `service_id` is the stable identity for an entity over its lifetime, while `endpoint_gen` marks the temporal incarnation of that identity.
