## Production-readiness gap matrix

- Memory safety: Good, bounds checking exists.
- Capability isolation: Needs future integration with IOMMU policies.
- SMP concurrency: Netstack is currently single-threaded. Needs locks or per-core state for ARP cache.
- Packet processing latency: Polling based; could be improved with IRQ.
- Queue saturation: Need proper TX backpressure.
- DMA correctness: Uses direct phys pointers; needs IOMMU awareness.
- TCP correctness gap: Incomplete connection establishment.

Next prioritized tasks:
1. TCP state machine (SYN/ACK).
2. Proper interrupt-driven RX.
3. DHCP/DNS clients.
