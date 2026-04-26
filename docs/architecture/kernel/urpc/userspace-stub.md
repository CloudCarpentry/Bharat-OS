---
title: User-space Stubs
status: Proposed
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - kernel
see_also:
  - README.md
---
# User-space Stubs

## Overview
Writing raw binary messages that conform to the `msg-wire-format-v1.md` specification is error-prone and tedious. To provide a type-safe, function-call-like experience for developers, Bharat-OS uses an Interface Definition Language (IDL) and a stub generator.

## The Interface Definition Language (BIDL)
Bharat-OS defines its services using **BIDL** (Bharat Interface Definition Language), a declarative language similar to Protobuf or gRPC, but optimized for the capability-based microkernel environment.

### Example BIDL Definition (`vfs.bidl`)
```bidl
package bharat.vfs.v1;

service VfsServer {
    // A synchronous RPC call
    rpc OpenFile(request: OpenRequest) returns (response: OpenResponse);

    // An asynchronous one-way message
    oneway SyncAll(request: SyncRequest);
}

message OpenRequest {
    string<256> path;
    uint32 flags;
}

message OpenResponse {
    int32 status;
    // Capability to the opened file stream
    capability<Endpoint> file_stream;
}
```

## Stub Generation
A compiler tool (`bidlc`) parses the `.bidl` files and generates C (or Rust/C++) header and source files for both the client and the server.

### 1. Client Stubs
The generator creates a proxy function for the client:
`int bharat_vfs_v1_OpenFile(urpc_channel_t* chan, const char* path, uint32_t flags, bh_endpoint_t* out_file_stream);`

-   **Marshalling:** Inside this function, the stub allocates a buffer (or uses a pre-allocated slab), writes the canonical header (setting `service_id` and `opcode`), and carefully packs the `path` and `flags` into the little-endian wire format.
-   **Execution:** The stub calls `urpc_send()` (or blocks waiting for a reply).
-   **Unmarshalling:** When the reply arrives, the stub unpacks the `status` and extracts the received capability (`file_stream`) into the user's provided pointer.

### 2. Server Dispatcher
The generator creates a demultiplexer function for the server:
`void bharat_vfs_v1_dispatch(urpc_msg_t* msg);`

-   **Routing:** The dispatcher reads the `opcode` from the incoming message header.
-   **Unmarshalling:** It unpacks the payload into a C `struct`.
-   **Execution:** It calls the user's actual implementation function (e.g., `my_vfs_open_file(request_struct)`).
-   **Marshalling:** It takes the return values from the user's function, packs them into a response message, and sends it back over the uRPC channel.

## Memory Management in Stubs
To ensure deterministic execution (especially for RT profiles), the generated stubs avoid dynamic memory allocation (`malloc`) during message passing.
-   Bounded strings and arrays (`string<256>`) use statically sized buffers.
-   Variable-length data is passed out-of-line using Capwire descriptors to shared memory regions, which the user must explicitly provision beforehand.