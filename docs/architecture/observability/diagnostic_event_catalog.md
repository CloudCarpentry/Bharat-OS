<!-- SPDX-License-Identifier: MIT -->
# Diagnostic event catalog v1

Source kinds are `KERNEL`, `SERVICE`, `DRIVER`, `STACK`, `RUNTIME`, `BUILD_TOOL`, and `TEST_HARNESS`. Severities are `TRACE`, `INFO`, `NOTICE`, `WARNING`, `RECOVERABLE`, `CRITICAL`, and descriptive-only `FATAL`. Subsystems are boot, scheduler, memory, IPC, capability, process, service, driver, network, display, security, power, watchdog, and unknown.

Event type numbers are producer-specific until a registry contract is approved. Consumers preserve unknown types and must use `header_size` and `payload_size` to skip them safely.
