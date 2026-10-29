#include "test_support.h"
#include <stdio.h>
static void test_failures(void) {
    bh_servicemgr_t *mgr=test_setup(); test_add_service(true,0);
    mgr->deps.launcher=NULL;
    assert(servicemgr_start_all()==BHARAT_IPC_STATUS_ERR_PERM);
    assert(mgr->service_instances[0].state==SM_STATE_FAILED);
    mgr=test_setup(); test_add_service(true,0); test_spawn_status=BHARAT_IPC_STATUS_ERR_INTERNAL;
    assert(servicemgr_start_all()==test_spawn_status);
    assert(mgr->service_instances[0].state==SM_STATE_FAILED);
    mgr=test_setup();
    const char *deps[]={"absent"};
    bh_service_manifest_entry_t manifest={.name="a",.service_id=1,.hard_deps=deps,.hard_deps_count=1};
    assert(servicemgr_load_manifest(&manifest,1)==0);
    assert(servicemgr_validate_dependencies()==BHARAT_IPC_STATUS_ERR_NOT_FOUND);
    assert(servicemgr_start_all()==BHARAT_IPC_STATUS_ERR_NOT_FOUND && test_spawn_count==0);
}
static void test_stability_reset(void) {
    bh_servicemgr_t *mgr=test_setup(); test_add_service(true,0); assert(servicemgr_start_all()==0);
    bh_service_instance_t *inst=&mgr->service_instances[0]; test_ready(inst); mgr->handoff_committed=true;
    servicemgr_check_health(test_now); uint64_t start=test_now;
    test_now+=BH_NS_PER_MS; assert(test_health(inst,SM_HEALTH_STATE_DEGRADED)==0);
    assert(!mgr->stable_window_active && !mgr->stable_reached);
    servicemgr_check_health(test_now);
    assert(test_health(inst,SM_HEALTH_STATE_READY)==0);
    servicemgr_check_health(test_now);
    assert(mgr->stable_started_ns>start);
    for (unsigned i=0;i<11;++i) {
        test_now+=1000*BH_NS_PER_MS;
        assert(test_health(inst,SM_HEALTH_STATE_READY)==0);
        servicemgr_check_health(test_now);
    }
    assert(mgr->stable_reached);
    assert(test_health(inst,SM_HEALTH_STATE_FAILED)==0);
    assert(!mgr->stable_reached && !mgr->stable_window_active);
    assert(servicemgr_init(NULL)==0 && !mgr->stable_window_active);
}
static void test_bounded_backoff(void) {
    bh_servicemgr_t *mgr=test_setup(); test_add_service(false,SM_RESTART_POLICY_BOUNDED_RETRY);
    bh_service_instance_t *inst=&mgr->service_instances[0];
    for (unsigned i=0;i<MAX_RESTART_COUNT;++i) {
        inst->state=SM_STATE_FAILED; inst->process_id=0;
        uint32_t delay=DEFAULT_INITIAL_BACKOFF_MS*(1U<<i);
        servicemgr_check_health(test_now);
        assert(inst->state==SM_STATE_RESTART_BACKOFF && inst->next_retry_ns==test_now+delay*BH_NS_PER_MS);
        test_now=inst->next_retry_ns; servicemgr_check_health(test_now);
        assert(inst->state==SM_STATE_STARTING);
    }
    inst->state=SM_STATE_FAILED; inst->process_id=0;
    servicemgr_check_health(test_now);
    assert(inst->state==SM_STATE_FAILED && inst->restart_count==MAX_RESTART_COUNT);
}
int main(void) { test_failures(); test_stability_reset(); test_bounded_backoff(); puts("servicemgr hardening passed"); }
