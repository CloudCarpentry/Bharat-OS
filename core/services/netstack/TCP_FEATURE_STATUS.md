# TCP Feature Status Matrix

## Socket Lifecycle
- **Supported:**
  - Socket creation (`socket_create`)
  - Binding to an IP and Port (`socket_bind`)
  - Initializing TCP socket state variables (`tcp_state`, `tcp_seq`, `tcp_ack`, `tcp_window`).
- **Missing / Future Milestones:**
  - Connection Establishment (Three-Way Handshake)
  - Connection Teardown (FIN, CLOSE_WAIT, TIME_WAIT)

## TCP States
- **Supported:**
  - `TCP_STATE_CLOSED`: Supported for closed/invalid operations and rejecting TX attempts.
  - `TCP_STATE_ESTABLISHED`: Only state currently permitted for packet transmission (`tcp_tx`).
- **Missing / Future Milestones:**
  - `TCP_STATE_LISTEN`, `TCP_STATE_SYN_SENT`, `TCP_STATE_SYN_RECEIVED`, `TCP_STATE_FIN_WAIT_1`, `TCP_STATE_FIN_WAIT_2`, `TCP_STATE_CLOSE_WAIT`, `TCP_STATE_CLOSING`, `TCP_STATE_LAST_ACK`, `TCP_STATE_TIME_WAIT`.

## Transmission Behavior (TX)
- **Supported:**
  - Generating TCP header (Source Port, Destination Port, Seq, Ack, Window, PSH, ACK flags).
  - Checksum calculation (pseudo-header and TCP payload).
  - Explicit bounds checking and overflow protection for payload length.
  - Advancing `tcp_seq` solely upon successful IP layer transmission.
- **Missing / Future Milestones:**
  - Automatic Retransmission (timeouts and backoff).
  - Congestion Control (slow start, congestion avoidance, fast retransmit).
  - Window management (Dynamic updates based on flow control).

## Reception Behavior (RX)
- **Supported:**
  - Header validation and truncated packet rejection.
  - Checksum validation.
  - Socket lookup routing based on destination IP and port.
  - Extracting payload and invoking a callback (`rx_callback`).
- **Missing / Future Milestones:**
  - Out-of-order segment reassembly.
  - Processing duplicate ACKs and updating `tcp_window` dynamically.
  - Connection state tracking based on incoming flags (SYN/FIN/RST).
