<!-- SPDX-License-Identifier: MIT -->
# Boot evidence contract v1

The ordered stages are `BOOT_KERNEL_ENTRY`, `BOOT_PLATFORM_READY`, `BOOT_MEMORY_READY`, `BOOT_SCHEDULER_READY`, `BOOT_INIT_MODULE_FOUND`, `BOOT_INIT_ELF_VALIDATED`, `BOOT_INIT_SEGMENTS_MAPPED`, `BOOT_INIT_STACK_READY`, `BOOT_INIT_THREAD_CREATED`, `BOOT_INIT_THREAD_ENQUEUED`, `BOOT_USER_ENTRY_PREPARED`, `BOOT_USER_ENTRY_ATTEMPTED`, `BOOT_USERSPACE_REACHED`, `BOOT_SERVICE_HANDOFF`, and `BOOT_COMPLETE`. Results are `NOT_OBSERVED`, `STARTED`, `SUCCEEDED`, `FAILED`, and `SKIPPED`.

`check_boot_evidence.py` recognizes only exact structured markers and narrow failure expressions. Absence yields `EVIDENCE_INCOMPLETE`, never an inferred root cause. Duplicate, malformed, and out-of-order structured markers are malformed evidence. Raw lines are truncated to 4096 bytes and 128 entries; input processing is capped at one million lines. Memory dumps and secrets are not collected by default.

Usage: `python3 tools/testing/check_boot_evidence.py --log serial.log --arch x86_64 --profile desktop_headless --json-out boot-evidence.json`. Exit codes are 0 satisfied, 1 positively observed failure, 2 malformed evidence, 3 required marker missing, 4 unsupported configuration, and 5 tool failure.
