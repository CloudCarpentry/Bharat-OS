# HMEM and Tensor benchmark suite

This suite demonstrates reduced **software data movement and object-management
overhead**. QEMU timing is reported only as a same-run software-path indicator;
it is not evidence of physical GPU, NPU, DMA, cache, or memory performance and
must not be compared with Linux without an equivalent controlled implementation.

The benchmark-mode kernel runs a four-buffer baseline against one mapped HMEM
object. Deterministic PASS criteria require equal checksums, three baseline
handoff copies per iteration, and zero HMEM handoff copies/bytes. Instrumentation
is benchmark-local, so production HMEM paths are unchanged. The SDK executable
also compares a physical tensor slice with a metadata-only tensor view.

Certified benchmark targets currently cover x86_64, ARM64, and RISC-V 64 with
four virtual CPUs and 1 GiB RAM. QEMU provides CPU and memory virtualization;
these profiles do **not** declare a real GPU/NPU. Virtual accelerator integration
will be separate evidence when BHCF-P1-003 is implemented.

```bash
python3 tools/bench/run_hmem_bench.py --target \
  delivery/targets/qemu/x86_64_hmem_bench_release.yaml
python3 tools/bench/run_hmem_bench.py --matrix
```

The runner fails on missing targets, build/package failures, timeout, missing or
duplicate telemetry, checksum mismatch, non-zero HMEM handoff copies, or a guest
FAIL. It writes JSON and CSV under `build/bench-results/` by default. Generated
results are evidence artifacts and must not be committed.

Build and run the SDK path independently:

```bash
cmake -S interface/sdk -B build/sdk-bench -DCMAKE_BUILD_TYPE=Release \
  -DBHARAT_SDK_BUILD_BENCHMARKS=ON
cmake --build build/sdk-bench --target bench_hmem_tensor
./build/sdk-bench/bench_hmem_tensor/bench_hmem_tensor 1048576 100
```
