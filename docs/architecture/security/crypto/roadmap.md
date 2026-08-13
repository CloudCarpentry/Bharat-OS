---
title: Cryptography & Security Roadmap
status: Proposed
owner: Divyang Panchasara
last_updated: 2026-08-13
tags:
  - security
  - crypto
  - roadmap
see_also:
  - README.md
version: 1.0
---
# Cryptography & Security Roadmap

This roadmap outlines the phased implementation plan for the Bharat-OS Cryptography and Security subsystem. It is designed to establish a solid foundation in the kernel first (mechanisms), followed by the user-space services (policies and algorithms), and finally integration with the rest of the operating system.

## Delivery priority

Work proceeds in security dependency order:

1. **Secure random generation** — health-checked entropy providers and a
   portable DRBG/conditioning path before key-generating consumers.
2. **SHA-256 / SHA-384** — portable implementations first, followed by x86 SHA
   and ARM SHA dispatch with known-answer and differential tests.
3. **AES-GCM** — treat AES and polynomial multiply as independent eligibility
   requirements; provide portable authenticated encryption when either is
   unavailable.
4. **HMAC / HKDF** — build on the validated hash contract without duplicating
   ISA dispatch in the constructions.
5. **Boot/image signature verification** — keep only the minimal pre-userspace
   verification path in the trusted boot/kernel boundary; service-side image
   policy remains in user space.
6. **TLS cryptography** — integrate the audited user-space library/service; do
   not introduce a kernel TLS crypto stack.
7. **Disk/OTA integrity** — integrate through storage and update services after
   the primitive and signature contracts are validated.

Each stage must ship a portable implementation before optional ISA backends.
An accelerated path is incomplete until forced-fallback, unsupported-feature,
known-answer, and portable-versus-accelerated differential tests pass on its
target architecture. See
[`ADR-025`](../../../adr/ADR-025-portable-crypto-dispatch-boundary.md).

## Implementation snapshot (as of 2026-04-21)

The architecture remains target-state oriented, but the following items now have concrete kernel code:

* Provider registry and invocation entry points are implemented in `core/kernel/src/security/crypto_registry.c`.
* Capability-gated crypto stubs are implemented in `core/kernel/src/security/crypto_caps.c`.
* Secure memory zeroization and secure page wrappers are implemented in `core/kernel/src/crypto/secure_mem.c`.
* Kernel self-tests exist for provider registration, capability checks, unsupported op handling, RNG convenience API, and key zeroization dispatch in `core/kernel/src/quality/tests/security/test_crypto_registry.c`.

The largest remaining gaps are formal capability-object integration (beyond stubs), full secure-element backends, and service-level production wiring.

## Phase A: Contracts & Architecture (Current)

The goal of this phase is to establish the structural boundaries and the ABI contracts between the kernel and user-space services.

*   [x] Define the 4-Layer Security Architecture model.
*   [x] Document the boundary between kernel mechanisms and user-space policies.
*   [x] Define the internal kernel crypto capability types (`CAP_TYPE_CRYPTO_PROVIDER`, `CAP_TYPE_CRYPTO_KEY`, `CAP_TYPE_RNG`, `CAP_TYPE_SEALER`).
*   [x] Define the kernel-side crypto provider interface and the operations it must support (`CRYPTO_OP_GET_RANDOM`, `CRYPTO_OP_HASH_BUFFER`, etc.).
*   [x] Define the user-space (UAPI) contract for interacting with the kernel's crypto providers.

## Phase B: Kernel Primitives

This phase implements the core, low-level mechanisms within the kernel, strictly adhering to the contracts defined in Phase A.

*   [x] **Provider Registry:** Implement the central registry where hardware-specific drivers (accelerators, secure elements, RNGs) can register themselves.
*   [x] **Capability-Checked Invocation:** Implement the unified API for user-space to request cryptographic operations, enforcing the required capabilities before routing the request to the registered provider.
*   [x] **Zeroization Helpers:** Implement compiler-safe primitives (e.g., `memset_explicit()`) for clearing sensitive memory buffers.
*   [ ] **Secure Memory Allocation:** Implement memory allocation tags or classes to guarantee buffers are not swapped, dumped, or accessed by unauthorized DMA.
*   [x] **Entropy API:** Implement a unified API for exposing hardware RNGs to user-space and kernel subsystems.

## Phase C: Hardware Backends

This phase implements the drivers for the three abstract hardware backend classes defined in the architecture.

*   [ ] **CPU Accelerator Abstraction:** Implement operation-specific detection and portable-first routing for x86 AES-NI/PCLMUL/SHA, ARM AES/PMULL/SHA, and future RISC-V crypto extensions.
*   [ ] **TPM / Secure Element Abstraction:** Implement the low-level communication drivers (e.g., over SPI or LPC) for discrete security chips, focusing on exposing the device endpoint to user-space rather than implementing the full TPM 2.0 command set.
*   [ ] **Sealing Backend Abstraction:** Implement the specific mechanisms for tying cryptographic operations to the platform's trusted state (e.g., using TPM PCRs or TrustZone variables).

## Phase D: Services

This phase builds the user-space daemons that encapsulate complex cryptography logic, policies, and software fallback algorithms.

*   [ ] **Crypto Provider Service:** Implement the central daemon that provides a unified cryptographic API (like PKCS#11) to applications, managing software fallback when hardware acceleration is unavailable.
*   [ ] **Key Management Service (KMS):** Implement the service responsible for orchestrating key lifecycles, rotation policies, and usage constraints, leveraging the kernel's sealing primitives or external HSMs for secure storage.
*   [ ] **Attestation Service:** Implement the service responsible for collecting hardware measurements (e.g., TPM PCRs) and communicating with remote verifiers to prove the system's identity and integrity.

## Phase E: Stack Consumers

This phase integrates the cryptography subsystem with the rest of the Bharat-OS stack, utilizing the services built in Phase D.

*   [ ] **Secure Update Integration:** Implement image signature verification and rollback prevention using the Key Management Service and the Crypto Provider Service.
*   [ ] **Storage Encryption Integration:** Implement full disk or volume encryption, utilizing the KMS to securely seal the decryption keys to the platform's trusted state.
*   [ ] **Network Integration:** Implement standard secure communication protocols (TLS, mTLS, IPsec) leveraging the Crypto Provider Service.
