/* SPDX-License-Identifier: MIT */
#include "bharat/diag/diag_collector.h"
#include <assert.h>
static bh_status_t take(void *p,const bh_diag_record_t*r){(*(unsigned*)p)++;return r->header.event_type==77?BH_OK:BH_ERR_FAULT;}
int main(void){bh_diag_ring_t ring;bh_diag_ring_slot_t slots[2];bh_diag_event_header_t h={BH_DIAG_ABI_VERSION,sizeof(h),77,BH_DIAG_SEVERITY_INFO,BH_DIAG_SOURCE_SERVICE,0,0,0,1,8,0,BH_DIAG_SUBSYSTEM_SERVICE};bh_diag_collector_t c;bh_diag_health_snapshot_t health;unsigned n=0;assert(bh_diag_ring_init(&ring,slots,2,8)==BH_OK);bh_diag_collector_init(&c,8);assert(bh_diag_ring_try_write(&ring,&h,0)==BH_OK);assert(bh_diag_collector_consume(&c,&ring,take,&n)==BH_OK&&n==1);bh_diag_collector_health(&c,5,4,&health);assert(health.state==BH_DIAG_HEALTH_HEALTHY);return 0;}
