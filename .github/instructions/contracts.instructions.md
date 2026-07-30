---
applyTo: "interface/contracts/**,interface/include/**,interface/sdk/**,**/*syscall*,**/*uapi*"
---

# Bharat-OS Contract and ABI Instructions

- Treat `interface/contracts/abi/native_syscalls.json` as the native syscall authority.
- Preserve syscall numbers and semantics unless an approved migration explicitly changes them.
- Never add raw numeric syscall invocations or parallel hand-written dispatch tables.
- Do not hand-edit generated ABI outputs.
- Validate object type, rights, scope, generation/liveness, and capability source.
- Validate embedded capabilities only after fault-safe usercopy.
- Run `python3 tools/abi/syscall_abi.py --check`.
- Update `docs/architecture/CONTRACTS.md` and an ADR when the external contract changes.
