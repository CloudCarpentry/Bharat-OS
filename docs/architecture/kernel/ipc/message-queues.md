# Message Queues

## Overview
POSIX Message Queues (`mq_open`, `mq_send`, `mq_receive`) and System V Message Queues (`msgget`, `msgsnd`, `msgrcv`) provide asynchronous, buffered, and priority-ordered message passing.

## Contrast with Synchronous Endpoints
Unlike Bharat-OS Kernel Endpoints (`ipc_endpoint_send`), which are *synchronous* and unbuffered (the sender blocks until the receiver is ready, passing data in registers), POSIX Message Queues require the kernel (or a server) to allocate memory to buffer messages if the receiver is not ready.

## Implementation Architecture

In a microkernel, adding unbounded memory buffering to the core kernel is a security and denial-of-service risk. Therefore, POSIX Message Queues are implemented as a user-space **Message Queue Server** (or integrated into the VFS).

### The Message Queue Server
1.  **Creation (`mq_open`):** A client sends an IPC request to the MQ Server to create a queue named `/my_queue`. The server allocates a data structure (e.g., a linked list of messages sorted by priority).
2.  **Sending (`mq_send`):** The client sends an IPC message containing the payload and priority to the MQ Server. The server copies the payload into its own heap and acknowledges the sender.
3.  **Receiving (`mq_receive`):** The client sends a receive request to the MQ Server. If a message is available, the server replies with the highest-priority message. If not, the server holds the client's IPC reply capability and puts the client to sleep until a message arrives.

### Flow Control and Quotas
The MQ Server must enforce strict quotas (`mq_maxmsg`, `mq_msgsize`) to prevent a malicious client from exhausting the server's memory. When the queue is full, `mq_send` blocks (the server delays the reply) or returns `EAGAIN`.

### Zero-Copy Message Queues (Advanced)
For large messages, copying data to and from the MQ Server is inefficient. Bharat-OS supports a zero-copy capability-passing model:
-   Instead of sending the *data* to the MQ Server, the sender allocates a page of memory, writes the message to it, and sends a *Capability* to that page to the MQ Server.
-   The MQ Server stores the Capability in its queue.
-   When the receiver calls `mq_receive`, the server delegates the Capability to the receiver. The receiver now has direct access to the memory page.
-   This requires the sender and receiver to manage the lifecycle of the shared memory pages (e.g., using a pool of pre-allocated buffers).