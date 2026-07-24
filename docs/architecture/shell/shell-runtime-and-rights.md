# Shell Runtime and Rights

## Responsibility Boundary
The **Shell Service** is the administrative command-line interface for Bharat-OS. It is a client of the Console service and an orchestrator of system diagnostic calls.

## Runtime Flow
1. **Attachment:** The Shell attaches to a Console TTY session.
2. **Input Loop:** It waits for input from the TTY, parses commands, and dispatches them to the backend.
3. **Honest Backend:** The Shell uses the `shell_backend_runtime` to make real IPC calls to system services (`servicemgr`, `devmgr`, etc.).

## Capability/Right Model
Access control is enforced at the command level via `BHARAT_SHELL_RIGHT_*`:
- `DIAG`: Allows read-only diagnostic commands (`svc list`, `mem stat`).
- `ADMIN`: Allows state-changing commands (`reboot`, service control).

## Current Transitional Limitations
- Commands return "unsupported" if the backing system service is not yet fully integrated.
- The shell does not yet support scripting or complex redirection.

## Next Phase Work
1. **Interactive Input:** Fully enable the input path from UART -> Console -> Shell.
2. **Service Integration:** Connect diagnostic commands to real `servicemgr` and `memmgr` IPC endpoints.
3. **Auth Integration:** Connect shell rights to the kernel's principal/token model.
