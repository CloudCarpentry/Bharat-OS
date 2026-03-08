# Documentation Alignment Plan (Architecture + README + ADR)

This plan aligns project documentation with the current Bharat-OS implementation baseline while preserving roadmap clarity.

## 1. Validate current implementation claims

- Cross-check README and architecture summaries against:
  - current implementation review,
  - scheduler/memory/IPC architecture docs,
  - active ADR decisions.
- Remove or rephrase any language that implies production completeness where only baseline/scaffolding exists.

## 2. Update README for truthful status communication

- Add a concise status table that separates implemented baseline from deferred scope.
- Add **Device Profiles & Use-cases** grounded in present capability + known gaps.
- Add **AI Features & Roadmap** reflecting current kernel bridge/scheduler reality.
- Keep research references as architectural influences (not equivalence claims).

## 3. Expand architecture documentation

- Add `device-profiles-and-use-cases.md` to map architecture benefits across device classes.
- Add `ai-scheduler-status-and-roadmap.md` to capture implemented mechanism and roadmap.
- Link both docs from `docs/architecture/README.md` reading order.

## 4. Record documentation governance in ADRs

- Add ADR for status-label discipline and research-claim wording rules.
- Update ADR index pages to include the new decision.

## 5. Keep review artifacts consistent

- Ensure review documents and architecture docs use compatible status terminology:
  - implemented baseline,
  - deferred/roadmap,
  - experimental.

## 6. Validation checklist for this plan

- Markdown docs render cleanly.
- Cross-links from README and architecture index resolve.
- ADR index includes newly added ADR entries.
