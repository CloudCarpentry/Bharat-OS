---
title: Service-Level Security Architecture
status: Proposed
owner: Divyang Panchasara
version: 1.0
last_updated: 2024-05-15
tags: [security, crypto, user-space, service]
---

# Service-Level Security Architecture

In Bharat-OS, the complex algorithms, policies, and protocols related to cryptography and security are implemented outside the kernel in dedicated user-space services. This enforces the microkernel philosophy, keeping the core kernel lean, reducing its attack surface, and allowing specialized configurations (e.g., automotive profiles versus general-purpose server profiles) to swap out entire policy layers without modifying the kernel.

These services represent **Layer 3: Crypto Services** and **Layer 4: Stack Integrations** in the Bharat-OS 4-Layer Security Architecture. They consume the raw primitives exposed by the kernel (Layer 2) and provide higher-level APIs to applications and other subsystems.

## Layer 3: Crypto Services (User-Space Daemons)

These daemon processes manage the complex cryptography logic, often bridging the gap between hardware capabilities and software requirements.

### Key Management Service (KMS)
The central authority for managing cryptographic keys throughout their lifecycle.
*   **Orchestration:** Managing key creation, rotation, retirement, and destruction based on defined policies.
*   **Usage Constraints:** Enforcing rules on how and when keys can be used (e.g., this key can only be used for signing, not encryption).
*   **Storage Integration:** Interfacing with the kernel's sealing primitives or external Hardware Security Modules (HSMs) for secure key storage.
*   **Auditing:** Logging key usage events for compliance and security monitoring.

### Certificate / Trust Manager
Responsible for managing the Public Key Infrastructure (PKI) components.
*   **Trust Anchors:** Maintaining the system's root certificates (CA bundles).
*   **Parsing:** Handling complex parsing logic for X.509 certificates, ASN.1 structures, and certificate revocation lists (CRLs).
*   **Validation:** Verifying certificate chains and signatures to establish trust.

### Crypto Provider Daemon
The primary interface for applications requiring cryptographic operations.
*   **Algorithm Framework:** Providing a unified API (e.g., a PKCS#11 interface or a custom IPC contract) for various algorithms.
*   **Software Fallback:** Implementing software versions of cryptographic algorithms that are not supported by the underlying hardware accelerators.
*   **Hardware Routing:** Multiplexing requests to the kernel's accelerator dispatch contract when hardware support is available.

### Attestation Service
Responsible for proving the system's identity and integrity to remote parties.
*   **Measurement Collection:** Gathering hardware measurements (e.g., TPM PCR values) via kernel interfaces.
*   **Quoting:** Requesting the hardware root of trust (e.g., the TPM) to cryptographically sign (quote) the measurements.
*   **Protocol Logic:** Implementing the specific attestation protocols required by remote verifiers (e.g., a cloud provider or an enterprise management system).

## Layer 4: Stack Integrations (Higher-Level Consumers)

These are standard operating system subsystems and services that heavily rely on the cryptography services provided by Layer 3.

### Storage Encryption
Securing data at rest.
*   **Full Disk / Volume / File Encryption:** Orchestrating the encryption and decryption of data blocks or files.
*   **Config Partition Sealing:** Protecting sensitive configuration data by binding its decryption to the platform's trusted state.

### Network Security
Securing data in transit.
*   **Protocols:** Implementing TLS, mTLS, DTLS, IPsec, or WireGuard-style secure communication channels.
*   **Integration:** Utilizing the Crypto Provider Daemon for the cryptographic operations required by these protocols.

### Update Framework
Ensuring the integrity and authenticity of system updates.
*   **Image Verification:** Verifying the digital signatures of update packages before installation.
*   **Rollback Protection:** Securely managing metadata to prevent the system from being downgraded to an older, potentially vulnerable version.

### Identity & Credentials
Managing the identities of entities within the system.
*   **Device Identity:** Establishing and utilizing the unique cryptographic identity of the device.
*   **Service Identity:** Providing credentials for services to authenticate with each other.
*   **Signing:** Providing APIs for applications to digitally sign data.

### Secrets for Services
Providing a mechanism for applications and services to securely store and retrieve sensitive information.
*   **Sealed Credentials:** Storing API keys, authentication tokens, or passwords securely, often leveraging the Key Management Service and kernel sealing primitives.
