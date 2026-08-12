# BharatOS Code Analysis Report

## TODO Items
```
./third_party/lvgl/upstream/src/libs/nanovg/nanovg_gl.h:532:    // TODO: mediump float may not be enough for GLES2 in iOS.
./third_party/lvgl/upstream/src/libs/lodepng/lodepng.c:100:/* TODO: support giving additional void* payload to the custom allocators */
./third_party/lvgl/upstream/src/libs/lodepng/lodepng.c:452:/*TODO: this ignores potential out of memory errors*/
./third_party/lvgl/upstream/src/libs/lodepng/lodepng.c:470:        /* TODO: increase output size only once here rather than in each WRITEBIT */
./third_party/lvgl/upstream/src/libs/lodepng/lodepng.c:483:        /* TODO: increase output size only once here rather than in each WRITEBIT */
./third_party/lvgl/upstream/src/libs/lodepng/lodepng.c:626:    /*TODO: implement faster lookup table based version when needed*/
./third_party/lvgl/upstream/src/libs/lodepng/lodepng.c:1286:                /* TODO: revise error codes 10,11,50: the above comment is no longer valid */
./third_party/lvgl/upstream/src/libs/lodepng/lodepng.c:1412:            /* TODO: revise error codes 10,11,50: the above comment is no longer valid */
./third_party/lvgl/upstream/src/libs/lodepng/lodepng.c:1535:    /*binary search (only small gain over linear). TODO: use CPU log2 instruction for getting symbols instead*/
./third_party/lvgl/upstream/src/libs/lodepng/lodepng.c:1562:    /*TODO: return error when this fails (out of memory)*/
./third_party/lvgl/upstream/src/libs/lodepng/lodepng.c:1583:    /*TODO: do this not only for zeros but for any repeated byte. However for PNG
./third_party/lvgl/upstream/src/libs/lodepng/lodepng.c:2845:/* TODO: make this faster */
./third_party/lvgl/upstream/src/libs/lodepng/lodepng.c:4631:    /* TODO: remove this. One should use a new LodePNGState for new sessions */
./third_party/lvgl/upstream/src/libs/lodepng/lodepng.c:4649:    /*TODO: remove the undocumented feature that allows to give null pointers to width or height*/
./third_party/lvgl/upstream/src/libs/lodepng/lodepng.c:5091:            /*TODO: possible efficiency improvement: if in this reduced image the bits fit nicely in 1 scanline,
./third_party/lvgl/upstream/src/libs/lodepng/lodepng.c:5832:        /*TODO: check if this works according to the statement in the documentation: "The converter can convert
./third_party/lvgl/upstream/src/libs/lodepng/lodepng.c:6879:            TODO: more conversions may be possible, and it may also be possible to get a more appropriate color type out of
./third_party/lvgl/upstream/src/libs/lodepng/lodepng.h:743:    /* TODO: make a system involving warnings with levels and a strict mode instead. Other potentially recoverable
./third_party/lvgl/upstream/src/libs/lodepng/lodepng.h:1180:TODO:
./third_party/lvgl/upstream/src/libs/svg/lv_svg_parser.c:633:    // TODO: use NEON to optimize this function on ARM architecture.
./third_party/lvgl/upstream/src/libs/svg/lv_svg_token.c:174:    //TODO: processing DTD type
./third_party/lvgl/upstream/src/libs/tiny_ttf/stb_truetype_htcw.h:967:// @TODO: don't expose this structure
./third_party/lvgl/upstream/src/libs/tiny_ttf/stb_truetype_htcw.h:1711:        STBTT_assert(0); // @TODO: high-byte mapping for japanese/chinese/korean
./third_party/lvgl/upstream/src/libs/tiny_ttf/stb_truetype_htcw.h:3498:                    // @TODO: maybe test against sy1 rather than y_bottom?
./third_party/lvgl/upstream/src/libs/gltf/gltf_view/lv_gltf_view_render.cpp:1131:    /* TODO: test both ways to see which has less distortion*/
./third_party/lvgl/upstream/src/libs/gltf/gltf_view/assets/lv_gltf_view_shader.c:1126:                // TODO: taking length of blue ray, ideally we would take the length of the green ray. For now overwriting seems ok
./third_party/lvgl/upstream/src/libs/gltf/stb_image/stb_image.h:1283:    // @TODO: move stbi__convert_format to here
./third_party/lvgl/upstream/src/libs/gltf/stb_image/stb_image.h:1309:    // @TODO: move stbi__convert_format16 to here
./third_party/lvgl/upstream/src/libs/gltf/stb_image/stb_image.h:1310:    // @TODO: special case RGB-to-Y (and RGBA-to-YA) for 8-bit-to-16-bit case to keep more precision
./third_party/lvgl/upstream/src/drivers/display/st7796/lv_st7796.c:107:    /* NOTE: the generic method is not supported on ST7796, TODO: implement gamma tables */
./third_party/lvgl/upstream/src/drivers/display/lcd/lv_lcd_generic_mipi.c:332:    /* TODO: implement resolution change */
./third_party/lvgl/upstream/src/widgets/scale/lv_scale.c:1144:        /* TODO: Add compensation for the width of the first and last tick over the arc */
./third_party/lvgl/upstream/src/widgets/scale/lv_scale.c:1168:            /* TODO: Add compensation for the width of the first and last tick over the arc */
./third_party/lvgl/upstream/src/stdlib/builtin/lv_tlsf.c:256:    ** TODO: We can increase this to support larger sizes, at the expense
./third_party/lvgl/upstream/src/misc/lv_matrix.c:125:    /*TODO: use NEON to optimize this function on ARM architecture.*/
./third_party/lvgl/upstream/src/misc/lv_text.c:212: * TODO: Returned word_w_ptr may overestimate the returned word's width when
./third_party/lvgl/upstream/src/misc/lv_text.c:330:             * TODO: it would be appropriate to update the returned
./third_party/lvgl/upstream/src/draw/nanovg/lv_draw_nanovg_label.c:196:        /* TODO: draw rotated bitmap */
./third_party/lvgl/upstream/src/draw/nanovg/lv_draw_nanovg_label.c:322:                        /* TODO: draw_letter_outline(t, glyph_draw_dsc); */
./third_party/lvgl/upstream/src/draw/sw/blend/neon/lv_draw_sw_blend_neon_to_rgb888.c:591:#if 0 /* TODO: Figure out the problem with the algorithm below */
./staging/ai/ai_kernel_bridge.c:7:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./build-tests-host/generated/include/CapabilityService_dispatch.c:15:        // TODO: call Validate
./build-tests-host/generated/include/CapabilityService_dispatch.c:18:        // TODO: call Create
./build-tests-host/generated/include/CapabilityService_dispatch.c:21:        // TODO: call Revoke
./build-tests-host/generated/include/CapabilityService_dispatch.c:24:        // TODO: call Transfer
./build-tests-host/generated/include/CapabilityService_dispatch.c:27:        // TODO: call Inspect
./build-tests-host/generated/include/bharat_monitor_v1_dispatch.c:13:        // TODO: call Heartbeat
./build-tests-host/generated/include/bharat_monitor_v1_dispatch.c:16:        // TODO: call NodeJoin
./build-tests-host/generated/include/bharat_monitor_v1_dispatch.c:19:        // TODO: call TlbInvalidate
./core/services/faultmgr/main.c:12:    // TODO: Track critical services via servicemgr
./core/services/faultmgr/main.c:13:    // TODO: Wait for coremgr or telemetrymgr health alerts
./core/services/faultmgr/main.c:24:        // TODO: Handle panic messages, quarantine nodes, or initiate service restarts
./core/services/faultmgr/CMakeLists.txt:19:    # TODO: Add runtime and ipc libraries when available
./core/services/schedmgr/CMakeLists.txt:23:    # TODO: Add runtime and ipc libraries when available
./core/services/legacy/net/main.c:67:    // TODO: Acquire Capability for NIC Driver Rings via io_setup_zero_copy_nic_ring()
./core/services/legacy/net/main.c:68:    // TODO: Register the NIC driver as a `netdev_t` device
./core/services/legacy/net/main.c:69:    // TODO: Launch protocol processing thread (e.g. lwIP or native stack)
./core/services/core/policymgr/profile_policy.c:14:    // TODO: Load from boot config/hardware profile
./core/services/drivers/main.c:60:    // TODO: Map device registers into this process space using kernel APIs.
./core/services/drivers/main.c:61:    // TODO: Await interrupts via IPC from the kernel.
./core/services/drivers/main.c:62:    // TODO: Map URPC shared memory for high-bandwidth endpoints (Network, Disk).
./core/services/drivers/main.c:63:    // TODO: Bridge CAN/LIN/Ethernet drivers to user-space policy supervisors.
./core/services/devmgr/main.c:12:    // TODO: Register with servicemgr/namesvc
./core/services/devmgr/main.c:13:    // TODO: Discover root buses (PCIe, CXL)
./core/services/devmgr/main.c:14:    // TODO: Configure IOMMU domains / isolation policies
./core/services/devmgr/main.c:25:        // TODO: Wait for hotplug events, device reset requests, or driver bind requests
./core/services/devmgr/CMakeLists.txt:19:    # TODO: Add runtime and ipc libraries when available
./core/services/system/shell/src/shell_backend_runtime.c:14:    // TODO: Call real time/diag service
./core/services/system/shell/src/shell_backend_runtime.c:31:    // TODO: Call servicemgr
./core/services/system/shell/src/shell_backend_runtime.c:51:    // TODO: Call devmgr
./core/services/system/shell/src/shell_backend_runtime.c:56:    // TODO: Call memmgr
./core/services/coremgr/main.c:12:    // TODO: Register with namesvc or servicemgr
./core/services/coremgr/main.c:13:    // TODO: Establish local core topology map
./core/services/coremgr/main.c:14:    // TODO: Monitor core online/offline status
./core/services/coremgr/main.c:25:        // TODO: Wait for remote execution requests, topology updates, or transport health alerts
./core/services/coremgr/CMakeLists.txt:19:    # TODO: Add runtime and ipc libraries when available
./core/services/memmgr/main.c:12:    // TODO: Connect to kernel mechanisms (e.g. wait on page faults queue)
./core/services/memmgr/main.c:13:    // TODO: Define initial memory pressure thresholds
./core/services/memmgr/main.c:14:    // TODO: Establish basic region and COW policies
./core/services/memmgr/main.c:25:        // TODO: Handle kernel page faults, memory migration requests, and pressure events
./core/services/memmgr/CMakeLists.txt:19:    # TODO: Add runtime and ipc libraries when available
./core/services/storagemgr/main.c:12:    // TODO: Link up with lower-level block drivers or file systems
./core/services/storagemgr/main.c:13:    // TODO: Determine initial namespace/mount trees
./core/services/storagemgr/main.c:24:        // TODO: Wait for mount requests, block queue alerts, or tiered-storage migration triggers
./core/services/storagemgr/CMakeLists.txt:19:    # TODO: Add runtime and ipc libraries when available
./core/arch/hal/riscv/riscv64/dma.c:11:// TODO: If Zicbom is available, use `cbo.clean` and `cbo.inval`
./core/arch/hal/riscv/riscv64/hal_cpu.c:72:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/arch/hal/mpu/arm32/mpu_backend.c:22:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/arch/hal/arm/arm64/hal_cpu.c:37:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/arch/hal/x86/x86_64/topology_acpi.c:213:        // TODO: SRAT and SLIT for NUMA memory/distance
./core/arch/hal/x86/x86_64/pmu.c:18:    // TODO: Check CPUID for Architectural PMU version and capabilities.
./core/arch/hal/x86/x86_64/hal_cpu.c:76:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/hal/common/hal_dma_coherency.c:62:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/src/mm/dma/dma_grant.c:29:        // TODO: Consult profile/policy for fallback allowance
./core/kernel/src/sched/sched_thread.c:187:    // TODO: store intent somewhere
./core/kernel/src/sched/sched_thread.c:196:    // TODO: retrieve intent from somewhere
./core/kernel/src/tests/ktest_capability.c:19:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/src/tests/test_vm_space.c:8:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/src/tests/test_vm_space.c:10:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/src/tests/test_vm_space.c:12:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/src/tests/test_vm_space.c:14:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/src/tests/test_vm_space.c:16:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/personality_ops.h:6:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/profile/profile.h:42:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/profile/system_profile.h:16:/* TODO: Unify legacy profile.h definitions with this new system profile model in a future PR */
./core/kernel/include/device.h:158:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/io_subsys.h:54:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/core/multikernel.h:47:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/arch/arch_cpu_caps.h:27:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/arch/arch_cpu_caps.h:30:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/arch/arch_cpu_caps.h:34:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/arch/arch_cpu_caps.h:37:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/mm/aspace.h:52:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/mm/vm_object.h:63:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/mm/vm_object.h:65:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/mm/pmm.h:102:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/mm/iommu.h:84:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/mm/dma.h:25:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/mm/dma.h:27:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/mm/mm_remote.h:27:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/security/isolation.h:89:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/mm.h:18:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/mm.h:20:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/mm.h:22:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/mm.h:101:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/mm.h:105:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/mm.h:124:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/mm.h:129:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/hal/hal_discovery.h:205:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/hal/hal.h:22:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/hal/hal_iommu.h:60:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/tests/ktest.h:16:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/bharat/cpu_local.h:22:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/kernel/include/bharat/cpu_local.h:24:// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
./core/lib/runtime/src/crt0.c:11:/* TODO: Implement real TLS initialization for __thread variables */
./core/lib/runtime/host/runtime_host.c:15:    // TODO: Initialize capability arena and establish URPC connection to service manager.
./core/lib/runtime/host/runtime_host.c:21:    // TODO: Construct SpawnRequest over URPC to lifecycle manager.
./core/lib/runtime/host/runtime_host.c:27:    // TODO: Send state update over URPC to lifecycle manager.
./core/lib/runtime/host/runtime_host.c:33:    // TODO: Map to underlying kernel thread creation capability.
./core/lib/runtime/host/runtime_host.c:39:    // TODO: Wait on thread capability handle.
./core/lib/runtime/host/runtime_host.c:47:    // TODO: Perform URPC lookup against local namespace manager.
./core/lib/runtime/host/runtime_host.c:53:    // TODO: Perform URPC synchronous send/receive.
./core/lib/runtime/host/runtime_host.c:81:    // TODO: URPC read request to the capability handle.
./core/lib/runtime/host/runtime_host.c:89:    // TODO: URPC write request to the capability handle.
./core/lib/runtime/host/runtime_host.c:97:    // TODO: Drop capability handle.
./quality/tests/test_vmm_aspace.c:265:    // TODO: Add a test seam to explicitly test FLAT profile behavior once architecture allows easier mocking,
./interface/include/bharat/uapi/device/bharat_device_accelmgr_v2_dispatch.c:24:        // TODO: call GetBrokerInfo
./interface/include/bharat/uapi/device/bharat_device_accelmgr_v2_dispatch.c:27:        // TODO: call GetDeviceCount
./interface/include/bharat/uapi/device/bharat_device_accelmgr_v2_dispatch.c:30:        // TODO: call GetDeviceInfo
./interface/include/bharat/uapi/device/bharat_device_accelmgr_v2_dispatch.c:33:        // TODO: call RequestAdmission
./interface/include/bharat/uapi/device/bharat_device_accelmgr_v2_dispatch.c:36:        // TODO: call ReleaseContext
./interface/include/bharat/uapi/device/bharat_device_accelmgr_v2_dispatch.c:39:        // TODO: call CreateQueue
./interface/include/bharat/uapi/device/bharat_device_accelmgr_v2_dispatch.c:42:        // TODO: call DestroyQueue
./interface/include/bharat/uapi/device/bharat_device_accelmgr_v2_dispatch.c:45:        // TODO: call RegisterBuffer
./interface/include/bharat/uapi/device/bharat_device_accelmgr_v2_dispatch.c:48:        // TODO: call UnregisterBuffer
./interface/include/bharat/uapi/device/bharat_device_accelmgr_v2_dispatch.c:51:        // TODO: call SubmitJob
./interface/include/bharat/uapi/device/bharat_device_accelmgr_v2_dispatch.c:54:        // TODO: call CancelJob
./interface/include/bharat/uapi/device/bharat_device_accelmgr_v2_dispatch.c:57:        // TODO: call QueryJob
./interface/include/bharat/uapi/device/bharat_device_accelmgr_v2_dispatch.c:60:        // TODO: call QueryFence
./interface/include/bharat/uapi/device/bharat_device_accelmgr_v2_dispatch.c:63:        // TODO: call QueryHealth
./interface/include/bharat/uapi/device/bharat_device_accelmgr_v2_dispatch.c:66:        // TODO: call SetThermalConstraint
./interface/include/bharat/uapi/device/bharat_device_accelmgr_v2_dispatch.c:69:        // TODO: call SetSafeMode
./interface/include/bharat/uapi/device/bharat_device_accelmgr_v2_dispatch.c:72:        // TODO: call GetTelemetrySnapshot
./tools/bidl/bidlc.py:124:            f.write(f"        // TODO: call {rpc['name']}\n")
./tools/check_arch_maturity.py:16:# TODO: Cross-check delivery/targets/target_matrix.json once target metadata is stable.
./generate_report.sh:10:grep -rnw --exclude-dir=.git --exclude="report.md" "TODO:" . >> "$REPORT_FILE"
```

