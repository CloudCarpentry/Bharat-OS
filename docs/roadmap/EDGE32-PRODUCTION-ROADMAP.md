# Bharat-OS EDGE32: Production Hardening and Realistic SoC Demonstrations

For ARM32 and RISC-V32, I recommend moving beyond basic boot and page-table tests to hardware-enforced memory protection, realistic QEMU boards, and actual userspace applications.

I inspected the current `developer` branch. Bharat-OS already contains MMU-Lite and MPU-only target configurations, ARM32/RV32 GUI profiles, EDGE32 memory tests, and basic userspace applications.

However, there is an important distinction between a target that boots, a profile that compiles, and a system that actually enforces isolation.

## 1. Critical findings from the current code

| Component                | Finding                                                                                                    | Priority |
| ------------------------ | ---------------------------------------------------------------------------------------------------------- | -------- |
| ARM32 MPU                | `arm32_mpu_activate()` only iterates software regions; its hardware programming calls are commented out    | P0       |
| RISC-V32 PMP             | MPU-only target exists, but actual PMP enforcement was not established by the reviewed code                | P0       |
| MMU-Lite                 | Common `prot_domain.c` backend exists; actual activation/isolation needs runtime testing                   | P0       |
| ARM32/RV32 edge GUI      | QEMU target YAMLs exist with VirtIO GPU/input devices                                                      | P1       |
| MPU-only target accuracy | ARM32 `cortex-a15` QEMU target is labeled MPU-only, but this does not prove Cortex-M-style MPU enforcement | P0       |
| Userspace applications   | `hello-bharat`, `bharat-info`, diagnostics and GUI showcase already exist                                  | P1       |

The most important finding is the hardware-model mismatch.

The current ARM32 MPU-only target requests `cortex-a15` on QEMU `virt`. A Cortex-A15 is a Cortex-A application processor with an MMU, not the Cortex-M MPU platform normally used for small embedded RT controllers.

Consequently, a successful boot of this target cannot certify an ARM Cortex-M MPU implementation.

Similarly, RISC-V Sv32 paging and RISC-V PMP are fundamentally different hardware protection mechanisms. The selected QEMU machine, CPU privilege levels, firmware and runtime must be appropriate for the protection model being tested.

## 2. Recommended architecture: two separate EDGE32 product families

EDGE32-A

MMU-Lite · HMI / gateways

ARM Cortex-A / RV32 Sv32

Userspace processes

Virtual address spaces

Small GUI or headless apps

GP / MIX scheduling

EDGE32-M

MPU / PMP · controllers

Cortex-M / RV32 with PMP

Static protection domains

Deterministic memory layout

UART / GPIO / sensor apps

RT scheduling

This separation lets Bharat-OS preserve its common kernel and capability contracts without pretending that every 32-bit CPU supports the same memory-management features.

For MMU-Lite, a lightweight GUI is feasible. For MPU/PMP controllers, I recommend a serial diagnostic interface and sensor/control applications first, with an optional framebuffer interface only on suitable boards.

## 3. Target board and QEMU validation matrix

I recommend these configurations for the next development cycle.

| Target                  | QEMU machine / CPU                           | Memory model                          | Demo                    |
| ----------------------- | -------------------------------------------- | ------------------------------------- | ----------------------- |
| ARM32 HMI               | `virt` / Cortex-A15                          | MMU-Lite                              | Dashboard GUI           |
| RV32 gateway            | `virt` / RV32 Sv32-capable CPU               | MMU-Lite                              | Sensor gateway          |
| ARM32 controller        | `mps2-an386` / Cortex-M4                     | MPU                                   | RT sensor/control loop  |
| ARM32 secure controller | `mps2-an505` / Cortex-M33                    | MPU and optional TrustZone capability | Isolation demonstration |
| RV32 controller         | PMP-capable RV32 machine/CPU, to be verified | PMP                                   | RT sensor/control loop  |

QEMU documents the MPS2 variants, including Cortex-M4 and Cortex-M33 configurations. Their peripherals and hardware behavior are machine-specific, which makes them better validation environments than pretending that QEMU `virt` represents an embedded MCU.

