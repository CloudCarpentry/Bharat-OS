## Plan to solve issue #1
1. *Refactor kernel boot_events to provide real boot progress.*
   - Introduce `interface/include/bharat/uapi/boot/boot_events.h` and use it to replace hardcoded strings with structured progress updates.
   - Refactor `core/kernel/src/kernel_boot.c` to use `boot_events_publish` to provide a single structured source of progress/stage events.
   - Introduce `core/kernel/include/boot/boot_events.h` and `core/kernel/src/boot/boot_events.c` implementation that delegates to `boot_gui_update_progress` for now.
   - Verify the kernel compiles successfully by running `nirmaan build x86_64-generic-qemu`.

2. *Create log subscription API.*
   - Add new headers `interface/include/bharat/uapi/console/console_log.h`.
   - Update `core/services/system/console/main.c` to handle the new `BH_CONSOLE_OP_READ_RECORDS` and `BH_CONSOLE_OP_SUBSCRIBE` opcodes as stubs returning OK (since actual implementation requires routing via IPC to kernel memlog ring, which we will just stub in userspace to build the initial API). Wait, to actually fetch logs, we need a kernel IPC call. We can add a simple stub implementation in `console` service for now that returns fake logs to unblock the UI layer.
   - Verify the console service compiles successfully.

3. *Run tests and Pre-commit steps*
   - Run tests using `nirmaan build x86_64-generic-qemu` to verify the build is stable.
   - Complete pre commit steps to ensure proper testing, verification, review, and reflection are done.

4. *Submit the change.*
   - Submit the git branch.
