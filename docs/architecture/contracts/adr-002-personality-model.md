# ADR 002: Personality Model Architecture

## 1. Status
**Accepted**

## 2. Context
Bharat-OS currently utilizes a "profile" (e.g., DESKTOP, EDGE) and a "personality" (e.g., NONE, LINUX, ANDROID). While profiles effectively dictate the hardware/runtime optimization intent, the "personality" concept has historically been under-modeled. It existed merely as a build flag, a directory structure, and a default trap implementation, without enforcing architectural boundaries.

To support multiple external execution models without bloating the minimal kernel, we must elevate "Personality" into a first-class architecture axis. The kernel must remain mechanism-only; services provide policy; and personalities provide the interface shape and contract exposure.

## 3. Decision
We are adopting a rigid architectural rule for Bharat-OS:
**Kernel exports mechanisms. Services export policy. Personalities export interface shape.**

### 3.1 Personality Definition
A personality is the external interface, ABI, and runtime model exposed to applications. It controls the contract selector.

### 3.2 Profile vs. Personality
*   **Profile:** Hardware/runtime/system optimization intent (e.g., DESKTOP, EDGE, MOBILE, SAFETY, CLOUD).
*   **Personality:** External interface/ABI/runtime model (e.g., NATIVE, LINUX, ANDROID, AUTOMOTIVE).

### 3.3 Native-First Rule
**The NATIVE personality is the source of truth.**
All UAPI, IDL, capability models, and SDKs must be authored "native-first". Compatibility personalities (like LINUX or ANDROID) must adapt to and translate into native contracts. Bharat-OS will not bend its kernel or native services to natively execute foreign semantics.

### 3.4 Personality Classes
1.  **Native:** The default, capability-first, primary Bharat-OS model (`BHARAT_PERSONALITY_NATIVE`).
2.  **Compatibility:** Adapts foreign expectations into Bharat services (e.g., `BHARAT_PERSONALITY_LINUX`, `BHARAT_PERSONALITY_ANDROID`).
3.  **Domain:** Policy/interface profiles for special environments (e.g., `BHARAT_PERSONALITY_AUTOMOTIVE`).

## 4. Consequences
*   The previous `NONE` personality is renamed to `NATIVE` (`BHARAT_PERSONALITY_NATIVE`) to reflect its standing as the golden path.
*   Personality logic currently residing in kernel trap handlers (e.g., `core/kernel/src/personality/personality_default.c`) must be moved to a dedicated `core/personalities/` boundary, accessed only via registry/hooks.
*   UAPI, IDL, and SDK visibility will become personality-aware, filtering which syscalls and services are exposed.
*   The system architecture is redefined as: **Personality → UAPI view → capability policy → IDL/service contract → SDK binding → app/runtime usage**.
