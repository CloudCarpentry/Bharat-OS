# Bharat-OS Documentation

Welcome to the Bharat-OS documentation directory. This folder contains all the reference material, design specs, build instructions, and developer guidelines for the project.

## Directory Structure

To help navigate the codebase and the documentation, we have organized the content into several focused categories:

### [Architecture (`docs/architecture/`)](architecture/)
Contains high-level system design documents, subcomponent architectures, kernel models, and network/storage strategies.
* [Kernel Architecture](architecture/000_KERNEL_ARCHITECTURE.md)
* [Process & Scheduler Architecture](architecture/core/process-scheduler-architecture.md)
* [Automotive Architecture](architecture/AUTOMOTIVE_ARCHITECTURE.md)
* [Boot Architecture](architecture/boot/BOOT_ARCHITECTURE.md)
* [Boot Selftest Policy](architecture/BOOT_SELFTEST_POLICY.md)
* [Network Architecture](architecture/network-architecture.md)

### [Build & Environment (`docs/build/`)](build/)
Guides for setting up your environment, building, running, and testing Bharat-OS.
* [Environment Preparation](build/ENV_PREP.md)
* [Build Guide: ARM64](build/BUILD_ARM64.md)
* [Build Guide: RISC-V 64](build/BUILD_RISCV64.md)
* [Build Guide: x86_64](build/BUILD_X86_64.md)
* [Host Build, Test, and Run Guide](build/HOST_BUILD_TEST_RUN_GUIDE.md)

### [Developer Guidelines (`docs/dev/`)](dev/)
Contains best practices, status trackers, and implementation plans.
* [Current Code Status](dev/current-code-status.md) - *The source of truth for implementation status vs. architecture plans.*
* [Developer Guidelines](dev/developer_guidelines.md)
* [Shell Contributor Guide](dev/shell-contributor-guide.md)
* [Release Versioning](dev/release-versioning.md)

### [Research & References (`docs/research_doc/`)](research_doc/)
Papers, external research, and bibliographies that inform the design of Bharat-OS.
* [Papers](research_doc/papers.md)
* [References (BibTeX)](research_doc/references.bib)

### Other Key Directories
* **[ADRs (`docs/adr/`)](adr/)**: Architectural Decision Records detailing major design choices over time.
* **[AI Agents (`docs/ai-agents/`)](ai-agents/)**: Instructions, rules, and boundaries for AI agents interacting with this repository.
* **[Boards (`docs/boards/`)](boards/)**: Board-specific technical documents and hardware support matrices.
* **[Profiles (`docs/profiles/`)](profiles/)**: Documentation defining different capability and hardware tiers.

---

*Note: The architecture documents in `docs/architecture/` are forward-looking. For the current, code-backed implementation reality, always refer to [`docs/dev/current-code-status.md`](dev/current-code-status.md).*
