#ifndef BHARAT_IO_H
#define BHARAT_IO_H

#include <stdint.h>

/*
 * Bharat-OS High-Throughput Asynchronous I/O Architecture
 * Inspired by io_uring and DPDK for high-performance data center networking.
 */

typedef enum {
    IO_OP_READ,
    IO_OP_WRITE,
    IO_OP_POLL,
    IO_OP_SENDMSG,
    IO_OP_RECVMSG
} io_opcode_t;

// Submission Queue Entry
typedef struct {
    io_opcode_t opcode;
    uint32_t fd;
    uint64_t off;
    void* addr;
    uint32_t len;
    uint64_t user_data;
} io_sqe_t;

// Completion Queue Entry
typedef struct {
    uint64_t user_data;
    int32_t res;     // Resulting bytes read/written or error code
    uint32_t flags;
} io_cqe_t;

// A ring buffer mapped into both Kernel and User address spaces for zero-copy I/O
typedef struct {
    io_sqe_t* sq_ring;
    io_cqe_t* cq_ring;
    uint32_t sq_tail;
    uint32_t sq_head;
    uint32_t cq_tail;
    uint32_t cq_head;
    uint32_t ring_size;
} io_ring_t;

// Setup a zero-copy asynchronous I/O ring
int io_setup_ring(uint32_t entries, io_ring_t* out_ring);

// Submit accumulated entries in the Submission Queue
int io_submit_sqes(io_ring_t* ring);

#endif // BHARAT_IO_H
