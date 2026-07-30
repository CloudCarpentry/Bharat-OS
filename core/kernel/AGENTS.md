# Scoped Instructions: `core/kernel/`

These rules supplement the root `AGENTS.md`.

- Kernel code contains mechanism only; move policy and orchestration to services.
- New public kernel APIs use `bh_*`; constants/macros/enums use `BH_*`.
- Every mutable object must state ownership and synchronization near its definition.
- Prefer core-local state. Cross-core work uses pointer-free, bounded, versioned message protocols with explicit completion.
- Never wait for remote completion while holding a kernel object lock or spinlock.
- Memory changes must support MMU, MMU-Lite, and MPU through the authoritative operations interface.
- Capability checks must validate type, rights, scope, generation/liveness, and owner as required.
- Add focused invariant and negative-path tests; run the root full validation gates.
