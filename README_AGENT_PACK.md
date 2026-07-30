# Bharat-OS Agent Governance Pack

This pack provides repository-ready guidance for:

- Google Jules
- OpenAI Codex
- Google Antigravity
- Gemini CLI / Gemini Code Assist
- GitHub Copilot

## Why multiple files

`AGENTS.md` is the canonical policy. Other files are thin tool-specific adapters or scoped, on-demand skills. This avoids copy-paste drift while taking advantage of each tool's native instruction mechanism.

## Included structure

```text
AGENTS.md
GEMINI.md
.github/
  copilot-instructions.md
  instructions/
.agents/
  AGENTS.md
  agents.md
  skills/
  workflows/
core/kernel/AGENTS.md
interface/contracts/AGENTS.md
tools/AGENTS.md
docs/dev/AI_AGENT_WORKFLOW.md
docs/architecture/CONTRACTS.md
```

## Install

From the parent directory of this pack:

```bash
cp -a bharat-os-agent-governance-pack/. /path/to/Bharat-OS/
```

Review all files before committing. In particular, resolve the current repository gap for the requested PowerPC/ARM32 target names and `run_qemu_matrix.py --all-arch` support.
