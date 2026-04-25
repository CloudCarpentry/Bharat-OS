# Crypto Boundary Architecture

## Kernel vs Service Responsibility Matrix

Bharat-OS implements a strict capability-oriented microkernel design. The kernel remains minimal and is responsible only for mechanisms, isolation, and hardware mediation. Cryptographic policies, algorithms, and key management are strictly delegated to an isolated user-space service domain (`core/services/security/crypto`).

### Kernel Responsibilities (Mechanism)

| Area | Why |
| :--- | :--- |
| **Capability Enforcement** | Controls who can invoke crypto endpoints, map sensitive memory, access hardware, or delegate rights. Matches the core authority model. |
| **IPC/URPC Substrate** | Provides the endpoint IPC and URPC transport for request/reply messaging. |
| **Secure Memory Mechanisms** | Enforces isolation (zero-on-free, non-dumpable, DMA-safe mappings) without semantic crypto knowledge. |
| **HAL Hooks for Hardware** | Low-level device init, MMIO mediation, IRQ completion, DMA/IOMMU mapping for crypto engines. |
| **Raw Entropy Collection** | Exposes raw entropy from hardware RNG/jitter sources to the service. |
| **Panic-Path Secret Hygiene** | Zeroes sensitive kernel buffers before reboot/panic if feasible for diagnostics. |

### Service Responsibilities (Policy & Algorithms)

| Area | Why |
| :--- | :--- |
| **Algorithms** | AES, SHA, HMAC, HKDF, ChaCha20, RSA, ECC. High-churn, bug-prone; easier to patch outside Ring-0. |
| **Key Lifecycle** | Generate, import, wrap, unwrap, rotate, destroy. Policy-heavy and belongs in a secure process space. |
| **Session/Context State** | Streaming hash/MAC state, AEAD nonce discipline. Service-level state machines. |
| **Key Policy** | Semantic rights (sign, decrypt, derive, export-wrapped). |
| **Sealing/Storage** | Persist wrapped keys, sealed blobs. User-space security concern. |
| **Crash Dump Encryption** | Encrypt user-space dumps before persistence/export. |

## Guardrails

1. **No general-purpose crypto in the kernel**: Do not add AES, RSA, ECC, TLS, or X.509 stacks to Ring-0.
2. **Capability mediation only**: Do not bypass capability checks for "internal" calls.
3. **Hardware agnostic kernel API**: Hardware acceleration is optional; the kernel never dictates algorithm policy.
4. **Opaque key material**: Raw application keys must not be stored in kernel objects; they remain within the service.
