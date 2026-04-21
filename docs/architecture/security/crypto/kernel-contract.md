---
title: Kernel Cryptographic Contract
status: Draft
owner: Divyang Panchasara
version: 1.0
last_updated: 2024-05-15
tags: [security, crypto, kernel, contract]
---

# Kernel Cryptographic Contract

This document strictly defines the boundary between what the Bharat-OS kernel implements directly regarding cryptography and security, and what it delegates to user-space services. The core philosophy is to keep cryptography mechanisms in the kernel only where the kernel must enforce trust, isolation, boot integrity, or hardware binding.

## Current code-backed status (2026-04-21)

The kernel now includes a concrete, test-backed baseline for:
* Provider registration and lookup.
* Capability-gated provider invocation.
* Convenience RNG dispatch to the first registered RNG backend.
* Key-buffer zeroization dispatch through provider hooks.

These pieces are implemented but not yet fully production-hardened for concurrency, formal capability object routing, and complete secure-element orchestration.

## What the Kernel Owns (Mechanisms)

The kernel is responsible for the absolute minimum required to securely boot the system, establish trust, protect key material in memory, and safely expose hardware cryptographic features to authorized user-space services.

*   **Secure Boot Measurement Handoff:** Receiving and verifying the chain of trust established by the bootloader (e.g., UEFI Secure Boot, U-Boot FIT).
*   **Crypto Capability Types and Access Control:** Formal capability types (`CAP_TYPE_CRYPTO_PROVIDER`, `CAP_TYPE_CRYPTO_KEY`, `CAP_TYPE_RNG`, `CAP_TYPE_SEALER`) regulating which processes can perform operations.
*   **Secure Memory Primitives:** Allocating memory regions that are guaranteed not to be swapped to disk, dumped in core files, or accessed by unauthorized DMA.
*   **Hardware RNG Exposure:** Safely exposing hardware entropy sources (e.g., RDRAND, TPM RNG, SoC TRNG) to user-space and kernel subsystems via a unified, capability-checked interface.
*   **Hardware Keyslot / Secure Element / TPM Handoff Mechanisms:** Providing the low-level communication channels (e.g., SPI, I2C, MMIO) to interact with dedicated security hardware, without implementing full protocol stacks (like TPM 2.0 command generation).
*   **Sealing/Unsealing Hooks Tied to Platform Identity:** Providing the primitive mechanisms to bind data (like a disk encryption key) to a specific platform state (e.g., a specific set of TPM PCR values), ensuring the data can only be decrypted if the system boots into the same trusted state.
*   **Page Protections for Key Material Buffers:** Enforcing strict memory access controls on buffers containing sensitive cryptographic material.
*   **Attestation Measurement Collection Interface:** Exposing interfaces for user-space to retrieve hardware-level measurements (e.g., reading TPM PCRs).
*   **Optional Low-Level Acceleration Dispatch Contract:** Providing a capability-checked dispatch mechanism for hardware cryptographic accelerators (e.g., AES-NI, ARMv8 Crypto Extensions, RISC-V scalar crypto). This is merely routing; the kernel does not implement software fallbacks.
*   **Zeroization Primitives:** Providing compiler-safe mechanisms (like `memset_explicit()`) to guarantee that sensitive memory is overwritten before it is freed or reallocated.
*   **Kernel Objects/Endpoints Representing Crypto Devices/Providers:** Creating formal, reference-counted kernel objects representing the hardware resources, managed by the capability system.

## What the Kernel Does NOT Own (Policies & Algorithms)

The kernel explicitly avoids implementing complex cryptographic algorithms, parsing complicated formats, or making policy decisions. These are the domain of Layer 3 Crypto Services.

*   **TLS (Transport Layer Security):** The kernel handles the network transport (TCP/IP), but the TLS protocol itself (handshake, record encryption) must be in user-space.
*   **IPsec/IKE Policy:** The kernel may implement the low-level ESP/AH encapsulation/decapsulation using hardware accelerators, but the complex IKE key exchange and policy management are strictly user-space.
*   **Certificate Parsing and Trust Stores:** The kernel must never parse X.509 certificates, ASN.1 structures, or manage lists of trusted Certificate Authorities.
*   **PKCS / X.509 Heavy Logic:** All public key infrastructure logic resides in user-space.
*   **General-Purpose Software Crypto Libraries:** The kernel does not contain a comprehensive software crypto library (like OpenSSL or mbedTLS) for general use. If an algorithm lacks hardware acceleration, it is executed in a user-space daemon.
*   **User/Business Policy for Key Rotation:** The logic dictating when a key must be rotated or retired is policy, not mechanism.
*   **Full HSM Orchestration Logic:** Complex management of Hardware Security Modules is handled by dedicated user-space services communicating with the HSM via kernel-provided channels.
*   **App-Facing Secret Management Workflows:** High-level APIs for applications to store passwords or tokens are built on top of the kernel's sealing primitives, but the workflows themselves are user-space.
