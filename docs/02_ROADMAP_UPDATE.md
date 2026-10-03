# Proposed ROADMAP.md update — verification and hardware security track
Status: PROPOSED for developer branch; **not yet merged**. Insert into `ROADMAP.md` immediately after the maturity taxonomy; retain the existing phase tables and runtime priorities. Date: 2026-10-03.

## Phase 0V: Verification spine, concurrent with core stabilization (P0)
| ID | Deliverable | Dependency | Acceptance/evidence |
|---|---|---|---|
| VER-001 | Exact seL4-vs-Bharat capability semantic matrix; frozen definition of authority, delegation, revocation and linearization | Existing capability contract | ADR reviewed; non-equivalence and TCB listed |
| VER-002 | TLC-checked capability lifecycle model | VER-001 | No attenuation escalation/stale use/double commit in configured bounded state space; traceability to cap code |
| VER-003 | TLC-checked IPC capability transfer and queued-message model | VER-001 | lost/duplicate/delayed/failed receive and revoke races explored; negative C tests |
| VER-004 | TLC-checked shootdown request/ACK/failure model | Existing TLB contract | No premature frame reuse, ACK correlation, timeouts/poison semantics |
| SEC-001 | Audit cap validation and requester identity, remote mutation, transaction allocation failure | VER-001 | fail-closed tests; review of owner-only mutation and privileged PID 0 contract |
| SEC-002 | Security claim CI gate + evidence matrix | VER-002/003/004 | Each claim identifies config, tests/proof and limitation; no unsupported 'verified' claims |

## Phase 1S: Software baseline and actual runtime enforcement (P0/P1)
- Complete service-boundary authorization coverage and IPC transfer/revocation stress tests.
- Harden allocator (canary/redzone, quarantine, generations) without requiring optional hardware.
- Ensure ELF process/vm manager and service supervisor provide a genuine demo; keep existing baseline milestones as release blockers.
- Add CBMC/Frama-C bounded contracts for selected C paths where feasible; measure proof coverage, not just number of tools.

## Phase 2H: Optional hardware hardening (P2, no effect on unsupported architectures)
- HAL granular normalized MTE/CHERI-related *discovery* and opt-in enablement, with software baseline and target-specific unsupported return codes.
- ARM64 MTE isolated pilot with CPU/firmware checks and tagged allocator/fault tests.
- CHERI-RISC-V feasibility spike in a separate experimental toolchain/ABI image, explicitly not a generic riscv64 release requirement.
- Add DMA/IOMMU capability boundary tests before treating tagged pointers as device access controls.

## Phase 3T: Trusted lifecycle (P2/P3)
- Signed OTA manifest with algorithm/version/key-id, signature coverage, trust-anchor/rotation and anti-rollback; ML-DSA exploratory test vector first; legacy signature path retained by policy.
- Measured boot evidence accurate to platform trust chain, with explicit unsupported/not-measured/not-verified states.
- Confidential guest prototype target-by-target (SEV-SNP, TDX, CCA, CoVE), attestation and threat model; zero default 'TEE secure' claims.
- Rust for new *isolated services/drivers* only after ABI/error/lifetime contract; pilot `unsafe` inventory, no kernel rewrite.

## Deferred / forbidden shortcut
- Full kernel functional correctness certification, all-profile seL4-level parity, blanket CHERI or MTE, confidential host/hypervisor, universal PQ ROM boot are not P0 demo objectives.
- Reject silent software fallback with reported hardware protection, hardcoding trusted measurements, or moving domain policy into kernel/HAL.

## Release gates / owners (suggested)
G0: every new sensitive operation documented with owner, principal and capability; G1: TLC CI for three core models plus counterexample archive; G2: negative tests against implementation and all applicable target builds; G3: hardware enhancements only where probes, runtime tests and evidence are green; G4: architecture review ensures external user ABI freeze. Assign parallel agents: A cap semantics/model; B IPC model/tests; C TLB model/tests; D HAL/ISA feature inventory; E boot/update policy and evidence CI. Agents A-C may work independently until protocol contract freeze, then integrate.
