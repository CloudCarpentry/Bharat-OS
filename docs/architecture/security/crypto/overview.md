---
title: Cryptography & Security Subsystem Overview
status: Draft
owner: Divyang Panchasara
version: 1.0
last_updated: 2024-05-15
tags: [security, crypto, architecture]
---

# Cryptography & Security Subsystem

Bharat-OS enforces a strict boundary between the kernel's **trust-enforcing mechanisms** and the user-space **policy/algorithm services**.

The guiding rule is: **keep cryptography mechanisms in the kernel only where the kernel must enforce trust, isolation, boot integrity, or hardware binding; push algorithms, protocols, policy, and most key lifecycle work into services/stacks.** This aligns with the Bharat-OS direction of a small stable core kernel with profile-driven services/stacks layered on top.

## 4-Layer Security Architecture

Bharat-OS structures cryptography and security into four distinct layers:

### Layer 1: Boot Trust (Hardware & Bootloader)
*   **Measured Boot:** Recording the platform state into hardware registers (e.g., TPM PCRs) during startup.
*   **Secure Boot Handoff:** Verifying the kernel signature and passing trust roots from the bootloader to the kernel.
*   **Platform Identity Root:** The unalterable hardware identity (e.g., burned-in keys, DICE identity) establishing the device's true origin.

### Layer 2: Kernel Crypto Mechanisms (Core OS)
The kernel owns the absolute minimum required to safely multiplex hardware and enforce isolation:
*   **Capabilities & Access Control:** Regulating access to cryptographic hardware and keys via formal capability types.
*   **Provider Registry:** A consistent internal API for routing operations to hardware accelerators or secure elements.
*   **Secure Buffers:** Memory regions protected from user-space dumping, debugging, or swapping.
*   **RNG Contract:** Providing hardware-backed, cryptographically secure entropy to user-space and kernel subsystems.
*   **Sealing Hooks:** Tying cryptographic material to the current platform identity or boot state.
*   **Accelerator Abstraction:** Low-level dispatch for ISA extensions (AES-NI, ARMv8 Crypto).
*   *Note: The kernel explicitly does not own software fallback algorithms, PKI, or TLS.*

### Layer 3: Crypto Services (User-Space Daemons)
These services encapsulate the complex, policy-heavy cryptography logic:
*   **Key Manager:** Orchestrating key lifecycles, rotation, and usage constraints.
*   **Certificate / Trust Manager:** Maintaining CA bundles and parsing X.509 chains.
*   **Crypto Provider Daemon:** Implementing software fallback for algorithms not supported by hardware, providing a unified API to applications.
*   **Attestation Service:** Collecting evidence and quoting the platform state for remote verifiers.

### Layer 4: Stack Integrations (Higher-Level Consumers)
Standard subsystems that consume Layer 3 services (and occasionally Layer 2 capabilities):
*   **Storage Encryption:** Block/file-level encryption, volume sealing, config partition protection.
*   **Network Security:** TLS, mTLS, DTLS, IPsec, WireGuard implementations.
*   **Secure Update:** Image signature verification, rollback prevention metadata.
*   **Identity & Credentials:** Application signing, service-to-service authentication.
*   **App/Runtime APIs:** Developer-facing libraries for secure secret management.

## Document Directory
*   [Kernel Contract](kernel-contract.md) - Deep dive into what the kernel manages.
*   [Service Architecture](service-architecture.md) - Detailed breakdown of user-space crypto services.
*   [Hardware Backends](hardware-backends.md) - Supported ISAs, HSMs, and RNGs.
*   [Roadmap](roadmap.md) - Phased implementation plan.
