#ifndef BHARAT_SDK_IPC_H
#define BHARAT_SDK_IPC_H
#include <bharat/types.h>
typedef struct bh_message { uint32_t struct_size; uint32_t flags; void *data; size_t data_size; size_t transferred; } bh_message_t;
bh_status_t bh_endpoint_open(const char *name, bh_endpoint_t *endpoint);
bh_status_t bh_send(bh_endpoint_t endpoint, const bh_message_t *message, uint64_t timeout_ns);
bh_status_t bh_recv(bh_endpoint_t endpoint, bh_message_t *message, uint64_t timeout_ns);
bh_status_t bh_call(bh_endpoint_t endpoint, const bh_message_t *request, bh_message_t *response, uint64_t timeout_ns);
#endif
