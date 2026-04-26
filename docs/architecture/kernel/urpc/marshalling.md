---
title: Bharat-OS Message Wire Format v1
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
# Bharat-OS Message Wire Format v1

## 1. Overview
Bharat-OS implements a **transport-neutral binary message ABI** as the control-plane communication foundation. It supports local IPC, URPC (multi-core), and future distributed-kernel coordination.

This protocol is:
- Cross-architecture safe (little-endian, deterministic alignment).
- Compact and explicit (no hidden padding).
- Transport-independent (usable over ring buffers, endpoints, or sockets).
- Capability-aware (supports remote delegation/revocation descriptors).
- Bounded (no unchecked dynamic allocations on decode).

---

## 2. Canonical Message Header

Every message on the wire begins with a fixed, unpadded header encoded in canonical little-endian format. The header provides parsing bounds before payload or capability arrays are read.

| Offset | Field Name       | Type     | Description |
|--------|------------------|----------|-------------|
| 0x00   | `magic`          | `u32`    | Must be `0x42485254` ('BHRT') |
| 0x04   | `version_major`  | `u8`     | Protocol major version (v1) |
| 0x05   | `version_minor`  | `u8`     | Protocol minor version |
| 0x06   | `header_len`     | `u16`    | Size of this header in bytes |
| 0x08   | `service_id`     | `u16`    | ID of the target service |
| 0x0A   | `opcode`         | `u16`    | ID of the method being called |
| 0x0C   | `flags`          | `u32`    | Bitmask of message properties |
| 0x10   | `total_len`      | `u32`    | Total message length including header, inline payload, and descriptors |
| 0x14   | `request_id`     | `u64`    | Opaque correlation ID for req/resp pairs |
| 0x1C   | `src_node`       | `u32`    | Origin node identifier |
| 0x20   | `dst_node`       | `u32`    | Destination node identifier |
| 0x24   | `cap_count`      | `u16`    | Number of capability descriptors trailing the payload |
| 0x26   | `desc_count`     | `u16`    | Number of OOL payload descriptors trailing the cap array |
| 0x28   | `header_crc`     | `u32`    | Optional CRC/Checksum over the header bytes (0 if unused) |

*Note: All multi-byte fields must be written to and read from the wire using explicit little-endian operations. Decoders must never overlay a C `struct` directly onto the transport buffer.*

### Validation Rules
- **Magic:** Must equal the defined protocol magic.
- **Major Version:** Must match the supported major version. Minor mismatches are permitted if extensions are ignored.
- **Header Length:** `header_len >= 0x2C` (44 bytes). If `header_len` is larger, the parser must skip the extra header bytes before starting the payload.
- **Total Length:** `total_len >= header_len`. Messages exceeding MTU must be rejected before service dispatch.
- **Counts:** `cap_count` and `desc_count` arrays must fit within `total_len`.

---

## 3. Flag Space

| Flag | Name | Semantics |
|------|------|-----------|
| `1 << 0` | `MSG_FLAG_REQUEST`     | Message is an RPC request |
| `1 << 1` | `MSG_FLAG_RESPONSE`    | Message is an RPC response |
| `1 << 2` | `MSG_FLAG_EVENT`       | Message is a one-way event/datagram |
| `1 << 3` | `MSG_FLAG_ERROR`       | Message payload represents an error |
| `1 << 4` | `MSG_FLAG_ACK_REQ`     | Requires transport-level ACK |
| `1 << 5` | `MSG_FLAG_RELIABLE`    | Sent over/requires a reliable transport |
| `1 << 6` | `MSG_FLAG_FRAGMENTED`  | Part of a fragmented stream (reserved) |
| `1 << 7` | `MSG_FLAG_HAS_CAPS`    | Contains capability descriptors |
| `1 << 8` | `MSG_FLAG_HAS_OOL`     | Contains Out-Of-Line descriptors |

---

## 4. Payload Encoding Rules

Payloads immediately follow the header (starting at byte `header_len`).

### Primitives
- **Integers:** `u8`, `u16`, `u32`, `u64`, `i8`, `i16`, `i32`, `i64` encoded in little-endian.
- **Booleans:** Encoded as a single `u8` byte (`1` for true, `0` for false).
- **Enums:** Encoded as explicit `u16` or `u32` integers.

### Bounded Complex Types
- **Strings (`string<N>`):** Encoded as a `u32` byte-length followed by the UTF-8 bytes. No null terminator is sent on the wire.
- **Bytes (`bytes<N>`):** Encoded as a `u32` byte-length followed by the opaque payload bytes.
- **Arrays (`array<T, N>`):** Fixed arrays encode `N` elements back-to-back. Variable vectors (`vec<T, N>`) encode a `u32` element count followed by the elements.
- **Optional Fields:** Supported via a presence flag (usually a `u8` boolean) preceding the field.

### Alignment
- There is **no implicit padding** in the payload. A `u8` followed by a `u64` occupies exactly 9 bytes.
- Generated decoders use byte-by-byte copies to extract fields, avoiding unaligned access traps on platforms like older ARMs.

---

## 5. Capability on Wire (Capwire)

A local capability table index (handle) is meaningless on a different kernel node. When a capability is sent, it must be exported into a **Capability Wire Descriptor**.

Cap descriptors are stored sequentially at the end of the inline payload.

### Cap Descriptor Layout

| Offset | Field          | Type   | Description |
|--------|----------------|--------|-------------|
| 0x00   | `cap_type`     | `u8`   | Type of the capability being transferred (e.g., Service, Shmem, Endpoint) |
| 0x01   | `transfer_mode`| `u8`   | COPY(0), MOVE(1), BORROW(2), DELEGATE_ATTENUATED(3) |
| 0x02   | `rights_mask`  | `u32`  | The access rights granted over the object |
| 0x06   | `origin_node`  | `u32`  | Node ID that hosts the real object |
| 0x0A   | `issuer_id`    | `u32`  | Security/Auth domain of the issuer |
| 0x0E   | `object_id`    | `u64`  | Global or exported ID of the backing object |
| 0x16   | `nonce`        | `u64`  | Cryptographic/Random nonce for replay protection |
| 0x1E   | `generation`   | `u32`  | Version tracking to aid revocation |

When received, the importing node verifies the descriptor and instantiates a proxy object (`OBJ_TYPE_CAP_PROXY`) in its local tables, retaining origin, object, and nonce information to satisfy later method invocations.

---

## 6. Out-of-Line (OOL) Payload Descriptors

To avoid memcpying large payloads (like network frames or camera buffers), OOL descriptors define memory regions that back the message.

| Offset | Field          | Type   | Description |
|--------|----------------|--------|-------------|
| 0x00   | `desc_type`    | `u8`   | SHMEM(0), DMA(1), BLOB(2), PACKET(3) |
| 0x01   | `flags`        | `u8`   | READ_ONLY, READ_WRITE, VOLATILE, etc. |
| 0x02   | `region_id`    | `u64`  | ID of the referenced shared memory region |
| 0x0A   | `offset`       | `u64`  | Start offset within the region |
| 0x12   | `length`       | `u64`  | Length of the referenced data chunk |

---

## 7. Versioning

- **Protocol Version:** Dictates the layout of the canonical header. Breaking changes require a `version_major` bump.
- **Service Version:** Contained within the `service_id` (e.g., `bharat.monitor.v1` is assigned ID `1`). Breaking schema changes to a service's payload require defining a new `service_id`. Minor backward-compatible field additions can occur if clients are tolerant of longer-than-expected payloads.