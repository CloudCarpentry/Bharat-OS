#include "../../../core/services/servicemgr/tests/test_support.h"
#include <string.h>
#include <stdio.h>
#include <ipc_user.h>

static void test_syscall_fixture(void) {
    test_setup();
    uint8_t bytes[4] = {1,2,3,4}, result[4] = {0};
    uint32_t length = 99, cap = 99;
    assert(ipc_endpoint_receive(9,result,sizeof(result),&length,0,&cap)==-8);
    assert(length==0 && cap==0);
    assert(ipc_endpoint_send(9,bytes,sizeof(bytes),0,0,0)==0);
    assert(ipc_endpoint_receive(9,result,1,&length,0,&cap)==-2);
    assert(ipc_endpoint_receive(9,result,sizeof(result),&length,0,&cap)==0);
    assert(length==4 && memcmp(result,bytes,4)==0 && cap==0);
    assert(ipc_endpoint_receive(9,NULL,4,&length,0,&cap)==-1);
    assert(bh_syscall(-1,1,0,0,0,0,0)==-1);
}

static void test_handoff_validation(void) {
    bh_servicemgr_t *mgr = test_setup();
    sm_handoff_begin_t begin = {.abi_major=99, .struct_size=sizeof(begin), .boot_session_id=55, .selected_service_count=1};
    bharat_ipc_msg_header_t header = {.header_version=BHARAT_IPC_HEADER_VERSION_V1,
        .service_id=SERVICEMGR_SERVICE_ID, .opcode=SM_OP_HANDOFF_BEGIN, .payload_size=sizeof(begin)};
    test_enqueue(mgr->my_endpoint, &header, &begin);
    assert(servicemgr_poll_once(mgr, test_now) == BHARAT_IPC_STATUS_ERR_VERSION);
    begin.abi_major=1; begin.struct_size=12;
    test_enqueue(mgr->my_endpoint, &header, &begin);
    assert(servicemgr_poll_once(mgr, test_now) == BHARAT_IPC_STATUS_ERR_VERSION);
    begin.struct_size=sizeof(begin);
    header.capability_transfer=0x40; header.reply_endpoint=0x300;
    test_enqueue(mgr->my_endpoint, &header, &begin);
    assert(servicemgr_poll_once(mgr, test_now) == BHARAT_IPC_STATUS_ERR_PERM);
    assert(!mgr->handoff_begun);
    header.payload_size=sizeof(begin)-1;
    test_enqueue(mgr->my_endpoint, &header, &begin);
    assert(servicemgr_poll_once(mgr, test_now) == BHARAT_IPC_STATUS_ERR_LENGTH);
}
static sm_handoff_begin_t begin_record(uint32_t count) {
    return (sm_handoff_begin_t){.abi_major=1, .struct_size=sizeof(sm_handoff_begin_t),
        .boot_session_id=55, .selected_service_count=count};
}
static sm_handoff_service_t service_record(uint16_t index, uint16_t id, const char *name) {
    sm_handoff_service_t rec = {.record_index=index, .service_id=id, .process_id=100+id, .incarnation_id=7};
    strcpy(rec.service_name, name); return rec;
}
static void test_staging_rollback_and_adoption(void) {
    bh_servicemgr_t *mgr=test_setup(); test_add_service(true, SM_RESTART_POLICY_NONE);
    sm_handoff_begin_t begin=begin_record(2);
    assert(servicemgr_stage_begin(&begin)==0);
    assert(mgr->instance_count==1 && mgr->boot_session_id==42);
    sm_handoff_service_t rec=service_record(1, 10, "namesvc");
    assert(servicemgr_stage_service(&rec)==BHARAT_IPC_STATUS_ERR_INVALID);
    assert(!mgr->handoff_begun && mgr->instance_count==1);
    begin=begin_record(1); assert(servicemgr_stage_begin(&begin)==0);
    rec=service_record(0,10,"namesvc");
    assert(servicemgr_stage_service(&rec)==0);
    assert(mgr->service_instances[0].service_id==1);
    sm_handoff_commit_t commit={.boot_session_id=55,.record_count=1};
    assert(servicemgr_stage_commit(&commit)==0);
    assert(mgr->service_instances[0].process_id==110 && mgr->service_instances[0].incarnation_id==7);
    assert(mgr->service_instances[0].state==SM_STATE_STARTING);
    assert(servicemgr_stage_commit(&commit)==BHARAT_IPC_STATUS_ERR_INVALID);
    assert(mgr->service_instances[0].process_id==110);
}
static void test_duplicate_cycle_and_missing_dependencies(void) {
    bh_servicemgr_t *mgr=test_setup(); test_add_service(false,0);
    sm_handoff_begin_t begin=begin_record(2); assert(servicemgr_stage_begin(&begin)==0);
    sm_handoff_service_t a=service_record(0,10,"a"), b=service_record(1,10,"b");
    assert(servicemgr_stage_service(&a)==0);
    assert(servicemgr_stage_service(&b)==BHARAT_IPC_STATUS_ERR_INVALID);
    assert(mgr->instance_count==1 && mgr->service_instances[0].service_id==1);
    assert(servicemgr_stage_begin(&begin)==0);
    a.dependency_count=1; a.dependencies[0]=11;
    b.service_id=11; b.process_id=111; b.dependency_count=1; b.dependencies[0]=10;
    assert(servicemgr_stage_service(&a)==0); assert(servicemgr_stage_service(&b)==0);
    sm_handoff_commit_t commit={.boot_session_id=55,.record_count=2};
    assert(servicemgr_stage_commit(&commit)==BHARAT_IPC_STATUS_ERR_INVALID);
    assert(mgr->instance_count==1);
    assert(servicemgr_stage_begin(&begin)==0); a.dependencies[0]=99; b.dependency_count=0;
    assert(servicemgr_stage_service(&a)==0); assert(servicemgr_stage_service(&b)==0);
    assert(servicemgr_stage_commit(&commit)==BHARAT_IPC_STATUS_ERR_NOT_FOUND);
    assert(mgr->instance_count==1);
}
int main(void) {
    test_syscall_fixture(); test_handoff_validation(); test_staging_rollback_and_adoption(); test_duplicate_cycle_and_missing_dependencies();
    puts("servicemgr: wire validation, forged authority, staging rollback/adoption/duplicates/cycles passed");
}
