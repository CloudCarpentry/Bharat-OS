## Networking Audit Report

Component | Implemented | Mock/Stub | Host Tested | QEMU Verified | Missing Work
--- | --- | --- | --- | --- | ---
**VirtIO-Net Driver** (`core/drivers/net/virtio_net/virtio_net.c`) | Partial | Mocked | No | No | PCI probing, Virtqueue setup, real DMA TX/RX rings, host notification.
**Netstack Adapter** (`core/services/netstack/src/driver_virtio_adapter.c`) | Partial | Mocked | No | No | Needs real PCI device handle passed down; currently hardcodes `mock_virtio_device = NULL`.
**Netstack Main Loop** (`core/services/netstack/src/main.c`) | Scaffold | Yes | No | No | Busy waits instead of real IPC/uRPC or sleeping. Doesn't configure IP.
**Ethernet** (`core/services/netstack/src/ethernet.c`) | Yes | No | Yes | No | Works basically but lacks multicast filtering.
**ARP** (`core/services/netstack/src/arp.c`) | Yes | No | Yes | No | Synchronous cache, doesn't queue pending packets, simple LRU.
**IPv4** (`core/services/netstack/src/ipv4.c`) | Yes | No | Yes | No | Basic routing implemented but fragmentation unsupported.
**ICMP** (`core/services/netstack/src/icmp.c`) | Yes | No | Yes | No | Echo reply is implemented; unreachable partially supported.
**Packet Buffers** (`core/lib/packet/src/packet.c`) | Yes | No | Yes | No | Simple preallocated array, not thread-safe.

*Duplicate/Legacy Stacks*: `core/services/legacy/net/` and `core/services/network/netstack/` exist but are legacy or alternate paths. We will focus on `core/services/netstack/`.
