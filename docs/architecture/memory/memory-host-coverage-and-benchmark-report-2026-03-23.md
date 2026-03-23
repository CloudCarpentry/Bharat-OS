# Memory Host Coverage + Benchmark Report (2026-03-23)

This note captures:

1. the **next memory task** we should execute,
2. the **current host-side coverage status** for memory-related code,
3. direct test and benchmark outputs that can be reused for planning.

---

## Recommended Next Task

### Task: Build a dedicated host conformance suite for PMM/NUMA branch coverage

The highest-impact next task is to close the host-coverage gap in:

- `kernel/src/mm/pmm/pmm.c`
- `kernel/src/mm/pmm/numa.c`

Current host tests strongly exercise per-core PMM cache behavior (`pmm_pcache.c`) but leave large PMM and NUMA decision paths unexecuted. To move toward 100% host coverage in memory areas, we should implement a new focused suite (or expand `tests/host/test_pmm_pcache.c`) to cover:

- zone selection failure and fallback branches,
- contiguous/high-order allocation failure modes,
- refcount edge/error paths,
- NUMA policy path selection and invalid-node guards,
- metadata invariant failure handling.

---

## Repro Commands Used

```bash
cmake -S . -B build/memory-host-coverage -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/host-linux-clang.cmake \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBHARAT_BUILD_HOST_TESTS=ON \
  -DBHARAT_DEVICE_PROFILE=DESKTOP \
  -DBHARAT_PERSONALITY_PROFILE=LINUX \
  -DCMAKE_C_FLAGS='-fprofile-instr-generate -fcoverage-mapping'

cmake --build build/memory-host-coverage --target \
  test_secure_mem test_pmm_pcache bench_runner

cd build/memory-host-coverage
LLVM_PROFILE_FILE='profiles/test_secure_mem.profraw' ./tests/host/test_secure_mem
LLVM_PROFILE_FILE='profiles/test_pmm_pcache.profraw' ./tests/host/test_pmm_pcache
LLVM_PROFILE_FILE='profiles/bench_runner.profraw' ./tests/benchmark/bench_runner

llvm-profdata merge -sparse \
  profiles/test_secure_mem.profraw \
  profiles/test_pmm_pcache.profraw \
  profiles/bench_runner.profraw \
  -o profiles/memory-suite.profdata

llvm-cov report ./tests/host/test_secure_mem \
  -instr-profile=profiles/memory-suite.profdata

llvm-cov report ./tests/host/test_pmm_pcache \
  -instr-profile=profiles/memory-suite.profdata

llvm-cov report ./tests/benchmark/bench_runner \
  -instr-profile=profiles/memory-suite.profdata
```

---

## Host Coverage Snapshot (Memory Scope)

### Core memory implementation files

| File | Region Coverage | Function Coverage | Line Coverage | Branch Coverage |
|---|---:|---:|---:|---:|
| `kernel/src/crypto/secure_mem.c` | 100.00% | 100.00% | 100.00% | 100.00% |
| `kernel/src/mm/pmm/pmm_pcache.c` | 96.30% | 100.00% | 100.00% | 87.50% |
| `kernel/src/mm/pmm/pmm.c` | 47.13% | 39.39% | 49.55% | 36.96% |
| `kernel/src/mm/pmm/numa.c` | 1.21% | 5.88% | 1.45% | 0.00% |

### Host benchmark memory suites

| File | Region Coverage | Function Coverage | Line Coverage | Branch Coverage |
|---|---:|---:|---:|---:|
| `tests/benchmark/suites/memory/bench_pmm.c` | 90.91% | 100.00% | 100.00% | 83.33% |
| `tests/benchmark/suites/memory/bench_vmm.c` | 90.91% | 100.00% | 100.00% | 83.33% |

---

## Direct Benchmark Results (Host)

From `./tests/benchmark/bench_runner`:

- `pmm_alloc_free` (memory): **2316 ns/op**, **431,604 ops/sec**
- `vmm_map_unmap_1pte` (memory): **13 ns/op**, **72,346,514 ops/sec**

These values are useful as a baseline for future memory-optimization PRs and for regression tracking in docs/CI artifacts.

---

## Gap-to-100% Plan (Short)

1. Add PMM failure-path tests (allocation exhaustion, invalid zone/order, fallback behavior).
2. Add NUMA policy tests with synthetic topology matrices.
3. Add branch-specific checks for secure memory allocation failure path and flag-specific behavior.
4. Run host coverage gate per memory target file and fail if <100% for selected scope.

Until PMM/NUMA branch surfaces are exercised, we cannot honestly claim 100% host coverage for memory-related areas.
