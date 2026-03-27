# Agent Specification (Cross-Platform)

Use this document as the shared baseline for any code agent operating in the repo.

## 1) Core Behavior

- **Strict Execution Directives**: Do not ask broad clarification questions when the requested change can be inferred from repo context, architecture docs, and existing code. Make a best-effort implementation.
- **Respect Layering**: Respect Bharat-OS layering: kernel for mechanisms only, services for policy, stacks for composed subsystems, arch for ISA-specific code, hal for contracts/glue, platform for board/machine integration.
- Prefer minimal, reversible changes.
- Read repository instructions before editing (e.g., `AGENTS.md`, contribution docs).
- Explain assumptions before risky operations.
- Keep diffs focused: do not mix unrelated refactors.

## 2) Planning & Execution

- Summarize task intent in plain language.
- Identify impacted files before editing.
- Run relevant checks after changes.
- Report exactly what was changed and why.

## 3) Editing Rules

- Preserve existing conventions, naming, and style.
- Do not remove comments or docs unless they are incorrect.
- Avoid hidden behavior changes.
- Update documentation when behavior changes.

## 4) Safety Rules

- Never commit secrets, tokens, private keys, or passwords.
- Do not disable security controls without explicit approval.
- Do not perform destructive operations unless requested.
- Escalate if requirements conflict.

## 5) Validation

- Any code change must include relevant host/e2e test updates and matching documentation updates.
- Never state that work is complete unless the relevant tests were executed and their outcomes are reported.
- Run the smallest meaningful test set first.
- If full tests are expensive, run targeted checks and state coverage limits.
- If checks cannot run, explain the environment limitation clearly.

## 6) Output Contract

Every final response should include:

- Summary of changes
- Files changed
- Commands run and outcomes
- Follow-up recommendations (if any)
