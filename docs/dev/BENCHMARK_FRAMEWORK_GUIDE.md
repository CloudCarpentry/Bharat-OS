# Bharat-OS Benchmark Framework Guide

## 1. Goals
The primary goal of the benchmark framework is to measure and report the performance characteristics (latency, throughput, scaling, fairness, fragmentation, contention cost) of Bharat-OS kernel subsystems across various execution profiles (RT, GP, MIX) and architectures.
It is designed to be separate from correctness tests, allowing for focused performance analysis without unnecessarily blocking CI on varying hardware.

## 2. Non-Goals
- Replacing functional or unit tests.
- Fusing functional correctness assertions with raw performance thresholds (except for specific regression gates).
- Becoming a bloated, generic test framework; it remains lightweight and focused on performance metrics.

## 3. Benchmark Taxonomy
We maintain three layers of testing, and this framework focuses strictly on **Benchmarks**:
1. **Unit/Functional Tests**: Verify correctness, edge cases, failure paths (handled by `kernel_test_t`).
2. **Stress Tests**: Repetition, contention, randomization, leak detection, long-run behavior.
3. **Benchmarks**: Measure precise performance metrics.

Benchmarks are further categorized by maturity:
- **B0**: Experimental, report only.
- **B1**: Stable, tracked for historical analysis.
- **B2**: Regression-gated in CI (fails build if threshold exceeded).
- **B3**: Release-signoff benchmark (strict latency/throughput guarantees).

## 4. Framework Architecture
The framework is designed to run across three backends using a single unified API:
- **Host Backend**: Fast CI and local development.
- **Kernel Backend**: Real kernel path execution.
- **Bare-metal/Profile Backend**: RT/GP/MIX target measurements.

### Directory Structure
```text
quality/tests/benchmark/
├── include/
│   ├── bench.h
│   ├── bench_metrics.h
│   └── bench_runner.h
├── core/
│   ├── bench_runner.c
│   ├── bench_clock.c
│   ├── bench_stats.c
│   ├── bench_output.c
│   └── bench_registry.c
└── suites/
    ├── memory/
    ├── vfs/
    ├── scheduler/
    └── network/
```

### Core API
```c
typedef struct {
    const char *name;
    const char *group;
    int (*setup)(void **ctx);
    int (*run)(void *ctx, uint64_t iterations);
    void (*teardown)(void *ctx);
} bench_case_t;
```

## 5. Metrics Definitions
The framework collects standard timing metrics and OS-specific events:
- **Standard Metrics**: total ns, ns/op, ops/sec, bytes/sec, min/max, median, p95/p99, stddev.
- **OS-specific Metrics**: allocation failures, retries/spin count, lock contention, context switches, migrations, cache hit/miss, queue depth, dropped packets, scheduler unfairness index.

## 6. Subsystem Benchmark Catalog
### Memory Benchmarks
- **PMM**: single-page alloc/free, batch alloc/free, fragmentation scaling, cross-core remote free.
- **VMM/TLB**: map/unmap latency, page-fault handling, TLB shootdown overhead, active address-space switch cost.

### VFS Benchmarks
- **Metadata**: create/unlink, open/close, stat/lookups, deep path traversal.
- **Data**: sequential read/write, random IO, varying sizes (1-byte to 1MB).

### Scheduler Benchmarks
- **Core**: context switch latency, wakeup latency, timer jitter, fairness, runqueue lock contention.
- **Scenarios**: thread ping-pong, CPU-bound vs IO-bound, mixed priority workloads.

### Networking Benchmarks
- **Internal**: packet alloc/free, RX parse, TX path cost, socket queueing.
- **End-to-End**: UDP echo latency, throughput (pps/Mbps), small-packet stress.

## 7. Output Formats
The runner supports multiple output formats for CI and human consumption:
- **Console**: Human-readable tables.
- **JSON**: Structured output for regression tracking.
- **CSV**: For spreadsheet analysis.

All outputs include an environment stamp (arch, profile, compiler, core count) to provide necessary context for the metrics.

## 8. CI and Regression Policy
- Only **B2** and **B3** benchmarks will gate CI pipelines.
- CI fails only if a stable benchmark regresses beyond a defined percentage threshold against the baseline.
- **B0** and **B1** are purely informational in CI.

## 9. Future Kernel-Mode Extension
Phase 4 of the framework roadmap includes porting the runner backend to `core/kernel/quality/tests/bench/` to allow seamless bare-metal, profile-aware benchmarking directly on the target hardware without relying on host stubs.
