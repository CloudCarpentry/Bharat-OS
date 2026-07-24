#include "loopback.h"
#include "ipv4.h"
//#include <stdio.h>

int loopback_tx(netbuf_t *nb) {
    // In a real system, we might enqueue this into a loopback queue or softirq.
    // For Phase 2, we can just synchronously feed it back into the IPv4 receive path,
    // as loopback bypasses Ethernet encapsulation.

    // Deep copy the netbuf so the caller's buffer isn't mutilated by the RX path,
    // though in many network stacks the buffer ownership is simply passed.
    // We'll pass ownership here (which effectively destroys the TX buffer, which is fine for Phase 2).

    return ipv4_rx(nb);
}
