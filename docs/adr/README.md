# ADR Guide and Index

This directory (`docs/adr`) is the canonical location for Architecture Decision Records (ADRs) in Bharat-OS. This page provides guidelines on how to write and update an ADR, followed by the index of accepted decisions.

If you have questions about the ADR process or need to propose a new one, please contact **Divyang Panchasara**.

## How to Write and Update an ADR

An ADR captures an important architectural decision made along with its context and consequences.

### 1. Naming and Creating a New ADR
- File name must follow the pattern `ADR-XXX-short-descriptive-title.md` (e.g., `ADR-014-new-subsystem.md`).
- Continue the numeric ordering from the highest numbered ADR currently in the folder.

### 2. Format / Template
Each ADR should include YAML frontmatter and follow the standard structure:

```markdown
---
title: ADR-XXX Short Title
status: Proposed | Accepted | Rejected | Superseded
owner: Your Name
reviewers: Reviewer Names
version: 1.0
last_updated: YYYY-MM-DD
tags: tag1, tag2
---

# ADR-XXX: Short Title

## Context
Describe the forces at play, the technological context, or the problem being solved. What is the current situation? Why is a decision necessary?

## Decision
What is the proposed change or decision? Write this in clear, assertive language.

## Consequences
What becomes easier or more difficult because of this change? Are there new risks or maintenance burdens? Are there performance or security implications?
```

### 3. Review and Update Process
- **Drafting:** Create a pull request adding the ADR with a status of `Proposed`.
- **Review:** Gather feedback from the team and stakeholders. Divyang Panchasara and other key reviewers should be tagged.
- **Acceptance:** Once consensus is reached, the status is changed to `Accepted` and the PR is merged.
- **Updates:** If an ADR becomes obsolete due to a newer decision, do not delete it. Instead, update its status to `Superseded` and link to the new ADR that replaces it.

---

## Accepted ADR Index

1. [ADR-001: Microkernel vs Hybrid](ADR-001-microkernel-vs-hybrid.md)
2. [ADR-002: Capability Model](ADR-002-capability-model.md)
3. [ADR-003: Multikernel Messaging](ADR-003-multikernel-messaging.md)
4. [ADR-004: Linux Personality First](ADR-004-linux-personality-first.md)
5. [ADR-005: ML Stays Out of Ring 0](ADR-005-ml-stays-out-of-ring-0.md)
6. [ADR-006: NUMA Awareness](ADR-006-numa-awareness.md)
7. [ADR-007: Experimental Scope](ADR-007-experimental-scope.md)
8. [ADR-008: Distributed VM Monitor and VM Spaces](008-distributed-vm-monitor-and-vm-spaces.md)
   *(Also: [ADR-008: AI Scheduler Plugin Contract](ADR-008-ai-scheduler-plugin-contract.md))*
9. [ADR-009: SDK Libc Strategy](009-sdk-libc-strategy.md)
   *(Also: [ADR-009: Documentation Status and Claims](ADR-009-documentation-status-and-claims.md))*
10. [ADR-010: Distributed Kernel Ownership](ADR-010-distributed-kernel-ownership.md)
11. [ADR-011: Structured Kernel Panic and Diagnostics](ADR-011-structured-kernel-panic-and-diagnostics.md)
12. [ADR-012: CAN Subsystem Architecture](ADR-012-can-subsystem-architecture.md)
    *(Also: [ADR-012: Interrupt Controller Evolution](ADR-012-interrupt-controller-evolution.md))*
13. [ADR-013: Multikernel Memory Protection Architecture](ADR-013-multikernel-memory-protection-architecture.md)

*(Note: Some ADR numbers overlap slightly from legacy tracking and are preserved here for historical context).*
