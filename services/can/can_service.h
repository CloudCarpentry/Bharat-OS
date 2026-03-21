#ifndef BHARAT_SERVICES_CAN_H
#define BHARAT_SERVICES_CAN_H

#include <stdint.h>
#include <stdbool.h>

/* These constants would normally be in a shared IDL or common header */
#define CAN_SVC_ENDPOINT_ID  400
#define CAN_SVC_MAX_CLIENTS  16
#define CAN_SVC_MAX_FILTERS  64

/* IPC Message Types */
typedef enum {
    CAN_IPC_MSG_REGISTER_CLIENT = 1,
    CAN_IPC_MSG_SUBSCRIBE,
    CAN_IPC_MSG_UNSUBSCRIBE,
    CAN_IPC_MSG_SEND_FRAME,
    CAN_IPC_MSG_RECV_FRAME,
    CAN_IPC_MSG_GET_STATS
} can_ipc_msg_type_t;

/* Stubbed representations for capabilities */
typedef uint32_t capability_id_t;

void can_service_init(void);
void can_service_loop(void);

#endif // BHARAT_SERVICES_CAN_H