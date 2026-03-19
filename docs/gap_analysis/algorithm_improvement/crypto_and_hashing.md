# Cryptography and Hashing Inefficiencies

This document tracks where cryptography or hashing logic currently operates in software and flags opportunities to substitute these with hardware offloading or hardware-accelerated CPU instructions.

## Subsystems

### 1. Crypto Service (`services/crypto/dispatch.c`, `crypto_service.h`)
The Crypto service is currently stubbed to handle requests in software via `handle_hash_init`, `handle_hash_update`, etc.
*   **File:** `services/crypto/dispatch.c`, `services/crypto/crypto_service.h`
*   **Context:** Handling crypto opcodes (`CRYPTO_OP_HASH_INIT`, `CRYPTO_OP_AEAD_SEAL`) in user-space.
*   **Improvement Suggestion:**
    *   **Hardware / Accelerator Offload:** Integrate hardware cryptography offloading (via `services/accelmgr` or direct device assignments). Replace generic software algorithms with CPU extensions where offload hardware is unavailable:
        *   **x86_64:** AES-NI for block ciphers, SHA extensions for hashing.
        *   **ARM64:** Cryptographic Extensions (CE) for AES, SHA-1, SHA-256.
        *   **RISC-V:** Scalar Cryptography Extensions (Zk*) for AES and hashing.
    *   **Algorithmic:** When relying on software crypto, make sure that side-channel resilient algorithms are implemented (e.g., constant time crypto libraries instead of naive implementations).

### 2. Physical Page Hashing (`kernel/src/mm/zswap.c`)
ZSwap compression maps rely on a trivial modulus hashing function.
*   **File:** `kernel/src/mm/zswap.c`
*   **Context:** `#define ZSWAP_HASH_TABLE_SIZE 1024` with `return key % ZSWAP_HASH_TABLE_SIZE;`.
*   **Improvement Suggestion:**
    *   **Algorithmic/Hardware:** The naive modulus hash creates an uneven distribution of collision buckets for physically contiguous pages. Replace this with a hardware-accelerated CRC32 hash (e.g., `crc32` instruction on x86_64 and ARM64, `crc32c` on RISC-V) for rapid $O(1)$ and evenly distributed page map lookups.

## Summary
To prevent CPU bottlenecks, the capability-mediated microkernel architecture expects cryptography and generic hashing to be heavily accelerated. Transitioning these features from software algorithms to architecture-native instructions or dedicated security enclaves (Hardware Security Modules, Accelerators) will significantly improve system throughput.
