# Bharat-OS: verification-first security architecture review
Date: 2026-10-03 | Basis: sampled `CloudCarpentry/Bharat-OS` developer-branch source via GitHub connector, plus three supplied project documents. This is a focused source review, not a full repository audit or executed test result. Proposed work is NOT implemented.

## Executive decision
Do not replace the native capability model with seL4, rewrite the C kernel in Rust, or promote CHERI/TEE/PQC to release claims now. Move a machine-checked capability/IPC protocol into P0, publish an explicit seL4 semantic comparison, and harden currently implemented authorization and cross-core operations. Hardware-supported defenses are optional backends selected by truthful discovery and profile gates, never silent fallbacks masquerading as protection.

## Review evidence and gap register
| ID | Evidence actually observed | Review disposition | Proposed action |
|---|---|---|---|
| CAP-01 | `core/kernel/include/capability.h`: 32-bit slot/generation handles, 64-bit rights, `bh_cap_locator_t`, parent/child/sibling locators, `CAP_STATE_*`, `revocation_epoch`, `cap_validate_ex` | Substantial existing substrate, not a mere placeholder | Specify lifecycle transitions, derivation ownership, object identity, revocation linearization and generation wrap policy |
| CAP-02 | `core/kernel/src/cap/cap_dispatch.c`: type/rights/state/generation checking; `cap_validate_scope_internal` allows requester PID 0 | Existing validation, with privileged-requester contract requiring explicit proof | Audit who can assert PID 0; derive principal from trusted kernel context, not untrusted request fields; deny on unknown identity |
| CAP-03 | `core/kernel/src/cap/cap_cspace.c`: local attenuation/delegation and remote transaction logic | Partial distributed protocol | Model duplicate/lost/out-of-order delivery, transfer/revoke race, crash recovery, bounded pending transactions |
| CAP-04 | `core/kernel/src/cap/cap_revoke.c`: iterative descendants, epoch increment before broadcast, 10ms acknowledgement deadline, panic on timeout; sorted multi-table locks | Security-conscious design with concurrency questions | Verify single-owner mutation and lock ordering, even across remote tables; prove no stale invocation after revoke completion. Audit behavior if transaction allocation fails |
| IPC-01 | `core/kernel/src/ipc/endpoint_ipc.c`: endpoint generation, cap lookup, transfer attenuation and DELEGATE requirement | Real implementation | Model queued messages vs revoke and receive, transfer rollback, endpoint reuse, authorization at dispatch |
| TLB-01 | `core/kernel/src/mm/tlb/tlb_shootdown.c`: pending request, ack/retry/deadline, diagnostics, isolation option; legacy mailbox fallback for some profiles | Older report calling this simply 'unbounded' is stale | Model mapping visibility and reclamation only after confirmed shootdown; failure/poison policy; disallow unsafe fallback for verified profiles |
| HW-01 | `core/hal/include/hal/hal_hw_caps.h` coarse flags; `hal_cpu_features.h` includes memory-tagging feature; KPRIM-002 and KHARDEN-001 propose fine-grained dispatch | Hardware safety proposal present, not a demonstrated MTE/CHERI implementation | Reconcile CPU feature discovery with normalized HAL capability contract and tested enablement paths |
| BOOT-01 | `core/arch/arm/arm64/hal_secure_boot.c` identifies placeholders; boot trust metadata exposed by HAL | Trust evidence is not proof of verified boot | Reject fabricated verified states; require platform roots, exact measured stages and negative boot-policy tests |
| FORMAL-01 | `docs/architecture/verification-scope.md` proposes proofs; `ROADMAP.md` lists Isabelle/HOL foundation in Phase 4 | Claim/evidence gap | Add executable TLA+ models and TLC CI to Phase 0, with scope and limitations clearly stated |
| RUNTIME-01 | developer README maturity matrix lists `process_mgr` and `vm_mgr` as SCAFFOLD, netmgr PARTIAL | Integration gaps remain | Do not let speculative advanced security delay service runtime demonstrability |

## seL4 reference: alignment, differentiation, non-claims
Align in **authority by possession**, typed object authority, rights attenuation, explicit kernel-mediated delegation, denial by default, object lifecycle discipline, IPC authorization, and small trusted mechanisms. Differences to document precisely:
1. Bharat-OS uses per-core-owned CSpaces and owner-validated cross-core uRPC; this is not seL4's exact kernel/object semantics. The distributed protocol requires new proofs for consistency, liveness and revoke completion.
2. Current flat fixed-size tables with derivation locators are not interchangeable with full seL4 CNode addressing and untyped retype/revocation semantics. Introduce explicit Untyped/Retype only if selected by a separate ABI decision.
3. Bharat-OS GP/RT/MIX and MMU_FULL/MMU_LITE/MPU profiles expand the configuration and assurance matrix. A proof of one profile does not prove the others.
4. Do not use 'seL4 verified', 'seL4 compatible', 'formally verified kernel', or 'memory-safe kernel' for Bharat-OS without corresponding evidence. A bounded TLA+ model is machine-checked protocol evidence, not a full C implementation proof.

