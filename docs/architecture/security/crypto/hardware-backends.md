---
title: Cryptographic Hardware Backends
status: Proposed
owner: Divyang Panchasara
last_updated: 2026-08-13
tags:
  - security
  - crypto
  - hardware
  - architecture
see_also:
  - README.md
version: 1.0
---
# Cryptographic Hardware Backends

Bharat-OS must support a diverse range of hardware security features across different architectures (x86_64, ARM64, RISC-V). To handle this complexity, the kernel defines three abstract backend classes that hardware drivers must implement. This ensures a stable internal API regardless of the underlying hardware.

CPU instruction backends are implementations of an operation, not standalone
algorithm APIs. The user-space runtime/service selects them through the
portable-first dispatch contract in
[`ADR-025`](../../../adr/ADR-025-portable-crypto-dispatch-boundary.md). A backend
is eligible only when the exact operation's usable capability is present; the
generic portable implementation remains the required fallback. Direct ISA
feature tests in TLS, storage, OTA, or application consumers are prohibited.

## Backend Classes

1.  `CPU_CRYPTO_ACCEL`: Represents inline ISA extensions for cryptographic operations (e.g., AES, SHA). These are fast, synchronous, and execute directly on the CPU core.
2.  `SECURE_ELEMENT_OR_TPM`: Represents a discrete hardware module (e.g., a TPM 2.0 chip, a dedicated HSM, or an isolated secure environment like TrustZone) that securely stores keys and performs cryptographic operations externally. These are typically asynchronous and slower than CPU accelerators.
3.  `RNG_PROVIDER`: Represents a hardware source of entropy (e.g., RDRAND, a dedicated TRNG on an SoC).

## Architecture Support Paths

The following outlines the specific hardware features and extensions that Bharat-OS aims to support on each architecture, mapped to the backend classes defined above.

### x86_64

*   **AES-NI (Advanced Encryption Standard New Instructions):** (`CPU_CRYPTO_ACCEL`) Instruction set extension for fast AES encryption/decryption.
*   **SHA Extensions:** (`CPU_CRYPTO_ACCEL`) Instruction set extension for hardware-accelerated SHA-1 and SHA-256 hashing.
*   **RDRAND/RDSEED:** (`RNG_PROVIDER`) Instructions for returning random numbers from an on-chip deterministic random bit generator (DRBG) seeded by a true random number generator (TRNG). The kernel must implement health-check policies to ensure the quality of the entropy.
*   **TPM 2.0 (Trusted Platform Module):** (`SECURE_ELEMENT_OR_TPM`) A standardized cryptoprocessor for secure storage, measurement, and attestation. The kernel provides the low-level communication (e.g., via the LPC or SPI bus), while user-space handles the complex TPM 2.0 command structures.
*   **Memory Encryption/TEE Hooks:** (Future) Support for technologies like AMD SEV (Secure Encrypted Virtualization) or Intel TDX (Trust Domain Extensions) to protect memory contents from the hypervisor or even the physical host.

### ARM64

*   **ARMv8 Crypto Extensions:** (`CPU_CRYPTO_ACCEL`) Optional instructions for accelerating AES, SHA1, SHA256, and GHASH operations.
*   **TrustZone / Secure Monitor Interaction Points:** (`SECURE_ELEMENT_OR_TPM` via SMC) The kernel provides the interface (SMC calls) to interact with the Secure World (TrustZone) for operations like key generation, secure storage, and specialized cryptographic functions implemented in a Trusted Execution Environment (TEE).
*   **RPMB (Replay Protected Memory Block) / Secure Storage Hooks:** Mechanisms to interface with secure storage often found on eMMC or UFS devices, frequently brokered through TrustZone.
*   **TPM or Discrete Secure Element:** (`SECURE_ELEMENT_OR_TPM`) Support for discrete TPM chips or other secure elements attached via standard buses (I2C, SPI) if present on the platform.

### RISC-V

*   **Scalar Crypto Extension:** (`CPU_CRYPTO_ACCEL`) Support for the standardized RISC-V scalar cryptographic extensions (e.g., Zkr for entropy, Zknd/Zkne for AES).
*   **SBI-Mediated Entropy / Security Hooks:** (`RNG_PROVIDER`, `SECURE_ELEMENT_OR_TPM` via SBI) Utilizing the Supervisor Binary Interface (SBI) to request entropy or perform security operations provided by the execution environment (e.g., firmware or a hypervisor).
*   **External Secure Element / TPM Integration Path:** (`SECURE_ELEMENT_OR_TPM`) Support for standard discrete TPMs or secure elements attached via SPI/I2C.
*   **Future Profile for DICE-like Measured Boot Flows:** Architectural support for Device Identifier Composition Engine (DICE) concepts, establishing a hardware root of trust and measured boot sequence specifically tailored for the RISC-V ecosystem.

## Dispatch and validation requirements

- Select independently for secure entropy, SHA-256/SHA-384, AES, and
  polynomial multiply; never infer one from a generic crypto bit.
- Use only capabilities marked usable after architecture detection and platform
  policy. System-wide dispatch must use the all-core capability set unless
  execution is pinned to a core with an equivalent owner-local guarantee.
- Keep architecture instruction bodies in `core/arch/`. HAL and runtime expose
  normalized contracts and selection; consumers remain architecture-neutral.
- Require known-answer tests and byte-for-byte differential tests against the
  portable implementation for every accelerated operation.
- On backend rejection or failure, return an explicit error or restart the
  complete operation with the portable backend before any output is published.
  Never mix partial outputs from different backends.
