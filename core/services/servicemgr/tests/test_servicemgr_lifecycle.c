#include "test_support.h"
#include <stdio.h>
static void test_lifecycle(void) {
    bh_servicemgr_t *mgr=test_setup(); test_add_service(true,SM_RESTART_POLICY_BOUNDED_RETRY);
    assert(servicemgr_start_all()==0);
    bh_service_instance_t *inst=&mgr->service_instances[0];
    assert(inst->state==SM_STATE_STARTING && inst->process_id==101);
    assert(test_health(inst,SM_HEALTH_STATE_SPAWNED)==0);
    assert(inst->state==SM_STATE_STARTING);
    assert(test_health(inst,SM_HEALTH_STATE_READY)==BHARAT_IPC_STATUS_ERR_PERM);
    test_ready(inst);
    assert(inst->last_heartbeat_ns==test_now);
    sm_req_health_update_t req={.struct_size=sizeof(req),.service_id=1,.process_id=101,
        .incarnation_id=1,.boot_session_id=42,.health_state=SM_HEALTH_STATE_READY,.heartbeat_sequence=inst->heartbeat_sequence+1};
    sm_resp_health_update_t resp;
    assert(servicemgr_handle_health_update(&req,&resp,102)==BHARAT_IPC_STATUS_ERR_PERM);
    req.boot_session_id=43;
    assert(servicemgr_handle_health_update(&req,&resp,101)==BHARAT_IPC_STATUS_ERR_PERM);
    req.boot_session_id=42; req.struct_size--;
    assert(servicemgr_handle_health_update(&req,&resp,101)==BHARAT_IPC_STATUS_ERR_INVALID);
    req.struct_size++;
    req.incarnation_id=0;
    assert(servicemgr_handle_health_update(&req,&resp,101)==BHARAT_IPC_STATUS_ERR_PERM);
    req.incarnation_id=1; req.heartbeat_sequence=inst->heartbeat_sequence;
    assert(servicemgr_handle_health_update(&req,&resp,101)==BHARAT_IPC_STATUS_ERR_PERM);
    test_now+=(uint64_t)HEARTBEAT_TIMEOUT_MS*BH_NS_PER_MS+1;
    servicemgr_check_health(test_now);
    assert(inst->state==SM_STATE_FAILED && inst->stop_requested);
    assert(test_spawn_count==1);
    test_process_state=PM_STATE_EXITED;
    servicemgr_check_health(test_now);
    assert(inst->state==SM_STATE_RESTART_BACKOFF && inst->restart_count==1);
    test_now=inst->next_retry_ns; servicemgr_check_health(test_now);
    assert(inst->state==SM_STATE_STARTING && inst->incarnation_id==2 && test_spawn_count==2);
    test_ready(inst);
    sm_req_stop_t stop={.service_id=1}; sm_resp_stop_t stopped;
    test_stop_status=BHARAT_IPC_STATUS_ERR_INTERNAL;
    assert(servicemgr_handle_stop(&stop,&stopped)==test_stop_status && inst->state==SM_STATE_RUNNING);
    test_stop_status=0;
    assert(servicemgr_handle_stop(&stop,&stopped)==0 && inst->state==SM_STATE_STOPPING);
    test_process_state=PM_STATE_EXITED; servicemgr_check_health(test_now);
    assert(inst->state==SM_STATE_STOPPED);
}
int main(void) { test_lifecycle(); puts("servicemgr lifecycle passed"); }
