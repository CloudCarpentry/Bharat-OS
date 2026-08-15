---
title: Portable-First Crypto Dispatch and Kernel Boundary
status: Accepted
owner: Security Architecture Working Group
last_updated: 2026-08-13
tags:
  - adr
  - security
  - crypto
  - architecture
see_also:
  - ../architecture/security/crypto/overview.md
  - ../architecture/security/crypto/hardware-backends.md
  - ../architecture/security/crypto/roadmap.md
  - ../architecture/crypto-boundary.md
---
# ADR-025: Portable-first crypto dispatch and kernel boundary

## Context

Bharat-OS already discovers CPU cryptographic capabilities instead of assuming
that an instruction is safe to execute. That detection rule must scale beyond
the initial x86 AES and polynomial-multiply hooks without scattering ISA tests
through consumers or moving general-purpose cryptography into the kernel.

Cryptographic implementations also need a mandatory portable path. Hardware
features can be absent, disabled by platform policy, unavailable on a migrated
thread, or heterogeneous across cores. A reported feature is therefore not, by
itself, permission to issue the corresponding instruction.

## Decision

Audited user-space crypto libraries and services use one operation-aware
dispatch contract:

```text
Portable implementation
        |
        +-- x86 AES-NI / PCLMUL / SHA
        +-- ARM AES / PMULL / SHA
        `-- future RISC-V crypto extensions
```

The portable implementation is mandatory and is the default until an
architecture backend is both compiled in and selected from usable runtime
capabilities. Architecture implementations stay in `core/arch/`; normalized
feature discovery stays behind HAL/runtime contracts; crypto consumers do not
perform direct ISA probing. Selection is per operation because AES, polynomial
multiply, SHA, and entropy instructions are independent capabilities. A
backend that cannot satisfy the complete requested operation is ineligible;
dispatch falls back to the audited portable implementation rather than
composing an unreviewed partial result.

Accelerated and portable implementations must have identical results, error
semantics, bounds checks, constant-time requirements, and key-zeroization
behavior. Known-answer and differential tests are required before an
accelerated backend is enabled. Capability discovery is read-only after its
boot publication; dispatch tables contain no cross-core mutable policy.

Kernel crypto is limited to security-critical kernel mechanisms that cannot be
delegated, including entropy collection/conditioning needed by the kernel,
verified boot or image verification before user space is trusted, and narrowly
scoped integrity or secret-hygiene mechanisms. General-purpose algorithms,
high-volume TLS or data-plane crypto, key policy, certificate parsing, and
storage/OTA orchestration belong in an audited user-space library or service
with the same ISA dispatch architecture. Access to hardware providers and
kernel-held security objects remains capability-mediated.

If neither a validated portable implementation nor an eligible accelerated
implementation exists, the operation returns an explicit unsupported/error
result and produces no output. Placeholders, reversible test transforms, and
partially initialized outputs must never be exposed as cryptographic success.

Implementation priority is:

1. secure random generation;
2. SHA-256 and SHA-384;
3. AES-GCM;
4. HMAC and HKDF;
5. signature verification for boot and images;
6. TLS cryptography; and
7. disk and OTA integrity.

## Consequences

### Positive

- Every supported ISA retains a safe, testable baseline.
- x86, ARM, and future RISC-V acceleration share one consumer contract.
- Operation-specific eligibility prevents one crypto feature from being
  mistaken for support for a different primitive.
- High-volume and policy-rich cryptography stays outside the kernel TCB.

### Negative

- Each accelerated implementation requires portable differential tests and
  architecture-specific execution evidence.
- Heterogeneous systems may use the portable backend unless scheduling or
  system-wide capability guarantees make an accelerated backend safe.
- Boot-time crypto may require a small separately audited implementation when
  the user-space service is not yet available.

### Follow-up requirements

- Replace the current pilot transform hooks with typed operations that fail
  closed until audited primitives are connected.
- Add operation-specific capability queries for entropy, SHA-256/SHA-384,
  AES, and polynomial multiply.
- Add known-answer, portable-versus-accelerated differential, forced-fallback,
  unsupported-operation, and output-on-failure tests.
- Do not claim an algorithm or ISA backend as supported until those tests run
  on the relevant target.

