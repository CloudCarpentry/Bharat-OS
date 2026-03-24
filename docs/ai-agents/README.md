# AI Agent & Skill Documentation Kit

This folder provides a **portable documentation structure** for code agents and IDE assistants, so teams can keep one source of truth and adapt it across tools such as:

- OpenAI Codex-style agents
- GitHub Copilot
- Claude-based coding assistants
- Google/Jules-style coding agents
- IDE environments (VS Code, Antigravity, and similar)

## Goals

- Standardize how agents should behave in this repository.
- Define guardrails (safety, quality, review limits).
- Document reusable skills and workflows.
- Keep platform-specific notes separate from platform-agnostic standards.

## Folder Structure

```text
docs/ai-agents/
├── README.md
├── standards/
│   ├── AGENT.md
│   ├── SKILL.md
│   └── GUARDRAILS.md
├── platforms/
│   ├── codex.md
│   ├── github-copilot.md
│   ├── claude.md
│   └── google-jules.md
├── ide/
│   ├── vscode.md
│   └── antigravity.md
└── templates/
    ├── AGENTS.template.md
    └── SKILL.template.md
```

## Usage Model

1. Start from `standards/` for common cross-agent policy.
2. Add/update `platforms/` notes for each assistant runtime.
3. Add IDE-level behavior in `ide/` for extension and task integration.
4. Use files in `templates/` when creating a new `AGENTS.md` or `SKILL.md`.

## Recommended Rollout

- Place `AGENTS.md` in repository root for global behavior.
- Add nested `AGENTS.md` in subdirectories that need tighter rules.
- Keep each skill documented in one `SKILL.md` near related scripts/tools.
- Review guardrails monthly or after major incident/architecture changes.
