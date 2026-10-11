## Canonical Network Architecture Proposal

**Packet Buffer Ownership and Lifetime:**
- Buffers are allocated by the network driver/adapter via `libpacket` (`packet_alloc`).
- Upon RX, the VirtIO net driver posts pre-allocated descriptors to the RX virtqueue.
- When QEMU fills an RX descriptor, the driver passes the `packet_buf_t` to the netstack.
- The netstack processes it and eventually calls `packet_free` (or it is passed to a user app which frees it via IPC/shared memory in the future).
- For TX, the netstack allocates a buffer, fills it, and passes it to the driver. The driver puts it in the TX virtqueue. When the device completes TX, the driver frees the buffer.

**Queue Ownership:**
- The network service (netstack) has exclusive capability-based control over the virtio-net NIC queues. In the future, this will be represented by hardware MMIO caps and IOMMU policies.

**NIC-to-service notification:**
- Currently achieved via polling (or driver IRQ). In Phase 3, we will use `bh_virtio_pci_notify_queue`.

**Packet Size and Bounds Validation:**
- Drivers enforce `packet_buf_t->tail_len` bounds.
- VirtIO limits descriptor len to MTU.

**Link state and service readiness:**
- The driver reports `NETDRV_STATE_STARTED` and `link_up` boolean.

**Error Propagation & Backpressure:**
- If the TX virtqueue is full, `virtio_net_tx` returns an error, and the packet is dropped (with a counter incremented). No complex queueing yet.
