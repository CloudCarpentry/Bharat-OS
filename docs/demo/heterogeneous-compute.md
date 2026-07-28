---
title: Bharat-OS Heterogeneous Compute Control Plane Demonstration (HET-DEMO-001)
status: Approved
owner: Compute Architecture WG
last_updated: 2026-04-25
tags:
  - demo
  - compute
  - heterogeneous-compute
  - npu
see_also:
  - README.md
  - docs/dev/current-code-status.md
---

# Truthful Virtual NPU & CPU Fallback Demonstration (HET-DEMO-001)

This document describes the design, execution, and verification of the heterogeneous compute dispatch substrate on Bharat-OS. The demonstration validates the architectural separating pillars of runtime routing, policy compliance, failover fallback, and hardware emulation without relying on fake hardware/mock success patterns.

## Architectural Story & Overview

Bharat-OS decouples heterogeneous acceleration into a clear, four-layer taxonomy:

1. **Kernel:** Enforces hardware capabilities and sandboxed device bounds.
2. **Drivers:** Provides hardware-specific execution queues under standard interface contracts.
3. **Services:** Orchestrates system-wide admission, QoS, and resource distribution.
4. **Runtime:** Selects the optimal available backend and handles high-level graph execution.

This demonstration models a **Tensor ReLU operation** dispatched dynamically to either an emulated hardware accelerator (Virtual NPU) or a software CPU fallback based on system capability state and policy rules.

```text
                 Bharat-OS Compute Fabric

                    Tensor workload
                         |
                  Runtime Dispatcher
                    /           \
                   /             \
             Virtual NPU       CPU
                |                |
            AVAILABLE         FALLBACK
                |                |
                +-------+--------+
                        |
                     Result
```

## Demonstration Scenarios

### Scenario A: Normal Acceleration
* **State/Policy:** Virtual NPU discovered (`caps->soc.npu = HW_CAP_STATE_PRESENT`), and safe mode is disabled (`safe_mode = false`).
* **Expected Dispatch:** The runtime dynamically selects the emulated `tensor_hw_generic` backend, submits the request to the `virt_accel_0` driver, and performs element-wise FP32 ReLU execution.
* **Result Verification:** Verify NPU driver execution counts increment and math outputs match exactly.

### Scenario B: Policy-Constrained Safe-Mode Fallback
* **State/Policy:** Virtual NPU discovered, but system safe mode is activated (`safe_mode = true`).
* **Expected Dispatch:** Due to strict platform safety governance, hardware execution is prohibited. The runtime dispatch layer falls back deterministically to the software CPU driver (`tensor_sw_fallback`).
* **Result Verification:** NPU driver submission counts remain unchanged, and outputs are correctly computed on the CPU.

### Scenario C: Unavailable Hardware Fallback
* **State/Policy:** System capability record indicates no NPU is available (`caps->soc.npu = HW_CAP_STATE_ABSENT`).
* **Expected Dispatch:** The dispatcher routes execution to the software CPU driver.
* **Result Verification:** NPU driver submission counts remain unchanged, and outputs are correctly computed on the CPU.

## Execution and Verification

The demonstration is fully verified using host-side isolated unit and contract tests:

```bash
# Build and run the host-side verification test suite
gcc -o test_accel_dispatch quality/tests/host/accel/test_accel_dispatch.c \
    core/lib/runtime/backend_dispatch/backend_registry.c \
    core/lib/runtime/accel/tensor_dispatch.c \
    core/drivers/accel/virt_accel.c \
    -I. -Icore/kernel/include -Iinterface -Iinterface/include -Icore/lib/include -Icore/lib/runtime/include

./test_accel_dispatch
```

### Emitted Telemetry & Validation Markers

During execution, the test suite emits the following deterministic markers:

```text
BHARAT_HETERO_DEMO:START
BHARAT_HETERO_DEMO:VIRT_NPU=AVAILABLE

BHARAT_HETERO_DEMO:CASE=NORMAL
BHARAT_HETERO_DEMO:BACKEND=tensor_hw_generic
BHARAT_HETERO_DEMO:RESULT=PASS

BHARAT_HETERO_DEMO:CASE=SAFE_MODE
BHARAT_HETERO_DEMO:BACKEND=CPU
BHARAT_HETERO_DEMO:RESULT=PASS

BHARAT_HETERO_DEMO:CASE=NPU_ABSENT
BHARAT_HETERO_DEMO:BACKEND=CPU
BHARAT_HETERO_DEMO:RESULT=PASS

BHARAT_HETERO_DEMO:COMPLETE
```
