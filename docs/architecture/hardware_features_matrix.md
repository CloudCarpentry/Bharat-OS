# Hardware Feature Support Matrix

This matrix documents the discovery and mapping of raw hardware capability features into standard Bharat-OS abstractions across supported instruction set architectures (ISAs).

| Feature / Abstraction | x86_64 | ARM64 | ARM32 | RISC-V64 | RISC-V32 |
| :--- | :---: | :---: | :---: | :---: | :---: |
| SIMD / Vector Ops | AVX / AVX2 / FMA | ASIMD / SVE | NEON | V | - |
| AES / SHA / Crypto Extensions | AES / SHA / PCLMUL | AES / SHA1 / PMULL | Crypto Extensions | - | - |
| Atomic Operations | - | LSE | - | A | A |
| Cache-block Operations | - | - | - | - | - |
| Fast Memory / String Ops | ERMS | - | - | - | - |
| Address-translation / TLB | PCID / INVPCID | - | - | - | - |
| Branch Protection / Memory Tagging | - | - | - | - | - |
| Bit Manipulation | - | - | - | Zba, Zbb, Zbc, Zbs | - |

> Note: Hardware Support discovery must be deterministic and safe across heterogeneous architectures. The feature state is exported as an *intersection* (All CPUs) and *union* (Any CPUs) over all CPUs mapped correctly by the architecture HAL CPU discovery subsystem before the freeze phase. Backends dispatch on features verified by the kernel context, and fall back to scalar equivalents when appropriate features are absent or disabled by the security/execution policy context constraints (e.g. `EARLY_BOOT` or `IRQ_SAFE` execution blocks).

## Hardware Capabilities Publish Contract

Hardware capability discovery requires sequential publication and explicit freezing.

- **Publisher:** Only the Boot processor (BSP) is authorized to publish the primary hardware capabilities to the `g_caps_state` via `hal_hw_caps_publish_raw`, `hal_hw_caps_publish_cpu`, and `hal_hw_caps_finalize`.
- **Publication Phase:** Publication is strictly a *serial-boot-only* process.
- **Reader Access:** Readers may access the capability structure via `hal_get_internal_hw_caps()` **only after** the capabilities are frozen (i.e., `hal_hw_caps_is_frozen()` returns true). Any queries prior to the freeze will yield `NULL`.
- **Immutability:** The capability structure (`g_internal_hw_caps`) is frozen by updating the state atomically with `__ATOMIC_RELEASE` to enforce a memory barrier. Concurrent or subsequent modifications are explicitly prohibited, eliminating data races between the sequential initialization and all subsequent parallel core reads.
