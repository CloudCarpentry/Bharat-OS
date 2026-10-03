# Formal Methods Scope

This directory contains the TLA+ models and TLC configurations forming the Verification spine (Phase 0V) for Bharat-OS.

## Scope of Proofs
The models rely on a bounded finite state space (e.g., small core count, limited CSpace slots, bounded message queues, bounded epoch counters) and explicit fairness assumptions for transitions.

They establish properties of the machine-checked protocols under these stated assumptions, providing *machine-checked protocol evidence*. **They do not automatically prove the C implementation correct.**

## Contents
* `capability/CapLifecycle.tla`: Verifies delegation never increases authority, and revoked/stale capabilities cannot regain authorization.
* `ipc/EndpointTransfer.tla`: Verifies cross-core transactions have defined commit and rollback behaviors, and IPC transfer/revocation races have consistent outcomes.
* `tlb/Shootdown.tla`: Verifies memory cannot be reclaimed prematurely during TLB invalidation.