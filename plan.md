1. **Implement Preemptive, Per-Core Scheduler**
   - Create `kernel/src/sched.c`.
   - Implement `core_runqueue_t` for `MAX_SUPPORTED_CORES` (e.g., 8). Use `hal_cpu_get_id()` to get the current core's ID.
   - Implement `sched_init()` to initialize local queues for all cores. Also allocate an idle thread and local queue for APs.
   - Implement thread creation to enqueue to a runqueue (e.g., default to core 0 or the current core).
   - Implement `sched_yield()` to schedule tasks from the local queue.
   - Support `AI_ACTION_MIGRATE_TASK` to migrate threads between cores.
   - Note: update `kernel/CMakeLists.txt` to compile `src/sched.c` instead of `src/sched_stub.c`.

2. **Implement Full Page-Table Manager**
   - Modify `kernel/src/mm/vmm.c`.
   - Update `mm_create_address_space` to allocate a full 4-level root (PML4) / RISC-V SATP page structure per process and correctly populate it.
   - Update `tlb_shootdown` logic in `vmm.c`. Keep a track of which cores are executing a given process. If other cores are executing threads of the modified address space, send an IPI to them via `hal_send_ipi_payload` to flush their TLB caches, along with the local `hal_tlb_flush`.
   - Ensure the VMM functions (`mm_vmm_map_page`, `mm_vmm_unmap_page`, `vmm_handle_cow_fault`) use these per-process structures correctly.

3. **Integrate Capability-Based Policy Engine**
   - In `vmm_map_device_mmio()` in `kernel/src/mm/vmm.c`, add capability checks. Use `cap_table_lookup()` on the given capability token to verify `CAP_RIGHT_DEVICE_NPU` or `CAP_RIGHT_DEVICE_GPU` (based on `is_npu` parameter).
   - Add a test case (e.g. `tests/test_capability_policy.c`) that explicitly tries to `vmm_map_device_mmio` with a capability lacking the required rights, asserting that it returns an error (e.g., `-3` or `-2` as defined in `vmm.c`).

4. **Tests and Verification**
   - Run tests using standard test commands (e.g., `cmake -S tests -B build-tests && cmake --build build-tests && ctest --test-dir build-tests`) to verify it builds and passes.

5. **Complete pre-commit steps**
   - Run the pre-commit instructions.

6. **Submit changes**
   - Commit and submit.
