# Foundational Research Papers

Bharat-OS draws heavy inspiration from several foundational operating systems and academic works. The following papers provide the canonical design guidance and architectural underpinnings for the OS.

| Inspiration | Paper Title | Authors / Key Venue | Link | Why Relevant to Bharat-OS |
| :--- | :--- | :--- | :--- | :--- |
| **seL4 (Capability-based verification)** | *seL4: Formal Verification of an OS Kernel* | G. Klein et al. (SOSP 2009) | [PDF](https://sigops.org/s/conferences/sosp/2009/papers/klein-sosp09.pdf) | Core influence for verification discipline and capability IPC; Bharat-OS mirrors seL4's minimal TCB and C implementation proofs. |
| **seL4 (Comprehensive verification)** | *Comprehensive Formal Verification of an OS Microkernel* | G. Klein et al. (ACM TOSEM 2014) | [PDF](https://trustworthy.systems/publications/nicta_full_text/7371.pdf) | Extends to full kernel proofs, guiding Bharat-OS's future verification efforts. |
| **Barrelfish (Multikernel model)** | *The Multikernel: A New OS Architecture for Scalable Multicore Systems* | A. Baumann et al. (SOSP 2009) | [PDF](https://sigops.org/s/conferences/sosp/2009/papers/baumann-sosp09.pdf) | Defines explicit message-passing for multicore scalability; directly informs Bharat-OS's URPC and state replication. |
| **L4 Family (Minimalism & separation)** | *L4 Microkernels: The Lessons from 20 Years of Research and Implementation* | G. Klein & J. Andronick (ACM Computing Surveys 2016) | [PDF](https://trustworthy.systems/publications/nicta_full_text/8988.pdf) | Surveys L4 evolution, emphasizing modularity; Bharat-OS builds on L4's IPC and driver isolation principles. |
| **AI-Driven Resource Management** | *Integrating Artificial Intelligence into Operating Systems: A Survey on Operating System Abstractions* | A. Korshun et al. (arXiv 2024) | [arXiv](https://arxiv.org/abs/2403.01185) | Surveys AI integration for scheduling/resource mgmt.; aligns with ADR-008's plugin contract for telemetry-driven optimizations. |
| **AI-Driven Resource Management** | *Enhancing Operating System Performance with AI: Optimized Scheduling and Resource Management* | S. S. et al. (IAEME 2025) | [PDF](https://iaeme.com/MasterAdmin/Journal_uploads/IJMET/VOLUME_11_ISSUE_12/IJMET_11_12_012.pdf) | Focuses on AI for efficiency/adaptability; relevant for Bharat-OS's hooks in power-aware and real-time scenarios. |

## Phase 4 Verification Roadmap

As part of Phase 4, we plan to integrate `seL4` tools for verification. Our initial focus will be on Isabelle/HOL proofs for our core IPC primitives.

We are actively seeking and welcome help from other developers on this roadmap. If you have experience in formal verification or theorem proving, please join us!
