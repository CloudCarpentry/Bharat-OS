# Scoped Instructions: `interface/contracts/`

These rules supplement the root `AGENTS.md`.

- Contract files are authorities, not informal examples.
- Preserve stable numbering, field meaning, fixed-width layout, and compatibility unless an approved migration explicitly changes them.
- Native syscall authority is `abi/native_syscalls.json`; never create a parallel source of truth.
- Generated outputs are not hand-edited.
- Add schema, semantic, compatibility, generator, and negative-path tests for contract changes.
- Update `docs/architecture/CONTRACTS.md` and an ADR when external behavior changes.
