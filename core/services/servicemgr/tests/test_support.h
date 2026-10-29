#ifndef SM_TEST_SUPPORT_H
#define SM_TEST_SUPPORT_H
#include "../servicemgr.h"
#include <assert.h>
#include <bharat/uapi/ipc/status.h>
#include <bharat/uapi/syscall_args.h>
#include <bharat/uapi/syscall_nr.h>
#include <bharat/uapi/process_manager/contract.h>
extern uint64_t test_now;
extern int32_t test_spawn_status, test_stop_status;
extern uint32_t test_process_state, test_spawn_count;
extern const bh_service_launcher_ops_t test_launcher;
bh_servicemgr_t *test_setup(void);
void test_enqueue(uint32_t endpoint, const bharat_ipc_msg_header_t *header, const void *payload);
int32_t test_health(bh_service_instance_t *inst, uint32_t state);
void test_ready(bh_service_instance_t *inst);
void test_add_service(bool critical, uint32_t policy);
#endif
