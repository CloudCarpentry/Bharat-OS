/* SPDX-License-Identifier: MIT */
#include "bharat/diag/diag_emit.h"
#include <assert.h>
#include <string.h>
static uint64_t clock_now(void *p){return ++*(uint64_t*)p;}
int main(void){bh_diag_ring_t r;bh_diag_ring_slot_t slots[4];bh_diag_record_t out;bh_diag_ring_stats_t st;uint64_t now=0;bh_diag_sink_t sink={0};
 assert(bh_diag_ring_init(0,slots,4,8)==BH_ERR_INVALID_ARGUMENT);assert(bh_diag_ring_init(&r,slots,4,8)==BH_OK);sink.ring=&r;sink.timestamp=clock_now;sink.timestamp_context=&now;sink.source_kind=BH_DIAG_SOURCE_TEST_HARNESS;
 assert(bh_diag_emit(&sink,1,BH_DIAG_SEVERITY_INFO,BH_DIAG_SUBSYSTEM_BOOT,0,0)==BH_OK);assert(bh_diag_ring_peek(&r,&out)==BH_OK&&out.header.sequence==1);assert(bh_diag_ring_try_read(&r,&out)==BH_OK);
 unsigned char p[8]={1,2,3};for(int i=0;i<4;i++)assert(bh_diag_emit(&sink,2,BH_DIAG_SEVERITY_WARNING,BH_DIAG_SUBSYSTEM_SERVICE,p,sizeof p)==BH_OK);assert(bh_diag_emit(&sink,3,BH_DIAG_SEVERITY_INFO,0,0,0)==BH_ERR_BUFFER_FULL);bh_diag_ring_get_stats(&r,&st);assert(st.dropped==1&&st.high_watermark==4);
 for(int i=0;i<4;i++){assert(bh_diag_ring_try_read(&r,&out)==BH_OK);assert(out.header.sequence==(uint64_t)i+2);}assert(bh_diag_emit(&sink,1,BH_DIAG_SEVERITY_COUNT,0,0,0)==BH_ERR_INVALID_ARGUMENT);assert(bh_diag_emit(&sink,1,0,0,p,9)==BH_ERR_INVALID_ARGUMENT);
 for(int i=0;i<1000000;i++){assert(bh_diag_emit(&sink,4,BH_DIAG_SEVERITY_TRACE,BH_DIAG_SUBSYSTEM_UNKNOWN,p,1)==BH_OK);assert(bh_diag_ring_try_read(&r,&out)==BH_OK);}bh_diag_ring_get_stats(&r,&st);assert(st.accepted==1000005&&st.consumed==1000005);
 bh_diag_ring_reset(&r);bh_diag_ring_get_stats(&r,&st);assert(st.accepted==0&&bh_diag_ring_try_read(&r,&out)==BH_ERR_NOT_FOUND);
 return 0;}
