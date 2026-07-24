# services/device/actuator_mgr

## Purpose
This module is a **device policy manager**, not a driver. It resides in `services/device/` and is responsible for managing actuator state policies, bounds checking, plausibility logic, and safe-state transitions (such as during system faults). Real hardware control over actuators is delegated to the appropriate drivers (e.g., `drivers/actuator/` or specific CAN/LIN controllers).

## Overview
The Actuator Manager orchestrates safety-critical output decisions. It evaluates commands against configured safety limits and system status. If the system enters a degraded mode, the Actuator Manager is responsible for commanding the underlying drivers into predefined safe states.
