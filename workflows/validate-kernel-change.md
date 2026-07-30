# /validate-kernel-change

Validate the current Bharat-OS change without modifying code unless a validation-only defect in the harness is explicitly in scope.

1. Read root `AGENTS.md` and load the `bharat-os-validation` skill.
2. Inspect the diff and classify affected subsystems, architectures, and MMU/MMU-Lite/MPU backends.
3. Run focused tests, linters, ABI checks when applicable, all five required builds, and the QEMU all-architecture gate.
4. Detect silent skips and missing targets/flags.
5. Produce the required completion table and mark the overall result PASS, FAIL, or BLOCKED.
6. Do not commit or push.
