#ifndef BHARAT_MK_PROTO_H
#define BHARAT_MK_PROTO_H

#include "../advanced/multikernel.h"

// Delivery Flags
#define MK_MSG_FLAG_ACK_REQUIRED  (1U << 0)
#define MK_MSG_FLAG_IS_REPLY      (1U << 1)
#define MK_MSG_FLAG_ERROR         (1U << 2)

// Reason Codes / Ack Nack
#define MK_REASON_SUCCESS      0
#define MK_REASON_BAD_AUTH     1
#define MK_REASON_UNSUPPORTED  2
#define MK_REASON_TIMEOUT      3

// Distributed Message Types
#define MK_MSG_MEM_RESERVE      10U
#define MK_MSG_PROC_LOOKUP      11U
#define MK_MSG_CAP_RETYPE       12U
#define MK_MSG_TLB_SHOOTDOWN    13U

#endif // BHARAT_MK_PROTO_H
