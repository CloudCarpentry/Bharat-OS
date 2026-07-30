# Bharat-OS AI Agent Workflow

## Purpose

This guide explains how repository instructions are mapped across coding agents without creating multiple competing sources of truth.

## Canonical and adapter files

| Tool | File(s) | Role |
|---|---|---|
| Google Jules | `/AGENTS.md` | Jules automatically loads the root repository guidance. |
| OpenAI Codex | `/AGENTS.md` plus scoped `AGENTS.md` | Codex layers instructions from root toward the working directory. |
| Gemini CLI / Gemini Code Assist | `/GEMINI.md` | Thin pointer to the canonical rules plus Gemini-specific context. |
| Google Antigravity | `/.agents/AGENTS.md`, `/.agents/agents.md`, `/.agents/skills/`, `/.agents/workflows/` | Persistent rules, roles, on-demand skills, and slash workflows. |
| GitHub Copilot | `/.github/copilot-instructions.md`, `/.github/instructions/*.instructions.md` | Repository-wide and path-specific guidance. |

The root `AGENTS.md` is the constitution. Tool adapters should summarize and link to it rather than copy the entire document.

## Repository installation

Copy the pack contents into the repository root while preserving hidden directories:

```bash
cp -a bharat-os-agent-governance-pack/. /path/to/Bharat-OS/
```

Review the diff:

```bash
git status --short
git diff -- AGENTS.md GEMINI.md .agents .github core/kernel/AGENTS.md interface/contracts/AGENTS.md tools/AGENTS.md docs/dev/AI_AGENT_WORKFLOW.md docs/architecture/CONTRACTS.md
```

Suggested commit:

```bash
git add AGENTS.md GEMINI.md .agents .github \
  core/kernel/AGENTS.md interface/contracts/AGENTS.md tools/AGENTS.md \
  docs/dev/AI_AGENT_WORKFLOW.md docs/architecture/CONTRACTS.md
git commit -m "docs: add cross-agent governance and validation rules"
```

Push only after review and only to the intended branch.

## Agent prompt pattern

Use task prompts that identify outcome, issue/ADR, scope, and evidence. Example:

> Implement the requested kernel change. Follow root and scoped AGENTS.md, preserve capability and per-core ownership invariants, support MMU/MMU-Lite/MPU, add regression tests, update affected contracts/docs, and run the mandatory validation gates. Report blocked targets honestly.

## Plan review checklist

Reject an agent plan that omits any applicable item:

- relevant contract/ADR review,
- ownership and lifecycle design,
- capability/trust-boundary analysis,
- MMU/MMU-Lite/MPU impact,
- focused tests,
- five required builds,
- QEMU all-architecture gate,
- documentation update,
- explicit rollback/failure behavior.

## Current tooling gap to resolve

At the time this pack was prepared, the `developer` branch's documented/current QEMU matrix used x86_64, arm64, riscv64, arm32, and riscv32 variants. The requested governance policy additionally requires `arm32_embedded_headless`, `powerpc_server_headless`, and a matrix-runner `--all-arch` flag.

Until those names and the flag exist in build/target truth:

- agents must attempt/inspect them,
- agents may run the current partial matrix for evidence,
- agents must mark the mandatory gate BLOCKED,
- agents must not substitute RISC-V 32 for PowerPC or rename a target in the report.

Recommended follow-up engineering task: add the missing target definitions and make the matrix runner fail when a required target is absent instead of silently skipping it.
