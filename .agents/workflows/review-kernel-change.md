# /review-kernel-change

Review a Bharat-OS patch for architecture, security, concurrency, lifecycle, testing, and documentation correctness.

1. Read root and scoped instructions.
2. Load `bharat-os-kernel-architecture` plus any applicable config, distributed-kernel, syscall-ABI, and validation skills.
3. Check ownership, pointer-free messaging, capability mediation, bounded deadlines/retries, lock/wait behavior, rollback/poisoning, backend coverage, naming, generated files, and layer placement.
4. Check tests against the actual failure modes.
5. Check documentation and maturity claims against evidence.
6. Report findings ordered by severity with file/line references and a safe remediation path.
7. Do not edit, commit, or push unless explicitly requested.
