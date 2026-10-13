# seL4 Semantic Delta

Bharat-OS shares principles with seL4 but diverges in implementation details:

## Alignments
* Authority by possession
* Typed object authority
* Rights attenuation
* Explicit kernel-mediated delegation
* Denial by default
* Object lifecycle discipline
* IPC authorization
* Small trusted mechanisms

## Divergences
1. **Per-core CSpaces**: Bharat-OS uses per-core-owned CSpaces and owner-validated cross-core uRPC. This requires new distributed proofs for consistency, liveness, and revoke completion.
2. **Fixed-size Tables vs CNode**: Bharat-OS uses flat fixed-size tables with derivation locators, not interchangeable with full seL4 CNode addressing or untyped retype/revocation semantics.
3. **Assurance Profiles**: Bharat-OS's GP/RT/MIX and MMU_FULL/MMU_LITE/MPU profiles expand the assurance matrix. A proof of one profile does not prove the others.
4. **Claims Boundary**: We do not claim 'seL4 verified' or 'memory-safe kernel' without matching evidence. A bounded TLA+ model checks protocol evidence, not the full C implementation.