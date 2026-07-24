# Console TTY Runtime

## Responsibility Boundary
The **Console Service** provides multiplexed text input/output (TTY) streams. It acts as the primary diagnostic and recovery interface for the system.

## Runtime Flow
1. **Initialization:** The Console starts early and claims the primary UART device.
2. **Display Lease:** It requests a display lease from the `display_broker` to enable the framebuffer text backend.
3. **Multiplexing:** Data written to the console via `WriteStream` is simultaneously sent to all active backends (UART and Framebuffer).
4. **Sessions:** Clients (like `shell`) open sessions to receive input events and send output.

## Capability/Right Model
Rights are governed by `TTY_RIGHT_*` constants:
- `READ`: Right to receive input from the console.
- `WRITE`: Right to send output to the console.
- `ATTACH`: Right to bind a shell or session to the TTY.

## Current Transitional Limitations
- Input is currently limited to UART. Framebuffer input (keyboard/pointer) is deferred to the `inputd` service.
- Font rendering is performed using a minimal built-in 8x16 bitmap font.
- Scrollback buffer is not yet implemented.

## Next Phase Work
1. **Input Routing:** Integrate with a dedicated input service for focus-based routing.
2. **Scrollback:** Implement a circular buffer for scrollback support on the framebuffer backend.
3. **ANSI Support:** Add support for basic ANSI escape codes for colored text and cursor positioning.