[QEMU documentation](https://www.qemu.org)

For the RISC-V PMP target, the agent must first identify a QEMU CPU/machine and firmware arrangement that actually exposes the required PMP CSRs and privilege transitions. It must not merely set `BHARAT_PROFILE_MPU_ONLY=ON` on a generic machine.

### Suggested QEMU validation levels

Level 1

Build and boot

Correct machine, architecture, image format, entry point, device tree and serial console.

Level 2

Real memory isolation

Execute forbidden accesses in the appropriate privilege context and verify the corresponding hardware exception.

Level 3

Userspace app and lifecycle

Start an application, use timer/UART or other devices, restart it, and confirm isolation and cleanup.

Level 4

Reliability and timing

Stress and fault-injection runs with measured memory, latency, resource exhaustion and recovery behavior.

A significant point: QEMU is useful for deterministic functional and privilege-isolation testing, but not for certifying real-time latency, physical interrupt timing, power consumption or hardware safety compliance. Those require a physical board and appropriate measurement equipment.

## 4. Four independent tasks for coding agents

| Priority | Task                                                         | Scope                                   | Agent   |
| -------- | ------------------------------------------------------------ | --------------------------------------- | ------- |
| P0       | BH-EDGE32-MPU-P0-002 — Real ARM32 MPU / RV32 PMP enforcement | HAL, protection activation, fault tests | Agent 1 |
| P0       | BH-EDGE32-BOARD-P0-003 — Realistic QEMU MCU/SoC profiles     | Board definitions, startup, runner, CI  | Agent 2 |
| P1       | BH-EDGE32-APP-P1-004 — Embedded reference applications       | Userspace sensor/control demos          | Agent 3 |
| P1       | BH-EDGE32-GUI-P1-005 — Lightweight HMI qualification         | MMU-Lite GUI, input, framebuffer        | Agent 4 |

The first task carries the main security risk. The others can proceed in parallel if their file ownership is kept separate.

### Agent 1: Hardware protection enforcement

# BH-EDGE32-MPU-P0-002 — Hardware-Enforced ARM32 MPU and RISC-V32 PMP

**Base branch:** developer
**Priority:** P0
**Primary goal:** Make MPU-only configurations genuinely enforce memory isolation.

## Phase 1 — Architecture audit

Review:

- `core/arch/mpu/arm32/mpu_backend.c`
- `core/kernel/src/mm/prot_domain.c`
- `core/kernel/include/mm/prot_domain.h`
- Canonical HAL MPU contracts
- ARM32 architecture privilege/trap implementation
- RISC-V32 trap and PMP support
- Profile selection and board capabilities

Determine the actual CPU families supported by the existing ARM32 implementation. Do not assume Cortex-A MPU and Cortex-M MPU mechanisms are interchangeable.

For RV32, distinguish PMP access control from Sv32 page translation.

## Phase 2 — Implement supported hardware backend

Implement actual protection programming, where the selected CPU supports it.

ARM Cortex-M path:

- MPU region descriptors and alignment
- Region size constraints
- Access permissions
- Execute-never permissions
- Privileged versus unprivileged access
- Background memory-map policy
- MPU activation/deactivation
- Context-switch protection-domain updates
- MPU fault handling

RISC-V32 path:

- PMP capability discovery
- PMP region capacity
- TOR/NAPOT/NA4 modes where supported
- PMP address and configuration registers
- M-mode versus S/U-mode access semantics
- Locked-region restrictions
- Context-switch domain programming
- Access-fault handling

Do not promise arbitrary revocation of permanently locked PMP entries.

## Phase 3 — Kernel integration

Preserve architecture → HAL → protection-domain layering.

Keep application capabilities separate from the hardware region programming mechanism.

Reject unsupported mappings and invalid access rights explicitly.

Require safe address-range validation, alignment checks and overflow protection.

Avoid allocating memory during protected context switches.

Do not implement an unrestricted userspace `mmap` interface for MPU-only targets.

## Phase 4 — Tests

Implement executable QEMU tests verifying:

1. Allowed application memory access.
2. Forbidden kernel memory read.
3. Forbidden kernel memory write.
4. Non-executable data region.
5. Unauthorized peripheral MMIO.
6. Isolation between two static application domains.
7. Region exhaustion.
8. Invalid alignment.
9. Invalid size.
10. Overflowed address range.
11. Context-switch isolation.
12. Interrupt entry and return correctness.
13. Repeated domain transitions.
14. Protection-fault reporting.
15. No successful protection activation when hardware programming failed.

Tests must generate actual CPU permission faults, not merely validate software metadata.

## Acceptance criteria

- No silent software-only MPU success.
- No unsupported PMP configuration advertised as protected.
- Application regions enforce CPU access restrictions.
- Kernel memory remains privileged.
- Faults are observable and correctly classified.
- Tests execute on suitable QEMU machines.
- Existing MMU-Full and MMU-Lite configurations are unaffected.

## Boundaries

Do not modify HMEM, ARM32/RV32 page-table backends, capability revocation, Name Service, Device Manager, crypto or Android personality.

Avoid changing generic scheduler or VM internals without a separate reviewed contract.

Document actual supported hardware and limitations.

Submit a focused PR with architecture test evidence and a memory-protection diagram.

**Important:** If existing ARM32 boot/trap code cannot execute on a Cortex-M machine, report that as a prerequisite rather than forcing Cortex-A startup code into the MCU backend.

### Agent 2: Realistic board qualification

# BH-EDGE32-BOARD-P0-003 — QEMU MCU/SoC Board Qualification

**Base branch:** developer
**Primary ownership:** `delivery/targets/`, `tools/targets/`, board-specific build/test scripts, documentation.

## Objective

Add realistic ARM32 and RV32 QEMU targets with accurate boot, device, CPU and protection-model configuration.

Do not simply clone existing YAMLs and rename the board.

## Required investigation

Verify support in the installed/pinned QEMU version for:

- ARM `mps2-an386`
- ARM `mps2-an505`
- ARM `virt` / Cortex-A15
- RISC-V `virt` with an explicitly selected RV32 CPU
- One suitable PMP-capable RV32 embedded SoC machine, if available

For each target document:

- CPU architecture and privilege levels
- RAM and ROM memory map
- Boot image format
- Exception vectors and startup requirements
- UART implementation
- Interrupt controller
- Timer
- GPIO/peripheral availability
- MMU/MPU/PMP availability
- Display availability
- Supported machine-specific limitations

## Implementation

1. Create or correct target definitions.
2. Implement board-specific startup only where supported by existing architecture interfaces.
3. Add board capability validation.
4. Reject inconsistent build combinations.
5. Implement serial boot markers and deterministic expected-output checks.
6. Add fault-injection hooks where feasible.
7. Add CI qualification jobs without modifying the generic QEMU runner unnecessarily.

## Crucial negative cases

A profile must reject:

- MPU-only on a CPU without the required MPU.
- Sv32 on a CPU without S-mode/page translation.
- Unsupported display devices.
- Unsupported privilege transitions.
- Memory configurations exceeding actual board memory.
- Invalid firmware/boot protocol combinations.

## Acceptance criteria

At least one ARM32 MMU-Lite target must boot and execute a userspace test.

At least one RV32 Sv32 target must boot and execute a userspace test.

For MCU/PMP targets, deliver a real boot and privilege test if current architecture support permits it; otherwise document exact missing prerequisites.

Do not substitute another CPU model and claim the target passed.

Report command lines, boot logs, QEMU version, feature detection and test results.

Keep this PR independent of the MPU/PMP implementation agent.

### Agent 3: Useful embedded applications

I recommend one reusable demonstration suite rather than many disconnected hello-world applications.

Sensor Gateway

Simulated temperature, humidity and device-health data with deterministic events.

RT Control Loop

Periodic control cycle, deadline monitoring and fault-safe state.

Device Diagnostics

CPU, memory profile, active protection mode and uptime through serial output.

HMI Dashboard

A lightweight visual dashboard for suitable MMU-Lite framebuffer targets.

# BH-EDGE32-APP-P1-004 — Embedded Userspace Demonstration Suite

Implement a portable suite of native Bharat-OS reference applications.

Target directories:

- `experience/apps/edge32_sensor_gateway/`
- `experience/apps/edge32_rt_control/`
- `experience/apps/edge32_diagnostics/`

Use existing SDK/libc and native services.

## Required features

Sensor gateway:

- Deterministic virtual sensor samples
- Periodic acquisition
- Bounded ring buffer
- Threshold-triggered alarms
- Serial diagnostic output
- No uncontrolled heap growth

RT controller:

- Periodic execution with a configurable period
- Expected-deadline tracking
- Deadline-miss counter
- Explicit safe-state transition
- No busy-spin unless justified by the target
- No direct privilege escalation

Diagnostics:

- Architecture and board information
- Declared and active memory-protection model
- Memory/resource usage where supported
- Task/service health
- Controlled error injection for development

## Tests

- Build on ARM32/RV32.
- Run MMU-Lite userspace demos in QEMU.
- Test embedded/static application execution separately.
- Confirm bounded memory consumption.
- Check repeated startup and shutdown.
- Validate error handling and deterministic serial output.

Unsupported SDK functions must fail explicitly. Do not substitute host-side application execution for a native guest test.

Do not modify generic kernel code or the ongoing protection-backend work.

Document which demos run on which target and why.

### Agent 4: Lightweight GUI for EDGE32

Do not start by porting a desktop compositor. The existing GUI target YAMLs already attempt VirtIO GPU and input devices, but this does not establish that the complete display pipeline works on ARM32/RV32.

The appropriate first milestone is a small framebuffer-based HMI, using the existing display contracts and LVGL integration where available.

# BH-EDGE32-GUI-P1-005 — Lightweight MMU-Lite HMI

**Scope:** ARM32/RV32 MMU-Lite GUI profiles and dedicated HMI application.

## Objective

Demonstrate a working native graphical application under Bharat-OS on at least one qualified 32-bit QEMU target.

## Requirements

- Reuse existing display and LVGL infrastructure.
- Small fixed-resolution framebuffer.
- Bounded memory consumption.
- Simple telemetry gauges.
- Device-health indicators.
- Alarm state.
- Keyboard or pointer interaction when supported.
- Graceful fallback to headless diagnostics.

## Validation

First verify framebuffer/GPU discovery and actual scanout operation on the selected QEMU board.

Do not assume that including `virtio-gpu-pci` in the YAML means a functioning display driver exists.

Then verify:

1. Display initialization.
2. Correct pixel output.
3. Repeated screen updates.
4. Input dispatch.
5. No framebuffer overflow.
6. App restart.
7. Memory-pressure behavior.
8. Screenshot-based regression where supported.

Collect guest-side logs and framebuffer evidence.

Avoid generic kernel display-handoff, HMEM and device-manager modifications while other agents own those components.

If the GUI stack has missing prerequisites, document them as blockers rather than claiming a headless boot proves GUI functionality.

Deliver a PR, memory/CPU footprint report and screenshots from a successfully running guest.

## 5. Recommended implementation sequence

Gate E0

Architecture and target truth

Confirm whether every YAML selects a supported CPU, protection backend and boot mode.

Gate E1

Real isolation

Verify MMU-Lite, Cortex-M MPU and RV32 PMP faults under actual guest execution.

Gate E2

Userspace workloads

Run sensor/control/diagnostic applications with lifecycle and fault recovery.

Gate E3

Display and robustness

Demonstrate the HMI where supported; measure reliability, footprint and performance.

My priority recommendation: Start Agent 2 (board qualification) immediately alongside Agent 1 (hardware enforcement). Agent 3 can build reusable applications in parallel; Agent 4 should first audit actual display readiness and avoid changing shared drivers until that work is stable.

This produces a much more credible path toward production-grade ARM32/RISC-V32 than simply increasing the number of QEMU targets that report `BOOT_PASS`.