# Agent Guardrails

Guardrails define hard boundaries and review gates for automated coding activity.

## A. Hard Stops (Never Do)

- Introduce or expose secrets.
- Bypass authn/authz checks in production paths.
- Disable audits, logs, or security checks silently.
- Delete data/schema without explicit instruction.
- Fabricate test results.

## B. High-Risk Change Categories

Require explicit human confirmation before merge:

- Authentication/authorization code
- Cryptography or key management
- Database migrations and data deletion
- Infrastructure/CI deployment pipelines
- Kernel/system-level memory or privilege boundaries

## C. Quality Gates

- Build or lint passes for touched scope.
- Tests for changed behavior.
- Updated docs for behavior, APIs, or workflows.
- Clear rollback strategy for risky changes.

## D. Review Checklist

- Is scope limited to requested task?
- Are assumptions documented?
- Are security implications addressed?
- Are performance impacts noted (if relevant)?
- Are commands and outcomes reproducible?

## E. Incident Handling

If an unsafe or uncertain condition is detected:

1. Stop automation.
2. Report exact risk and impacted files.
3. Propose safer alternatives.
4. Wait for human decision.
