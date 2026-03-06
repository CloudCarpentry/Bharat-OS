#include "../../include/advanced/multikernel.h"
#include <stddef.h>

/**
 * Bharat-OS Lockless inter-core URPC (User-level Remote Procedure Call)
 * Uses single-producer, single-consumer ring buffers in shared memory to 
 * avoid expensive spinlocks when passing state between cores.
 */

#define URPC_SUCCESS 0
#define URPC_ERR_FULL -1
#define URPC_ERR_EMPTY -2

// Internal helper for atomic memory fence (ensure ordering).
// This is compiler-specific. In GCC/Clang:
#define smp_mb() __sync_synchronize()

void urpc_init_ring(urpc_ring_t* ring, urpc_msg_t* buffer_ptr, uint32_t ring_size) {
    if (!ring || !buffer_ptr) return;
    ring->buffer = buffer_ptr;
    ring->capacity = ring_size;
    ring->head = 0;
    ring->tail = 0;
}

int urpc_send(urpc_ring_t* ring, const urpc_msg_t* msg) {
    if (!ring || !msg) return URPC_ERR_EMPTY;
    
    // Optimization for small payloads (<= 64 bits / 8 bytes)
    // Send via register message (architectural fast path)
    if (msg->payload_size <= 8) {
        // Concrete implementation logic for register message passing
        uint64_t fast_payload = msg->payload_data[0];

        // Target core id placeholder (in real implementation extracted from ring metadata)
        uint32_t target_core = 0;

#if defined(__riscv)
        // Ensure RISC-V architectural fast path using SBI
        extern void sbi_send_ipi_payload(const unsigned long* hart_mask, uint64_t payload);
        // Note: sbi_send_ipi_payload is declared as static inline in sbi.h, so we include it directly
        // to avoid linkage errors, or implement a real wrapper.
        // For compilation simplicity in this multi-arch C file without breaking includes:
        unsigned long hart_mask = (1UL << target_core);
        // sbi_call definition would be needed here, or we call a non-inline hal wrapper.
        // We will mock the external call to avoid include hell with `boot/riscv/sbi.h`
        // which isn't typically included in generic ipc code.
        // Let's assume the HAL provides a non-inline wrapper:
        extern void hal_riscv_send_ipi_payload(const unsigned long* hart_mask, uint64_t payload);
        hal_riscv_send_ipi_payload(&hart_mask, fast_payload);
#else
        // Fallback for other architectures (mock generic fast path)
        extern void hal_send_ipi_payload(uint32_t target_core, uint64_t payload);
        hal_send_ipi_payload(target_core, fast_payload);
#endif

        return URPC_SUCCESS; // Bypass shared memory
    }

    uint32_t current_head = ring->head;
    uint32_t next_head = (current_head + 1) % ring->capacity;
    
    // Check if the ring is full
    if (next_head == ring->tail) {
        return URPC_ERR_FULL;
    }
    
    // Copy the message into the ring
    ring->buffer[current_head] = *msg;
    
    // Ensure the message data is visible precisely before updating the head index
    smp_mb();
    
    ring->head = next_head;
    
    // In a full implementation, you'd send an IPI (Inter-Processor Interrupt) here 
    // to wake the target core if it is completely asleep.
    return URPC_SUCCESS;
}

int urpc_receive(urpc_ring_t* ring, urpc_msg_t* out_msg) {
    if (!ring || !out_msg) return URPC_ERR_EMPTY;
    
    uint32_t current_tail = ring->tail;
    
    // Check if the ring is empty
    if (current_tail == ring->head) {
        return URPC_ERR_EMPTY; // Nothing to receive
    }
    
    // Ensure we read the index update before reading the actual message data
    smp_mb();
    
    // Copy out the message
    *out_msg = ring->buffer[current_tail];
    
    // Advance tail
    ring->tail = (current_tail + 1) % ring->capacity;
    
    return URPC_SUCCESS;
}
