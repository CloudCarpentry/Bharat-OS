# BHCF-005: Benchmark Methodology

## Purpose
This document explains how to benchmark BHCF components and outlines what kinds of metrics are valid on different target environments (emulators vs. real hardware).

## Benchmarking Philosophy
Benchmark instrumentation should verify whether the copies were actually removed or just displaced. We measure changes in application semantics vs runtime/hardware fulfillment.

### Baseline vs HMEM Pipeline Diagram

**LEGACY Pipeline:**
```text
LEGACY

Stage A
  │
  │ memcpy
  ▼
Buffer B
  │
  │ memcpy
  ▼
Buffer C
  │
  │ memcpy
  ▼
Buffer D
```

**BHARAT HMEM Pipeline:**
```text
BHARAT HMEM

                  HMEM
                    │
          ┌─────────┼─────────┐
          ▼         ▼         ▼
       Stage A   Stage B   Stage C
                    │
                    ▼
                 Stage D
```

## Environment Capabilities

### QEMU (Emulators)
QEMU is appropriate for proving:
```text
copy reduction
bytes-moved reduction
allocation reduction
mapping reduction
tensor-view efficiency
API overhead
software-path latency
```

QEMU is **not** appropriate for proving:
```text
real GPU performance
real NPU performance
actual memory-energy savings
real DMA throughput
real PCIe/SMMU/IOMMU latency
```

### Real Hardware Target
Real hardware benchmarking validates the QEMU observations on top of true device synchronization, cache operations, DMA transfers, and physical memory migration. All claims regarding speedups (e.g. `In benchmark X, latency changed from A to B`) and power savings must be run and documented on real hardware configurations before broadly claiming heterogeneous execution benefits.

Please coordinate with the benchmark agent and use the performance claim discipline defined in [BHCF-004-benefits.md](BHCF-004-benefits.md). Do not invent numbers.
