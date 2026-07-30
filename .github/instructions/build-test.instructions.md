---
applyTo: "tools/**,delivery/**,quality/**,CMakeLists.txt,**/CMakeLists.txt,BUILD.md,build.sh,build.ps1"
---

# Bharat-OS Build, Test, and Evidence Instructions

- Treat `tools/build.py`, `delivery/targets/`, and `BUILD.md` as build/runtime truth.
- Never invent a target name or report a skipped target as validated.
- Preserve deterministic out-of-tree generation under `build/`.
- Run applicable linters and focused tests before the full matrix.
- Required policy targets are listed in root `AGENTS.md`; missing targets are BLOCKED.
- Preferred QEMU gate is `python3 tools/run_qemu_matrix.py --headless --smoke --all-arch`.
- If `--all-arch` is unsupported, run the current partial matrix for evidence but report the required gate BLOCKED.
- A matrix runner that skips missing YAMLs is not proof that every required target passed.
- Update `BUILD.md` whenever target names, prerequisites, commands, boot markers, or runner behavior change.
