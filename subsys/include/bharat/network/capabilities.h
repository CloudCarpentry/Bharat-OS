#ifndef BHARAT_NETWORK_CAPABILITIES_H
#define BHARAT_NETWORK_CAPABILITIES_H

/* Capability bits/rights specific to networking */

#define BNET_CAP_IFACE_MANAGE   (1ULL << 0)
#define BNET_CAP_ROUTE_MANAGE   (1ULL << 1)
#define BNET_CAP_ADDR_MANAGE    (1ULL << 2)

#define BNET_CAP_QUEUE_BIND     (1ULL << 3)
#define BNET_CAP_FASTPATH_BIND  (1ULL << 4)

#define BNET_CAP_PACKET_TX      (1ULL << 5)
#define BNET_CAP_PACKET_RX      (1ULL << 6)

#define BNET_CAP_RAW_SOCKET     (1ULL << 7)
#define BNET_CAP_PROMISC        (1ULL << 8)

#define BNET_CAP_PTP_CONTROL    (1ULL << 9)

#endif // BHARAT_NETWORK_CAPABILITIES_H
