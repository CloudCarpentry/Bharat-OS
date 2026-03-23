# Memory Host Coverage + Benchmark Report (2026-03-23)

This note captures:

1. the **next memory task** we should execute,
2. the **current host-side coverage status** for memory-related code,
3. direct test and benchmark outputs that can be reused for planning.

---

## Recommended Next Task (Updated)

### Task: Extend host memory conformance + retire legacy naming

The immediate follow-up task from the previous report has now been partially executed:

- `tests/host/test_pmm_pcache.c` was expanded with additional PMM command-surface checks (invalid order guard, contiguous alloc/free path, invalid-free guard).
- host CMake now includes dedicated HAL PT/TLB contract conformance targets:
  - `host_test_hal_pt_common`
  - `host_test_hal_pt_fallbacks`
  - `host_test_hal_caps_consistency`
- legacy `vmm_legacy` references were removed from architecture/memory docs.

Remaining high-impact gap: `kernel/src/mm/pmm/numa.c` and distributed TLB shootdown host-link coverage still need a dependency-clean host harness.

---

## Repro Commands Used (Current Update)

```bash
cmake --preset host-test
cmake --build --preset host-test --target \
  test_pmm_pcache \
  host_test_hal_pt_common \
  host_test_hal_pt_fallbacks \
  host_test_hal_caps_consistency \
  test_bench_hal_pt_arm64

cd build/host-test
./tests/host/test_pmm_pcache
./tests/host/host_test_hal_pt_common
./tests/host/host_test_hal_pt_fallbacks
./tests/host/host_test_hal_caps_consistency
./tests/test_bench_hal_pt_arm64

# Legacy-name retirement validation
cd /workspace/Bharat-OS
rg -n "vmm_legacy"
```

---

## Host Validation Snapshot (Memory Scope)

### Added/updated host test coverage focus

| Area | Target/Test | Status |
|---|---|---|
| PMM command paths | `tests/host/test_pmm_pcache.c` (new Phase H) | PASS |
| HAL contracts | `host_test_hal_pt_common`, `host_test_hal_pt_fallbacks`, `host_test_hal_caps_consistency` | PASS |
| x86_64 PT & TLB contracts | `host_test_hal_pt_common` + fallback/caps tests on host x86_64 profile | PASS |
| arm64 PT & TLB (host benchmark proxy) | `test_bench_hal_pt_arm64` | PASS |
| ASpace / VM base objects | `test_vmm_aspace` remains in top-level tests (dependency-coupled in host subtree) | PENDING host-subtree migration |

### Historical llvm-cov coverage snapshot (kept for continuity)

> Note: the following file-coverage numbers are retained from the earlier 2026-03-23 coverage capture and are still useful as baseline trend data until the next llvm-cov refresh.

| File | Region Coverage | Function Coverage | Line Coverage | Branch Coverage |
|---|---:|---:|---:|---:|
| `kernel/src/crypto/secure_mem.c` | 100.00% | 100.00% | 100.00% | 100.00% |
| `kernel/src/mm/pmm/pmm_pcache.c` | 96.30% | 100.00% | 100.00% | 87.50% |
| `kernel/src/mm/pmm/pmm.c` | 47.13% | 39.39% | 49.55% | 36.96% |
| `kernel/src/mm/pmm/numa.c` | 1.21% | 5.88% | 1.45% | 0.00% |

### Host benchmark memory suites

| Suite | Result |
|---|---:|
| `arm64 page-table zero init (manual loop)` | 1676 ns/op |
| `arm64 page-table zero init (arch_memset)` | 43 ns/op |

### Historical memory benchmark suites (kept for continuity)

| File | Region Coverage | Function Coverage | Line Coverage | Branch Coverage |
|---|---:|---:|---:|---:|
| `tests/benchmark/suites/memory/bench_pmm.c` | 90.91% | 100.00% | 100.00% | 83.33% |
| `tests/benchmark/suites/memory/bench_vmm.c` | 90.91% | 100.00% | 100.00% | 83.33% |

---

## Direct Host Test/Baseline Results

From this run:

- `test_pmm_pcache`: PASS including new Phase H PMM command-surface checks.
- `host_test_hal_pt_common`: PASS
- `host_test_hal_pt_fallbacks`: PASS
- `host_test_hal_caps_consistency`: PASS
- `test_bench_hal_pt_arm64`:
  - manual loop: **1676 ns/op**
  - arch_memset path: **43 ns/op**

Additional hygiene result:

- `rg -n "vmm_legacy"` returned no matches.

---

## Gap-to-100% Plan (Short, Revised)

1. Move `test_vmm_aspace` and VM-object checks into `tests/host/` with explicit prot-domain/arch-cap mocks so ASpace framework is host-subtree-native.
2. Add a dedicated `tests/host/test_numa_policy.c` harness to exercise `kernel/src/mm/pmm/numa.c` policy and migration branches with cache stubs.
3. Add host-safe TLB shootdown harness build wiring (current `tests/host/test_tlb_shootdown.c` has include/layout coupling).
4. Re-run llvm-cov file-level reporting for PMM/NUMA/VM once the above harnesses are merged.