## Stub Code
```
./gui_native_report.md:25:- The `gui_showcase` will need the actual BIDL client implementations mapped in place of the current weak IPC stub linkage inside the standalone native application once `P0-002` dynamic capabilities settle.
./docs/reviews/syscall-production-hardening-review.md:15:The Bharat-OS syscall layer has been converted from a scaffold/stub level into a production-grade foundation.
./docs/reviews/gap_analysis_merged_review_2026-04-22.md:80:- **G2 — Present as scaffold/stub:** API exists, core behavior partial.
./docs/reviews/kernel-production-readiness-audit-2026-08.md:37:selected production path is non-stub, fails closed, and has executable negative
./docs/reviews/kernel-production-readiness-audit-2026-08.md:86:stub traps, unsupported syscalls, scaffold MMU, unsupported SMP, and
./docs/reviews/kernel-production-readiness-audit-2026-08.md:220:| riscv32 | Partial boot, stub trap, unsupported syscall, scaffold MMU, no SMP; MMU-lite and MPU YAML exist. | Fundamental privilege, fault, isolation, user ABI, PMP, and reset qualification incomplete. | Complete RV32 privilege/trap/syscall, Sv32 MMU-lite, PMP MPU, timer/PLIC, reset, and QEMU/hardware validation. |
./docs/reviews/kernel-production-readiness-audit-2026-08.md:226:| MMU-full | Backend-neutral page-table and VM contracts exist for the 64-bit tier. | Prove map/protect/unmap atomicity, ASID/PCID lifecycle, W^X, user/kernel separation, TLB ACK-before-reuse, OOM rollback, huge-page alignment, COW/demand-fault races, and IOMMU independence. Never advertise IOMMU when the programming backend is a stub. |
./docs/reviews/code-quality-and-production-gap-analysis_2026-04-24.md:36:- `stub`: **277**
./docs/reviews/code-quality-and-production-gap-analysis_2026-04-24.md:210:- Replace init manifest stub-start in primary profile path.
./docs/reviews/gap_analysis/runtime_isa_extension_strategy.md:165:1. Implement non-stub feature probing in all 3 arch `cpu_caps.c` files.
./docs/reviews/gap_analysis/algorithm_improvement/memory_operations.md:33:### 3. Generic libc `memcpy` and `memset` (`core/kernel/src/lib/string.c`, `lib/posix/stub.c`)
./docs/reviews/gap_analysis/algorithm_improvement/memory_operations.md:35:*   **File:** `core/kernel/src/lib/string.c`, `lib/posix/stub.c`
./docs/reviews/gap_analysis/interrupt_controller_architecture_plan.md:27:> **Code-verified note (April 21, 2026):** This document was re-validated against the current tree and several statements below were adjusted from “implemented” to “partial/stub” where code is still placeholder (especially irq_domain and architecture controller depth).
./docs/reviews/scaffold_todo_and_arch_separation_review_2026-04-22.md:112:- `stub`
./docs/reviews/scaffold_todo_and_arch_separation_review_2026-04-22.md:129:- `rg -i -n "TODO|FIXME|XXX|HACK|TEMP|stub|placeholder|scaffold|compat|legacy|transitional|not implemented"`
./docs/reviews/scaffold_todo_and_arch_separation_review_2026-04-22.md:131:- `rg -l -i "stub|placeholder|scaffold|not implemented" | wc -l`
./docs/reviews/scaffold_todo_and_arch_separation_review_2026-04-22.md:138:- scaffold/stub/placeholder files: **364**
./docs/reviews/scaffold_todo_and_arch_separation_review_2026-04-22.md:274:- scaffold/stub,
./docs/reviews/scaffold_todo_and_arch_separation_review_2026-04-22.md:309:| F-002 | Storage stack | `core/stacks/storage/block/block.c` | Fake success | Unknown device path returned success and stub info, masking unsupported runtime path | core/stacks/storage | core/stacks/storage | IMPLEMENT_NOW | P0 | DONE |
./docs/reviews/scaffold_todo_and_arch_separation_review_2026-04-22.md:316:| F-009 | Service stubs enabled by path | `core/services/system/filesystem/*` | Scaffold runtime | VFS flow still stub-driven (`vfs_stub.c`) and demo request path | core/services/system | core/services/system + core/stacks/storage | IMPLEMENT_NOW | P1 | IN_PROGRESS |
./docs/reviews/kernel_algorithmic_foundations_code_mapping.md:36:Define `bh_rcu` headers and a functional stub (UP-safe/immediate).
./docs/dev/driver-contributor-rules.md:26:4. **Power Management**: At minimum, stub `suspend()` and `resume()` callbacks. Provide safe defaults upon resuming.
./docs/dev/ipc-contract-hardening.md:38:| `lib/ipc/include/bharat/ipc/ipc.h` | `bharat_ipc_send/recv/call` | lib | stub API | **Extend and implement** (add timeout-aware `_ex` variants). |
./docs/dev/ipc-contract-hardening.md:39:| `lib/ipc/src/ipc.c` | IPC API implementation | lib | stub-only | **Implement** using existing endpoint syscall path. |
./docs/dev/release-versioning.md:23:*   **Release Channel**: Reflects maturity (`stable`, `beta`, `experimental`, `stub`). *Rule:* `stub` and `experimental` components cannot be labeled `stable`.
./docs/architecture/services/init_architecture.md:24:This document describes the current baseline (stub) state and the fully developed future architecture for the multi-layered, profile-driven init service.
./docs/architecture/services/init_architecture.md:34:- `core/services/core/init/init_main.c` exists as a stub that initializes the runtime (`bharat_runtime_init()`), attempts to acquire a root capability (`bharat_runtime_get_bootstrap_cap()`), resolves a profile context (`init_profile_get_context()`), and runs a minimal status loop that suspends itself when done.
./docs/architecture/contracts/bidl-adoption-roadmap.md:181:  * List outputs: `.h` headers, opcode enums, struct definitions, dispatch skeleton, client stub, namesvc manifest.
./docs/architecture/contracts/bidl-runtime-mapping.md:105:// Generated client stub helper
./docs/architecture/SHAKTI_BHARAT_OS_IMPLEMENTATION_PLAN.md:118:- **Stage A (SHAKTI stub):** reset entry, stack, `.bss`, temporary traps, early UART, board discovery, boot metadata.
./docs/architecture/SHAKTI_BHARAT_OS_IMPLEMENTATION_PLAN.md:230:- I2C sensor stub;
./docs/architecture/core/isa-capability-dispatch-analysis-and-implementation-plan.md:200:- Non-empty, deterministic arm32 capability record; no stub implementation remains.
./docs/architecture/security/init_service_architecture_and_delivery_plan.md:17:`core/services/core/init` already follows the right **directional idea** (manifest filtering, profile gating, dependency-aware start order, and supervisor handoff hook), but it is still a scaffold-level bootstrap with stub launch functions and compile-time simulated context.
./docs/architecture/security/init_service_architecture_and_delivery_plan.md:268:| Tiny | compile-time static | strict static order | 0..1 attempts | fail-fast required only | quiescent/exit or idle stub |
./docs/architecture/network/edge-connectivity-profiles.md:21:2.  **Optional Protocol Stacks (`core/services/network/netstack/`):** Loaded per profile. Features include IPv4, IPv6, UDP, TCP, DHCP client, DNS stub, mDNS, CoAP, MQTT, BLE host subset, etc.
./docs/architecture/network/edge-connectivity-profiles.md:42:*   DHCP, DNS stub
./docs/architecture/network/edge-connectivity-profiles.md:110:*   **Protocols:** Ethernet, IPv4, ARP, ICMP, UDP, DHCP client, DNS stub, basic TCP.
./docs/architecture/console/console_architecture.md:152:4. At least one command path (`status`/`svc list`) proves end-to-end response through service backend (not local stub).
./docs/architecture/system/shell-architecture.md:105:1. Shell backend is service-IPC based (not local stub).
./docs/architecture/boot/boot_display_architecture.md:119:Firmware/board gives display info. The architecture-specific boot stub normalizes this into the canonical `boot_info_t` (specifically `boot_console_info_t`). The `kernel_main_common` flow (`boot_validate_all`, `boot_mode_resolve`) validates the structure and safely maps the framebuffer before creating the `CAP_DISPLAY_FB` capability and launching `boot_displayd`.
./docs/architecture/sdk-libc-plan.md:98:  * Logging/syslog compatibility stub or service
./docs/architecture/sdk-libc-plan.md:185:* [ ] **Task 5:** Create POSIX subset matrix and mark each API: native, shim, stub, deferred.
./docs/architecture/kernel-functionality-tiers-v1.md:47:4. Driver boundary model stub
./docs/architecture/uapi/native_abi.md:44:   - Kernel stub path available in `core/kernel/src/mm/mem_class.c`.
./docs/architecture/kernel/kernel-data-structures-and-algorithms.md:159:| `bh_rcu` | Needs hardening | `core/kernel/src/ds/` | `bh_rcu.h` | Capability Revocation | Lockless readers | Static | None (Stub) | Stub implementation only | Replace stub in KDS-RCU-001. |
./docs/architecture/kernel/kernel-data-structures-and-algorithms.md:216:* Replace `bh_rcu` stub with epoch-based RCU-lite (KDS-RCU-001).
./docs/architecture/kernel/kernel-data-structures-and-algorithms.md:233:- **KDS-RCU-001**: Replace `bh_rcu` stub with epoch-based RCU-lite.
./docs/architecture/kernel/process_thread_future_tasks.md:60:*   Update `core/kernel/src/personality/personality_default.c` with stub implementations.
./docs/architecture/kernel/status.md:41:    *   **Service Layer:** `core/services/file_system/main.c` exists but is just an empty stub with `TODO`s for POSIX semantics and block devices.
./docs/architecture/kernel/urpc/userspace-stub.md:16:Writing raw binary messages that conform to the `msg-wire-format-v1.md` specification is error-prone and tedious. To provide a type-safe, function-call-like experience for developers, Bharat-OS uses an Interface Definition Language (IDL) and a stub generator.
./docs/architecture/kernel/urpc/userspace-stub.md:52:-   **Marshalling:** Inside this function, the stub allocates a buffer (or uses a pre-allocated slab), writes the canonical header (setting `service_id` and `opcode`), and carefully packs the `path` and `flags` into the little-endian wire format.
./docs/architecture/kernel/urpc/userspace-stub.md:53:-   **Execution:** The stub calls `urpc_send()` (or blocks waiting for a reply).
./docs/architecture/kernel/urpc/userspace-stub.md:54:-   **Unmarshalling:** When the reply arrives, the stub unpacks the `status` and extracts the received capability (`file_stream`) into the user's provided pointer.
./docs/architecture/kernel/urpc/README.md:58:- [User-space Stubs](userspace-stub.md) - How IDL stubs are generated and linked.
./docs/architecture/profiles/profile_implementation_status.md:53:* **Details:** Aside from some basic ethernet hooks inside the automotive personality and a `ptp_clock.c` stub, a real, lightweight embedded TCP/UDP stack or zero-copy packet path is missing from the core/kernel/core.
./docs/architecture/profiles/profile_implementation_status.md:83:* **Missing for Production:** Genuine underlying CAN/LIN/Ethernet drivers (the `subsys_automotive_send_can_frame` merely returns true), IOMMU/VFIO protection for isolating untrusted devices (IOMMU is a stub), strong Crash Consistency.
./docs/architecture/personalities/roadmap.md:54:  - Dummy/stub implementations of specific Android properties (e.g., `/dev/__properties__`).
./docs/adr/ADR-007-experimental-scope.md:67:| 4        | Darwin/macOS (Mach/BSD)       | Experimental stub           |
./docs/adr/ADR-007-experimental-scope.md:68:| 5        | Windows NT                    | Experimental stub           |
./docs/adr/ADR-018-syscall-entry-and-bootstrap-handoff.md:9:The x86_64 `SYSCALL` stub called the two-argument common syscall gate with only
./core/services/faultmgr/main.c:25:        break; // break for stub to avoid infinite loop
./core/services/security/crypto/main.c:21:    return -1; // Keep it blocked/failing for the stub loop
./core/services/devmgr/main.c:26:        break; // break for stub to avoid infinite loop
./core/services/devmgr/README.md:19:- Implement root PCIe complex enumeration stub.
./core/services/system/diag/freeze_frame.c:1:// stub
./core/services/system/diag/debounce.c:16:// simple stub:
./core/services/system/filesystem/README.md:18:- `services/file_system/main.c` currently starts and calls an empty initialization stub (`vfs_init()`).
./core/services/can/candiag/diag_service.c:3:// diag stub
./core/services/can/canmgr/subscriptions.c:3:// subscriptions stub
./core/services/can/cangw/gateway.c:3:// gateway stub
./core/services/netstack/src/main.c:52:        // In the stub environment, break to avoid 100% CPU usage
./core/services/netstack/src/main.c:56:        break; // For compilation/stub execution
./core/services/sensor_hub/registry.c:1:// stub for sensor hub registry
./core/services/device/actuator_mgr/policy.c:1:// stub
./core/services/time_sync/quality.c:1:// stub
./core/services/coremgr/main.c:26:        break; // break for stub to avoid infinite loop
./core/services/memmgr/main.c:26:        break; // break for stub to avoid infinite loop
./core/services/netfast/src/main.c:10:    // Architectural stub - implementation pending
./core/services/netfast/README.md:13:* **Current:** Architectural stub (placeholder `main` only).
./core/services/storagemgr/main.c:25:        break; // break for stub to avoid infinite loop
./core/services/netmgr/src/ipc_auth.c:54:    // The actual system service logger or console_snprintf will replace this stub.
./core/services/netmgr/src/driver_health.c:86:    // State machine stub: Transition to RESTARTING and inc counter.
./core/stacks/can/core/can_route.c:3:// router stub
./core/stacks/ui/adapters/lvgl/src/lvgl_input_adapter.c:5:// Define a stub structure or use the actual API if the input manager is globally available.
./core/drivers/class/motor/pwm_motor_backend.c:1:// stub
./core/drivers/class/motor/qei_backend.c:1:// stub
./core/drivers/block/virtio_blk/virtio_blk.c:36:    // Provide some minimal fallback caps but indicate that we are a stub/scaffold
./core/drivers/display/virtio_gpu/virtio_gpu.c:7: * Overrides the weak hal_console_display_write() stub so that every
./core/drivers/display/virtio_gpu/virtio_gpu.c:479: * hal_console_display_write — strong override of the weak HAL stub.
./core/drivers/storage/nvme/nvme_core.c:56:    // Setup Admin Queue (stub logic)
./core/arch/arc/arc32/context_switch.c:26:    // This C stub represents the logical transition.
./core/arch/common/accel_caps_publish.c:5:// Weak stub for arch-specific publish logic
./core/arch/xtensa/xtensa32/README.md:5:- minimal boot stub
./core/arch/xtensa/xtensa32/README.md:7:- interrupt entry stub
./core/arch/hal/riscv/riscv64/dma.c:12:// We will stub cache operations out if the platform isn't Zicbom capable yet.
./core/arch/hal/riscv/riscv64/pmu.c:59:    // Other counters not programmed by default in this stub
./core/arch/hal/riscv/riscv64/iommu/riscv_iommu/riscv_iommu_stub.c:3:// Basic stub to allow compilation
./core/arch/hal/arm/arm32/hal_irq.c:7:    return 0; // Simple stub
./core/arch/hal/arm/arm64/pmu.c:5:// Simple ARMv8 PMUv3 abstraction stub
./core/arch/hal/arm/arm64/pmu.c:53:        // Assume enabled for stub
./core/arch/hal/arm/arm64/pmu.c:69:        // For stub, return cycle as approximation if real event not mapped
./core/arch/hal/arm/arm64/pmu.c:79:    // Other counters not programmed by default in this stub
./core/arch/hal/arm/arm64/iommu/arm_smmu/arm_smmu_stub.c:3:// Basic stub to allow compilation
./core/arch/hal/arm/arm64/hal_cpu.c:197:  // Explicit unsupported/no-op stub
./core/arch/hal/x86/x86_64/trap_entry.S:76:    # One alignment slot follows the C-defined raw frame before stub state.
./core/arch/hal/x86/x86_64/trap_entry.S:96:    # Hardware/stub state follows the C-defined raw frame.  CPL3 alone adds
./core/arch/hal/x86/x86_64/vtd_iommu.c:52:    // (Hardware programming omitted for stub)
./core/arch/hal/x86/x86_64/iommu/intel_vtd/intel_vtd_stub.c:3:// Basic stub to allow compilation
./core/arch/hal/x86/x86_64/hal_cpu.c:580:  // Assume APIC bus frequency is around 1GHz for simplicity in bare-metal stub
./core/platform/boards/avh-corstone310/board.c:7:    // Minimal stub for AVH Corstone-310 initialization.
./core/platform/boards/avh-corstone310/board.c:12:// Basic serial stub for early printk
./core/platform/boards/avh-corstone310/board.c:16:    // This is just a stub representation.
./core/hal/mmu_lite/common/mmu_lite_backend.c:48:    state->hardware_root_pt = NULL; // We'll stub this out for now
./core/hal/common/hal_pt.c:21:// This weak stub handles cases where no registration happens early.
./core/kernel/linker.ld:82:    /* ── Uninitialized data (zeroed by boot stub) ────────────────────── */
./core/kernel/src/ds/bh_rcu_stub.c:4:/* Baseline RCU stub implementation */
./core/kernel/src/ds/bh_rcu_stub.c:7:    /* Baseline stub: no-op in non-preemptive kernel */
./core/kernel/src/ds/bh_rcu_stub.c:11:    /* Baseline stub: no-op in non-preemptive kernel */
./core/kernel/src/ds/bh_rcu_stub.c:15:    /* Baseline stub: return constant epoch 1 */
./core/kernel/src/ds/bh_rcu_stub.c:20:    /* Baseline stub: synchronize is immediate in non-preemptive/UP environment */
./core/kernel/src/ds/bh_rcu_stub.c:24:    /* Baseline stub: invoke callback immediately for now */
./core/kernel/src/mm/pmm/pt_pool.c:35:// Hugepage promotion stub
./core/kernel/src/mm/pmm/numa_policy.c:63:// Scheduler hints (stub implementation)
./core/kernel/src/mm/pt/pt_pool.c:79:// Hugepage promotion stub
./core/kernel/src/mm/tlb/tlb_asid.c:5:// Basic ASID/PCID abstraction stub
./core/kernel/src/mm/iommu/iommu.c:17:// Explicit stub for external HAL inclusion of null backend
./core/kernel/src/mm/mem_class.c:56:    // For V1/stub we return a simulated handle.
./core/kernel/src/kernel_boot.c:436:// Forward declaration for user-space initialization stub
./core/kernel/src/ipc/mk_dispatch.c:9:// Simple first-pass authorization stub
./core/kernel/src/demo/bharat_demo.c:161:        DEMO_PRINT("  [SCHED] WARN: process_create() returned NULL (stub env).\n");
./core/kernel/src/demo/bharat_demo.c:162:        DEMO_PRINT("  [SCHED] Scheduler demo -- PARTIAL (stub mode)\n");
./core/kernel/src/demo/bharat_demo.c:178:        DEMO_PRINT("  [SCHED]   WARN: thread creation failed (stub).\n");
./core/kernel/src/demo/bharat_demo.c:190:        DEMO_PRINT("  [SCHED]   WARN: thread creation failed (stub).\n");
./core/kernel/src/demo/bharat_demo.c:220:                DEMO_PRINT("  [SCHED]   WARN: remote handoff returned error (expected in 1-core or stub env).\n");
./core/kernel/src/demo/bharat_demo.c:264:        DEMO_PRINT("  [IPC]  WARN: request pool exhausted (stub env).\n");
./core/kernel/src/demo/bharat_demo.c:325:/* framebuffer driver would write pixels here.  The stub just prints to serial. */
./core/kernel/src/demo/bharat_demo.c:328:    demo_section("FRAMEBUFFER OUTPUT (stub)");
./core/kernel/src/sched/sched_deg.c:4:// Define a minimal kmalloc/kfree stub if we don't have one, since this is a kernel
./core/kernel/src/sched_stub.c:312:    slot->process.addr_space = NULL; // Dummy for stub
./core/kernel/src/sched_stub.c:317:    // In stub environments hal_cpu_get_id may be tricky, just set to 0.
./core/kernel/src/sched_stub.c:384:    // __builtin_free(thread); // Dummy for stub
./core/kernel/src/sched_stub.c:646:            return 0; // Throttle core not implemented fully in stub
./core/kernel/linker_arm32.ld:72:    /* ── Uninitialized data (zeroed by boot stub) ────────────────────── */
./core/kernel/include/arch/arch_cpu_caps.h:40:// Fallback for unknown/stub
./core/kernel/include/security/crypto_caps.h:35: * we provide a stub interface.
./core/kernel/include/display/boot_gui_init.h:7: *   - When BHARAT_BOOT_GUI=0, boot_gui_run() is a no-op stub, so the linker
./core/kernel/include/hw_security.h:36:// Authenticate an instruction pointer (ARM PAC translation stub)
./core/kernel/include/sched/sched.h:665:// Multikernel IPC integration stub
./core/kernel/include/bharat/kernel/ds/bh_rcu.h:12: * The initial implementation is a "baseline stub" suitable for uniprocessor (UP)
./core/kernel/include/bharat/kernel/ds/bh_rcu.h:39: * For the baseline stub, this may return immediately if the kernel is non-preemptive.
./core/lib/bharatlibc/CMakeLists.txt:199:# 5. bharatlibc_crt (freestanding CRT initialization stub)
./core/lib/net/include/bharat/net/net.h:40:// Basic initialization contract (stub)
./core/lib/CMakeLists.txt:8:add_library(bharat_libcmin STATIC posix/stub.c)
./drivers/bus/pci/pci.c:70:                // Extremely simplified BAR parsing (assuming memory-mapped 32-bit for stub purposes)
./drivers/bus/pci/pci.c:115:    return -2; // Unsupported architecture stub
./ROADMAP.md:88:| Capability mediation claims | `netmgr` now uses fail-closed validation, but capability mediation is still not complete across all manager/dispatch paths. | Keep mediation at **Partial**, track removal of remaining permissive/stub checks, and block production claims until strict enforcement is end-to-end. |
./experience/user/apps/ai_governor/ai_governor.c:1:// ai_governor stub that doesn't use standard headers for cross compiling
./experience/user/apps/demo/bharat_demo.c:161:        DEMO_PRINT("  [SCHED] WARN: process_create() returned NULL (stub env).\n");
./experience/user/apps/demo/bharat_demo.c:162:        DEMO_PRINT("  [SCHED] Scheduler demo -- PARTIAL (stub mode)\n");
./experience/user/apps/demo/bharat_demo.c:178:        DEMO_PRINT("  [SCHED]   WARN: thread creation failed (stub).\n");
./experience/user/apps/demo/bharat_demo.c:190:        DEMO_PRINT("  [SCHED]   WARN: thread creation failed (stub).\n");
./experience/user/apps/demo/bharat_demo.c:220:                DEMO_PRINT("  [SCHED]   WARN: remote handoff returned error (expected in 1-core or stub env).\n");
./experience/user/apps/demo/bharat_demo.c:264:        DEMO_PRINT("  [IPC]  WARN: request pool exhausted (stub env).\n");
./experience/user/apps/demo/bharat_demo.c:325:/* framebuffer driver would write pixels here.  The stub just prints to serial. */
./experience/user/apps/demo/bharat_demo.c:328:    demo_section("FRAMEBUFFER OUTPUT (stub)");
./interface/include/bharat/uapi/device/bharat_device_accelmgr_v2_dispatch.c:1:// Dispatch stub
./tools/abi/check_sdk_symbols.py:16:    # This is a stub implementation that would invoke `nm -g <file>`
./tools/runners/runner_real_board.py:26:    print("[Real Board Runner] Deployment successful (stub).")
./tools/runners/runner_unicorn.py:31:    print("[Unicorn Runner] Harness execution successful (stub).")
./tools/runners/runner_renode.py:32:    print("[Renode Runner] Execution successful (stub).")
./tools/bidl/bidlc.py:114:        f.write("// Dispatch stub\n\n")
./tools/check_arch_maturity.py:21:FORBIDDEN_FOR_PRODUCTION = ["stub", "unsupported", "scaffold"]
./delivery/targets/arch_maturity.yaml:35:  trap: stub
./delivery/targets/arch_maturity.yaml:45:  trap: stub
./delivery/release/manifest/os-release.json:17:    "namesvc": { "version": "0.1.0", "iface": 1, "channel": "stub" },
./delivery/release/manifest/os-release.json:18:    "servicemgr": { "version": "0.1.0", "iface": 1, "channel": "stub" },
./delivery/release/manifest/os-release.json:22:    "file_system": { "version": "0.1.0", "iface": 1, "channel": "stub" }
```
