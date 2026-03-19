# lib/packet (Packet Buffer Abstractions)

**Status:** Stub / Baseline Target

This library defines early packet abstractions and buffer descriptors.

## Scope
* Packet buffer descriptors (`packet_buf_t`).
* Buffer ownership tracking and lifetime comments.
* Headroom, tailroom fields, and total length.
* Flags for checksum and offload intents (future).

This library does *not* contain protocol parsers, routing logic, or socket implementations (see `lib/net` and `services/netstack`).