## Formal methods ladder
**P0:** `quality/formal/capability/CapLifecycle.tla`, `quality/formal/ipc/EndpointTransfer.tla`, `quality/formal/tlb/Shootdown.tla`, TLC configs, reproducible toolchain and CI. Properties: authority cannot amplify; stale generation/epoch denied; no use after completed revoke; no double-commit; failed transfer leaves no live orphan; ACK only for matching request; no premature frame reuse after TLB change. Use bounded finite model and fairness assumptions stated in a README. Add C implementation-to-model traceability and negative/fault-injection tests.
**P1:** Alloy for bounded CSpace derivation shapes if valuable; CBMC/Frama-C or bounded C proof tools for current C paths, plus compiler sanitizers and static analysis. Kani/Verus/Creusot only for selected Rust components; Dafny/Lean if a concrete model demands them, not because a tool list sounds impressive.
**P2:** narrow functional refinement proof for authorization and object lifecycle after code and ABI stabilize. Audit proof assumptions, compiler/assembly, DMA, HAL, boot chain, and environmental trust boundary.

## Security architecture additions
- `interface/uapi/` contains stable user-visible authority and security status contracts. Do not export kernel pointers or raw CHERI architectural capability representations over generic uRPC.
- `core/arch/`: CPU/ISA probing, MTE/CHERI/TEE primitive implementations; `core/hal/`: normalized availability + enablement and error reporting; `core/platform/`: board roots, firmware and topology; `core/kernel/`: enforcement, map/tag state, revoke gates, confidential guest primitive handoff; `core/services/security/`: attestation policy, key lifecycle, OTA signature verification/orchestration; `core/drivers/`: device IOMMU/DMA mechanisms.
- Keep software capability authority distinct from CHERI architectural bounds/permissions/tagged pointers. A mapping between them requires an explicit ABI and object lifetime model. CHERI requires CHERI-aware compiler, ABI, loader, runtime, libraries, device/DMA boundary audit and supported board/emulator; normal RV64 builds remain unchanged.
- MTE requires supported ARM64 CPU, memory/page attributes, tag-aware allocator/free and exception path, DMA policy, CPU-local feature gating and tests. Unsupported builds use separately labeled software hardening, not 'MTE equivalent'.
- Confidential computing starts with **guest** integration and attestation reports, not a new TEE hypervisor. Separate AMD SEV-SNP / Intel TDX / Arm CCA / RISC-V CoVE into independent platform statuses: research, emulated, hardware-tested. A generic HAL does not imply portability of trust claims.
- PQC: begin with signed OTA image manifest and anti-rollback. ML-DSA is a standardized PQ signature scheme; LMS/XMSS are hash-based stateful options with major signing-state management. Do not claim standards-compliant firmware secure boot unless ROM/firmware trust roots actually verify the chosen chain. A hybrid classical/PQ manifest is a migration design, not a universal silicon capability.

## Five-architecture/profile assurance gates
| Target | P0 software authorization/model | Hardware phase |
|---|---|---|
| x86_64 | TLA+ protocol + host/QEMU negative tests | SEV-SNP/TDX guest prototype on suitable firmware/hardware |
| arm64 | Same C semantics and target tests | MTE opt-in prototype; Arm CCA separately gated |
| riscv64 | Same, Shakti/QEMU where supported | CHERI-RISC-V experimental ABI separate; CoVE exploratory |
| arm32 | Same applicable C authorization + MPU/MMU-lite tests | Software hardening, platform-specific boot root only |
| riscv32 | Same applicable C authorization + MPU/MMU-lite tests | Software hardening; do not assume 64-bit atomics/MMU |

## Release evidence labeling
Design only → compiles → host tests → boot/runtime integration → adversarial tests → model checked → implementation linked to model → hardware measured → independently audited. Every README feature claim must point to an artifact, target and configuration. Preserve executable OS demo gates; security roadmap is additive, not replacement.

## Reference reading (external, not evidence of Bharat-OS implementation)
- seL4: https://sel4.systems/ ; manual and verification summaries
- Microkit/LionsOS: https://github.com/seL4/microkit ; https://lionsos.org/
- CHERI: https://www.cl.cam.ac.uk/research/security/ctsrd/cheri/
- ARM MTE: https://docs.kernel.org/arch/arm64/memory-tagging-extension.html
- NIST ML-DSA FIPS 204: https://csrc.nist.gov/pubs/fips/204/final
- NIST stateful hash-based signatures SP 800-208: https://csrc.nist.gov/pubs/sp/800/208/final
