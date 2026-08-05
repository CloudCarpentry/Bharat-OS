#ifndef BHARAT_MK_DISPATCH_H
#define BHARAT_MK_DISPATCH_H

#include "../core/multikernel.h"

// Dispatches an incoming URPC message received on a specific channel.
// This function authenticates and validates the message before passing
// it on to the scheduler or IPC subsystems.
// Returns 0 on successful validation and routing, non-zero on failure.
int mk_dispatch_message(mk_channel_t *channel, urpc_msg_t *msg);
kstatus_t bh_mk_legacy_tx_complete_by_id(uint64_t legacy_txn_id, kstatus_t result);

#endif // BHARAT_MK_DISPATCH_H
