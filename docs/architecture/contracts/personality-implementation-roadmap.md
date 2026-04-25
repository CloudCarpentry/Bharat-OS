# Personality Implementation Roadmap

## Program Name
**Contract and Personality Unification Program**

## Program Goal
Make Bharat-OS enforce one complete chain:
**Personality → UAPI view → capability policy → IDL/service contract → SDK binding → app/runtime usage**

Personality is not an afterthought. It is the selector and filter over what contracts exist and how they are exposed.

---

## Epic 1 — Personality architecture and governance
### Ticket 1.1 — Add personality architecture ADR
**Status:** DONE
Create `docs/architecture/contracts/adr-002-personality-model.md`.

### Ticket 1.2 — Add personality contract architecture doc
**Status:** DONE
Create `docs/architecture/contracts/personality-contract-architecture.md`.

### Ticket 1.3 — Add personality vocabulary and naming
**Status:** DONE
Create `docs/architecture/contracts/personality-naming.md`. Replace "NONE" with "NATIVE".

---

## Epic 2 — Restructure repo around personality as a first-class layer
### Ticket 2.1 — Create real `core/personalities/` implementation roots
**Status:** IN PROGRESS
Ensure `core/personalities/common/`, `core/personalities/native/`, `core/personalities/compat/linux/`, `core/personalities/compat/android/`, `core/personalities/domain/automotive/` exist and become active.

### Ticket 2.2 — Move personality logic out of trap-local silos
**Status:** IN PROGRESS
Move personality-specific logic toward the dedicated `core/personalities/` layer, leaving only a narrow trap hook inside kernel.

### Ticket 2.3 — Add personality dispatch contract
**Status:** PENDING
Create `core/kernel/include/personality/personality_hooks.h` and `core/personalities/common/personality_registry.c`.

---

## Epic 3 — Make contracts personality-aware
### Ticket 3.1 — Extend contract model
**Status:** PENDING
Update the contract docs so every service/UAPI can declare available personalities, capability translation modes, etc.

### Ticket 3.2 — Add personality metadata to IDL
**Status:** PENDING
Every `.bidl` service should be able to declare visible personalities, wrappers, and native contract authority.

### Ticket 3.3 — Add personality visibility manifest
**Status:** PENDING
Create `interface/uapi/bharat/personality/personality_manifest.h`.

---

## Epic 4 — Native personality as the golden path
### Ticket 4.1 — Rename bootstrap native personality
**Status:** IN PROGRESS
Replace "NONE" with "NATIVE" in architecture and code/build surface.

### Ticket 4.2 — Define Bharat native SDK surface
**Status:** PENDING
Define `user/interface/sdk/native/` and `user/interface/sdk/include/bharat/interface/sdk/...`.

### Ticket 4.3 — Native-first input vertical slice
**Status:** PENDING
Add native input IDL, native SDK bindings, and keep `interface/uapi/bharat/input/input.h` as native ABI event format.

---

## Epic 5 — Compatibility personality framework
### Ticket 5.1 — Linux compatibility contract map
**Status:** PENDING
Create `docs/architecture/contracts/linux-compat-mapping.md`.

### Ticket 5.2 — Android compatibility contract map
**Status:** PENDING
Create `docs/architecture/contracts/android-compat-mapping.md`.

### Ticket 5.3 — Compatibility shim layer boundary
**Status:** PENDING
Create `core/personalities/compat/common/` and `lib/compat/`.

---

## Epic 6 — Domain personality framework
### Ticket 6.1 — Automotive personality contract
**Status:** PENDING
Create `core/personalities/domain/automotive/` and `docs/architecture/contracts/automotive-personality.md`.

### Ticket 6.2 — Safety/minimal native personality
**Status:** PENDING
Create `core/personalities/domain/safety_minimal/`.

---

## Epic 7 — Personality-aware build and boot
### Ticket 7.1 — Formalize personality selection in build system
**Status:** PENDING
Formalize personality enums and default mappings.

### Ticket 7.2 — Boot manifest includes personality
**Status:** PENDING
Add personality field to boot/runtime manifest.

### Ticket 7.3 — Service startup filtered by personality
**Status:** PENDING
Control service exposure per personality (e.g., native desktop vs safety).

---

## Epic 8 — Personality-aware SDK and app model
### Ticket 8.1 — Split SDK by personality facade
**Status:** PENDING
`user/interface/sdk/core/`, `user/interface/sdk/native/`, `user/interface/sdk/compat/linux/`, `user/interface/sdk/compat/android/`.

### Ticket 8.2 — Forbid apps from bypassing personality facade
**Status:** PENDING

### Ticket 8.3 — Add sample apps per personality
**Status:** PENDING
Native input demo, compat sample, automotive demo.

---

## Epic 9 — Testing
### Ticket 9.1 — Personality contract test matrix
**Status:** PENDING

### Ticket 9.2 — Native personality golden tests
**Status:** PENDING

### Ticket 9.3 — Compatibility conformance smoke tests
**Status:** PENDING
