---
name: bharat-os-syscall-abi
description: Apply when adding or changing syscalls, syscall metadata, UAPI arguments, capability requirements, usercopy phases, ABI lock, or generated dispatch artifacts.
---

# Bharat-OS Syscall ABI Skill

## Authority and rules

- Authority: `interface/contracts/abi/native_syscalls.json`.
- Compatibility lock: `interface/contracts/abi/native_syscalls.lock.json`.
- Generator/checker: `tools/abi/syscall_abi.py`.
- Generated files are outputs, not editing surfaces.

## Procedure

1. Define the ABI change in the manifest with fixed-width, stable argument semantics.
2. Specify capability source, object type, rights, scope, and validation phase.
3. For pointers, define direction and bounded size source.
4. Ensure fault-safe usercopy occurs before reading embedded capability fields.
5. Preserve number/symbol/argument compatibility unless the task explicitly authorizes a migration.
6. Generate only into the build directory.
7. Run:

```bash
python3 tools/abi/syscall_abi.py --check
```

8. Run generation through the normal build or explicitly with `--generate` only when validating the generator.
9. Add positive and negative tests: valid, wrong type, insufficient rights, wrong scope, stale/revoked, malformed pointer/size, usercopy fault.
10. Update `docs/architecture/CONTRACTS.md` and an ADR for externally observable changes.

Do not use raw numeric syscall invocations or hand-written duplicate tables.
