---
title: uRPC Call Model
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - kernel
see_also:
  - README.md
---
# uRPC Call Model

## Overview
The uRPC (User-level Remote Procedure Call) system provides a flexible communication paradigm over its underlying lockless ring buffers. The call model dictates how the sender and receiver synchronize during a message exchange.

## 1. Synchronous Call / Reply (RPC)
This is the standard Request-Response paradigm, similar to a traditional function call.

-   **Mechanism:**
    1.  The Sender constructs a request message (with `MSG_FLAG_REQUEST`) containing a unique `request_id`.
    2.  The Sender places the message in the outgoing URPC ring buffer.
    3.  The Sender *blocks* (suspends execution) waiting for a response on its incoming ring buffer matching the `request_id`.
    4.  The Receiver processes the request and places a response message (with `MSG_FLAG_RESPONSE`) back in the ring buffer.
    5.  The Sender wakes up and retrieves the result.
-   **Use Case:** Reading a file from the VFS server, querying the network stack for an IP address.

## 2. Asynchronous Send (Future / Promise)
This model allows the sender to continue executing while the receiver processes the request.

-   **Mechanism:**
    1.  The Sender sends a request message with a unique `request_id`.
    2.  The Sender receives an immediate acknowledgment from the local URPC layer that the message was enqueued, but does *not* block.
    3.  The Sender continues working.
    4.  Later, the Sender checks its incoming ring buffer (polling) or registers an event handler (interrupt) to process the `MSG_FLAG_RESPONSE` when it arrives.
-   **Use Case:** Initiating a long-running DMA transfer, issuing a batch of non-blocking I/O requests.

## 3. One-Way (Datagram / Fire-and-Forget)
The fastest communication model, where the sender does not require any response from the receiver.

-   **Mechanism:**
    1.  The Sender places an event message (with `MSG_FLAG_EVENT`) in the ring buffer.
    2.  The Sender immediately continues execution. The receiver processes the event whenever it polls its queue.
-   **Use Case:** Telemetry logging to the AI Governor (`ai_sched_update_telemetry`), sensor data streaming (where losing an old frame is acceptable if the ring buffer fills up).

## The `urpc_channel_bind` Protocol
Before two endpoints can communicate via URPC, they must establish a channel.

1.  **Bind (`urpc_channel_bind`):** A client requests to connect to a named service (e.g., `bharat.vfs.v1`). The kernel routes this request to the service's listener endpoint.
2.  **Accept (`urpc_channel_accept`):** The service accepts the connection, allocating a dedicated URPC ring buffer pair (one for RX, one for TX) and granting the client a capability to its half of the ring.
3.  **Close (`urpc_channel_close`):** Either party can terminate the channel, revoking the capabilities and freeing the shared memory ring buffers.