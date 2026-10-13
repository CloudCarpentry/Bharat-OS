# ADR-VERIFY-001: Verification-first Security Architecture

## Status
Accepted

## Context
Bharat-OS requires strong security guarantees. The repository documents (`01_CODE_REVIEW_AND_SECURITY_ARCHITECTURE.md`) outline the strategic direction. Options included replacing the native capability model with seL4, rewriting the C kernel in Rust, or prematurely asserting hardware protections (CHERI/TEE/PQC).

## Decision
1. We will **not** replace the native capability model with seL4.
2. We will **not** rewrite the C kernel in Rust.
3. We will **not** promote hardware defenses to release claims without truthful discovery and profile gates.
4. We **will** move a machine-checked capability and IPC protocol into Phase 0, using TLA+ bounded finite models.
5. We **will** publish an explicit seL4 semantic comparison and harden existing C implementations.
6. Any hardware-supported defenses must act as optional backends, selected transparently, and never as silent fallbacks.

## Consequences
- Requires new TLC CI paths.
- Establishes a verifiable contract for capabilities, IPC transfers, and TLB shootdowns without halting immediate demonstrability of Bharat-OS.
- Non-equivalence with seL4 is documented and accepted.