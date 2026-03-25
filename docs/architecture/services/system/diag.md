# Diagnostic Service (`services/system/diag`)

## Overview

The `diag` service manages system diagnostics. This includes collecting metrics, running self-tests, handling diagnostic events, and communicating these events to upstream systems.

## Responsibilities

- **Metrics Collection**: Gathering hardware and software telemetry.
- **Event Debouncing**: Preventing flood of identical errors via `debounce.c`.
- **Event Storage**: Securely persisting diagnostic faults and events using `store.c`.

## Boundaries

- Hardware interfacing occurs through HAL/Drivers. The `diag` service only provides the policy/logic layer over collected telemetry.
- IPC is used to expose diagnostic queries to other user-space managers.
