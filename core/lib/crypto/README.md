# Bharat-OS Cryptography Library

This library vendors [TF-PSA-Crypto](https://github.com/Mbed-TLS/TF-PSA-Crypto) to provide portable, independent userspace cryptography without relying on kernel primitives or host libraries.

## Provenance
- **Upstream**: https://github.com/Mbed-TLS/TF-PSA-Crypto
- **Version**: `v1.2.0` (pinned)
- **License**: Apache-2.0 or GPL-2.0
- **Modifications**: None to upstream sources. `bharat_crypto_config.h` restricts the build to a minimal required cryptographic profile (SHA-256, HMAC, HKDF, AES-GCM). Asymmetric algorithms, tests, and standard I/O dependencies are explicitly excluded. `#undef MBEDTLS_HAVE_ASM` is used to ensure portability across all cross-compilation architectures (preventing ARM64 inline asm pointer size failures in freestanding setups).

## Platform Integration
- **Entropy**: In `bharat_entropy.c`, `mbedtls_psa_external_get_random` fails closed with `PSA_ERROR_INSUFFICIENT_ENTROPY`.
- **Zeroize**: In `bharat_entropy.c`, `mbedtls_platform_zeroize` uses a simple `volatile` loop to ensure memory wipe without `explicit_bzero` dependency.
- **Memory**: In `bharat_memory.c`, `mbedtls_calloc` and `mbedtls_free` map to standard freestanding `malloc`/`free`.

## Cross-Architecture Matrix

| Architecture   | Profile         | Status     | Notes                                     |
|----------------|-----------------|------------|-------------------------------------------|
| x86_64         | GP / Native     | PASS       |                                           |
| arm64          | GP / Native     | PASS       | Required `#undef MBEDTLS_HAVE_ASM`        |
| riscv64        | GP / Native     | PASS       |                                           |
| arm32          | MMU-Lite        | PASS       | Provided `__aeabi_uidiv` for 32-bit arch  |
| riscv32        | MMU-Lite        | PASS       | Tested under riscv32-qemu-mmu-lite        |
