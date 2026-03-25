#pragma once

#include <stdint.h>
#include "packet/packet.h"
#include "net/netif.h"
#include "net/ethernet.h"

#define ARPHRD_ETHER 1      /* Ethernet 10Mbps */
#define ARPOP_REQUEST 1     /* ARP request */
#define ARPOP_REPLY 2       /* ARP reply */
#define ARP_HW_ADDR_LEN ETH_ALEN
#define ARP_PR_ADDR_LEN 4

#pragma pack(push, 1)
typedef struct {
    uint16_t ar_hrd;        /* format of hardware address */
    uint16_t ar_pro;        /* format of protocol address */
    uint8_t  ar_hln;        /* length of hardware address */
    uint8_t  ar_pln;        /* length of protocol address */
    uint16_t ar_op;         /* ARP opcode (command) */
    uint8_t  ar_sha[ARP_HW_ADDR_LEN]; /* sender hardware address */
    uint8_t  ar_spa[ARP_PR_ADDR_LEN]; /* sender protocol address */
    uint8_t  ar_tha[ARP_HW_ADDR_LEN]; /* target hardware address */
    uint8_t  ar_tpa[ARP_PR_ADDR_LEN]; /* target protocol address */
} arp_hdr_t;
#pragma pack(pop)

/* Process an incoming ARP packet. Might generate a response. */
int arp_input(netif_t *ni, packet_t *pkt);

/* Output an ARP request */
int arp_request(netif_t *ni, ipv4_addr_t target_ip);

/* Lookup MAC for IP, returns 0 on success, <0 on cache miss/pending */
int arp_lookup(netif_t *ni, ipv4_addr_t ip, mac_addr_t *mac_out);
