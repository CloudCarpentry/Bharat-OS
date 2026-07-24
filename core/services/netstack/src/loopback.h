#ifndef NETSTACK_LOOPBACK_H
#define NETSTACK_LOOPBACK_H

#include "netbuf.h"

/* Transmit a packet to the loopback interface.
   This simply turns around and feeds it back into the IPv4 RX path. */
int loopback_tx(netbuf_t *nb);

#endif // NETSTACK_LOOPBACK_H
