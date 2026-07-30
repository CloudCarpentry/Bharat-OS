# Scoped Instructions: `tools/`

These rules supplement the root `AGENTS.md`.

- Tool output must be deterministic, machine-readable where applicable, and fail non-zero on validation failure.
- Do not silently skip required targets or convert missing evidence into success.
- Preserve build-directory isolation and do not generate into source directories.
- Validate CLI arguments and provide useful `--help` output.
- Keep Linux/WSL/macOS paths portable; preserve PowerShell parity when the tool has Windows wrappers.
- Build/test tools must print the exact target, command, result, and reason for skip/block.
- Update `BUILD.md` whenever CLI, target, or runner behavior changes.
