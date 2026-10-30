#include "init_manifest.h"
#include <errno.h>
#include <stddef.h>


#include <bharat/uapi/process_manager/contract.h>
#include <bharat/uapi/process_manager/contract_v1.h>
#include <bharat/ipc/ipc.h>
#include <bharat/namesvc/client.h>
#include <bharat/runtime/runtime.h>
#include <ipc_user.h>

static int spawn_service(void *ctx) {
    init_service_runtime_t *sr = (init_service_runtime_t *)ctx;
    if (!sr || !sr->desc) return -1;

    bharat_runtime_log("INIT: SPAWN_SERVICE_START ");
    bharat_runtime_log(sr->desc->name ? sr->desc->name : "unnamed");
    bharat_runtime_log("\n");

    if (sr->desc->id == INIT_SVC_NAMESVC || sr->desc->id == INIT_SVC_PROCESS_MANAGER) {
        const char *name = sr->desc->id == INIT_SVC_NAMESVC
            ? "services/namesvc" : "services/process_manager";
        return bharat_bootstrap_launch(name, sr->desc->id, sr->namesvc_cap,
                                      sr->desc->id == INIT_SVC_PROCESS_MANAGER, &sr->launch);
    }

    bharat_service_id_t pm_svc_id = 0;
    bharat_ipc_endpoint_t pm_ep = sr->pm_cap;
    uint32_t pm_version = 0;

    if (pm_ep == 0 || pm_ep == BHARAT_CAP_INVALID_HANDLE) {
        int lookup_ret = namesvc_lookup("bharat.process_manager", &pm_svc_id, &pm_ep, &pm_version);
        if (lookup_ret != NAMESVC_STATUS_OK || pm_ep == BHARAT_CAP_INVALID_HANDLE) {
            bharat_runtime_log("SPAWN_FAIL: NO_PM_EP\n");
            return -1;
        }
    }

    uint64_t exec_handle = sr->desc->executable_desc.executable_handle;
    const char *module_name = sr->desc->executable_desc.module_name;
    if (exec_handle != 0 && module_name != NULL) {
        // Step 1: Register executable with Process Manager
        bh_pm_register_executable_request_v1_t reg_req = {
            .abi_version = BH_PM_INTERFACE_VERSION_V1,
            .struct_size = sizeof(bh_pm_register_executable_request_v1_t),
            .executable_handle = exec_handle,
        };
        __builtin_strncpy(reg_req.module_name, module_name, sizeof(reg_req.module_name) - 1);

        bharat_ipc_msg_header_t reg_req_hdr = {
            .header_version = BHARAT_IPC_HEADER_VERSION_V1,
            .service_id = PROCESS_MANAGER_SERVICE_ID,
            .interface_version = BH_PM_INTERFACE_VERSION_V1,
            .opcode = BH_PM_OP_REGISTER_EXECUTABLE_V1,
            .payload_size = sizeof(bh_pm_register_executable_request_v1_t),
            .reply_endpoint = 0,
        };

        int32_t reg_status = bharat_ipc_send(pm_ep, &reg_req_hdr, &reg_req);
        if (reg_status != BHARAT_IPC_STATUS_OK) {
            bharat_runtime_log("SPAWN_FAIL: REG_FAILED\n");
            return -1;
        }
    } else if (sr->desc->name != NULL) {
        exec_handle = (uint64_t)sr->desc->id;
        module_name = sr->desc->name;
    } else {
        bharat_runtime_log("SPAWN_FAIL: NO_EXEC_DESC\n");
        return -1;
    }

    // Step 2: Create readiness event endpoint pair
    uint32_t event_send = 0;
    uint32_t event_recv = 0;
    if (ipc_endpoint_create(&event_send, &event_recv) != 0) {
        bharat_runtime_log("SPAWN_FAIL: EP_CREATE_FAILED\n");
        return -1;
    }
    sr->launch.event_receive_cap = event_recv;

    // Step 3: Send spawn request with valid executable_handle via authenticated RPC
    bh_pm_spawn_request_v1_t req = {
        .abi_version = BH_PM_INTERFACE_VERSION_V1,
        .struct_size = sizeof(bh_pm_spawn_request_v1_t),
        .executable_handle = exec_handle,
        .priority = 10,
        .affinity_mask = 0xFFFFFFFF,
        .memory_profile = 0,
        .reserved = (uint32_t)sr->desc->id,
    };
    __builtin_strncpy(req.process_name, sr->desc->name, sizeof(req.process_name) - 1);

    bharat_ipc_msg_header_t req_hdr = {
        .header_version = BHARAT_IPC_HEADER_VERSION_V1,
        .service_id = PROCESS_MANAGER_SERVICE_ID,
        .interface_version = BH_PM_INTERFACE_VERSION_V1,
        .opcode = BH_PM_OP_SPAWN_V1,
        .payload_size = sizeof(bh_pm_spawn_request_v1_t),
        .capability_transfer = event_send,
        .reply_endpoint = 0,
    };
    bharat_ipc_msg_header_t rep_hdr = {0};
    bh_pm_spawn_response_v1_t rep = {0};

    int32_t call_status = bharat_ipc_call_ex(pm_ep, &req_hdr, &req, &rep_hdr, &rep, sizeof(rep), 1000);
    if (call_status != BHARAT_IPC_STATUS_OK || rep.status != 0) {
        (void)ipc_endpoint_close(event_send);
        (void)ipc_endpoint_close(event_recv);
        sr->launch.event_receive_cap = 0;
        bharat_runtime_log("SPAWN_FAIL: SPAWN_CALL_FAILED\n");
        return -1;
    }
    sr->launch.process_id = rep.kernel_process_id;

    bharat_sched_yield();
    return 0;
}


static int stub_rollback(void *ctx) {
    (void)ctx;
    return 0;
}

static int bootstrap_rollback(void *ctx) {
    init_service_runtime_t *sr = ctx;
    if (!sr || !sr->launch.process_cap) return -EINVAL;
    int status = bharat_bootstrap_stop(sr->launch.process_cap);
    if (status != 0) bharat_runtime_log("BOOT_FAIL: ROLLBACK_QUARANTINED\n");
    return status;
}

static const init_service_id_t deps_namesvc[] = { INIT_SVC_NONE };
static const init_service_id_t deps_devmgr[] = { INIT_SVC_NAMESVC };
static const init_service_id_t deps_process_manager[] = { INIT_SVC_NAMESVC };
static const init_service_id_t deps_vm_manager[] = { INIT_SVC_NAMESVC, INIT_SVC_PROCESS_MANAGER };
static const init_service_id_t deps_servicemgr[] = { INIT_SVC_NAMESVC };
static const init_service_id_t deps_faultmgr[] = { INIT_SVC_NAMESVC };
static const init_service_id_t deps_boot_displayd[] = { INIT_SVC_NAMESVC };
static const init_service_id_t deps_telemetrymgr[] = { INIT_SVC_NAMESVC, INIT_SVC_FAULTMGR };
static const init_service_id_t deps_storagemgr[] = { INIT_SVC_NAMESVC, INIT_SVC_DEVMGR };
static const init_service_id_t deps_accelmgr[] = { INIT_SVC_NAMESVC };

const init_service_desc_t g_init_manifest[] = {
    {
        .id = INIT_SVC_NAMESVC,
        .name = "namesvc",
        .boot_class = BOOT_CLASS_CORE,
        .start_deadline_ms = 1000,
        .ready_deadline_ms = 5000,
        .start_fn = spawn_service,
        .probe_fn = NULL,
        .bootstrap_hint_fn = NULL,
        .rollback_fn = bootstrap_rollback,
        .deps = deps_namesvc,
        .dep_count = 0,
        .retry_limit = 3,
        .policy = INIT_SERVICE_REQUIRED,
        .profile_mask = BHARAT_INIT_PROFILE_SMALL | BHARAT_INIT_PROFILE_EMBEDDED_RICH |
                        BHARAT_INIT_PROFILE_MOBILE | BHARAT_INIT_PROFILE_DESKTOP |
                        BHARAT_INIT_PROFILE_DRONE | BHARAT_INIT_PROFILE_CLOUD |
                        BHARAT_INIT_PROFILE_AUTOMOTIVE | BHARAT_INIT_PROFILE_TV |
                        BHARAT_INIT_PROFILE_APPLIANCE | BHARAT_INIT_PROFILE_WATCH,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_NONE,
        .executable_desc = { .executable_handle = 0x4E530001ULL, .module_name = "services/namesvc" },
    },
    {
        .id = INIT_SVC_PROCESS_MANAGER,
        .name = "process_manager",
        .boot_class = BOOT_CLASS_CORE,
        .start_deadline_ms = 1000,
        .ready_deadline_ms = 5000,
        .start_fn = spawn_service,
        .probe_fn = NULL,
        .bootstrap_hint_fn = NULL,
        .rollback_fn = bootstrap_rollback,
        .deps = deps_process_manager,
        .dep_count = 1,
        .retry_limit = 3,
        .policy = INIT_SERVICE_REQUIRED,
        .profile_mask = BHARAT_INIT_PROFILE_EMBEDDED_RICH | BHARAT_INIT_PROFILE_MOBILE |
                        BHARAT_INIT_PROFILE_DESKTOP | BHARAT_INIT_PROFILE_AUTOMOTIVE |
                        BHARAT_INIT_PROFILE_CLOUD,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_NONE,
        .executable_desc = { .executable_handle = 0x504D0001ULL, .module_name = "services/process_manager" },
    },
    {
        .id = INIT_SVC_VM_MANAGER,
        .name = "vm_manager",
        .boot_class = BOOT_CLASS_CORE,
        .start_deadline_ms = 1000,
        .ready_deadline_ms = 5000,
        .start_fn = spawn_service,
        .probe_fn = NULL,
        .bootstrap_hint_fn = NULL,
        .rollback_fn = stub_rollback,
        .deps = deps_vm_manager,
        .dep_count = 2,
        .retry_limit = 3,
        .policy = INIT_SERVICE_OPTIONAL,
        .profile_mask = BHARAT_INIT_PROFILE_EMBEDDED_RICH | BHARAT_INIT_PROFILE_MOBILE |
                        BHARAT_INIT_PROFILE_DESKTOP | BHARAT_INIT_PROFILE_CLOUD,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_MMU,
        .executable_desc = { .executable_handle = 0x564D0001ULL, .module_name = "services/vm_manager" },
    },
    {
        .id = INIT_SVC_DEVMGR,
        .name = "devmgr",
        .boot_class = BOOT_CLASS_INFRA,
        .start_deadline_ms = 1000,
        .ready_deadline_ms = 5000,
        .start_fn = spawn_service,
        .probe_fn = NULL,
        .bootstrap_hint_fn = NULL,
        .rollback_fn = stub_rollback,
        .deps = deps_devmgr,
        .dep_count = 1,
        .retry_limit = 2,
        .policy = INIT_SERVICE_OPTIONAL,
        .profile_mask = BHARAT_INIT_PROFILE_SMALL | BHARAT_INIT_PROFILE_EMBEDDED_RICH |
                        BHARAT_INIT_PROFILE_MOBILE | BHARAT_INIT_PROFILE_DESKTOP |
                        BHARAT_INIT_PROFILE_AUTOMOTIVE | BHARAT_INIT_PROFILE_TV |
                        BHARAT_INIT_PROFILE_APPLIANCE | BHARAT_INIT_PROFILE_WATCH |
                        BHARAT_INIT_PROFILE_CLOUD,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_NONE,
        .executable_desc = { .executable_handle = 0x44455601ULL, .module_name = "services/devmgr" },
    },
    {
        .id = INIT_SVC_FAULTMGR,
        .name = "faultmgr",
        .boot_class = BOOT_CLASS_INFRA,
        .start_deadline_ms = 1000,
        .ready_deadline_ms = 5000,
        .start_fn = spawn_service,
        .probe_fn = NULL,
        .bootstrap_hint_fn = NULL,
        .rollback_fn = stub_rollback,
        .deps = deps_faultmgr,
        .dep_count = 1,
        .retry_limit = 1,
        .policy = INIT_SERVICE_REQUIRED,
        .profile_mask = BHARAT_INIT_PROFILE_SMALL | BHARAT_INIT_PROFILE_EMBEDDED_RICH |
                        BHARAT_INIT_PROFILE_DRONE | BHARAT_INIT_PROFILE_AUTOMOTIVE |
                        BHARAT_INIT_PROFILE_MOBILE | BHARAT_INIT_PROFILE_DESKTOP |
                        BHARAT_INIT_PROFILE_WATCH | BHARAT_INIT_PROFILE_APPLIANCE |
                        BHARAT_INIT_PROFILE_CLOUD | BHARAT_INIT_PROFILE_TV,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_NONE,
        .executable_desc = { .executable_handle = 0x464C5401ULL, .module_name = "services/faultmgr" },
    },
    {
        .id = INIT_SVC_SERVICEMGR,
        .name = "servicemgr",
        .boot_class = BOOT_CLASS_INFRA,
        .start_deadline_ms = 1000,
        .ready_deadline_ms = 5000,
        .start_fn = spawn_service,
        .probe_fn = NULL,
        .bootstrap_hint_fn = NULL,
        .rollback_fn = stub_rollback,
        .deps = deps_servicemgr,
        .dep_count = 1,
        .retry_limit = 3,
        .policy = INIT_SERVICE_OPTIONAL,
        .profile_mask = BHARAT_INIT_PROFILE_EMBEDDED_RICH | BHARAT_INIT_PROFILE_MOBILE |
                        BHARAT_INIT_PROFILE_DESKTOP | BHARAT_INIT_PROFILE_AUTOMOTIVE |
                        BHARAT_INIT_PROFILE_CLOUD | BHARAT_INIT_PROFILE_TV,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_NONE,
        .executable_desc = { .executable_handle = 0x534D0001ULL, .module_name = "services/servicemgr" },
    },
    {
        .id = INIT_SVC_BOOT_DISPLAYD,
        .name = "boot_displayd",
        .boot_class = BOOT_CLASS_LATE,
        .start_deadline_ms = 1000,
        .ready_deadline_ms = 5000,
        .start_fn = spawn_service,
        .probe_fn = NULL,
        .bootstrap_hint_fn = NULL,
        .rollback_fn = stub_rollback,
        .deps = deps_boot_displayd,
        .dep_count = 1,
        .retry_limit = 2,
        .policy = INIT_SERVICE_OPTIONAL,
        .profile_mask = BHARAT_INIT_PROFILE_MOBILE | BHARAT_INIT_PROFILE_DESKTOP |
                        BHARAT_INIT_PROFILE_TV | BHARAT_INIT_PROFILE_WATCH,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_DISPLAY,
        .executable_desc = { .executable_handle = 0x44535001ULL, .module_name = "services/boot_displayd" },
    },
    {
        .id = INIT_SVC_TELEMETRYMGR,
        .name = "telemetrymgr",
        .boot_class = BOOT_CLASS_OPTIONAL,
        .start_deadline_ms = 1000,
        .ready_deadline_ms = 5000,
        .start_fn = spawn_service,
        .probe_fn = NULL,
        .bootstrap_hint_fn = NULL,
        .rollback_fn = stub_rollback,
        .deps = deps_telemetrymgr,
        .dep_count = 2,
        .retry_limit = 2,
        .policy = INIT_SERVICE_OPTIONAL,
        .profile_mask = BHARAT_INIT_PROFILE_MOBILE | BHARAT_INIT_PROFILE_DESKTOP |
                        BHARAT_INIT_PROFILE_AUTOMOTIVE | BHARAT_INIT_PROFILE_CLOUD |
                        BHARAT_INIT_PROFILE_TV | BHARAT_INIT_PROFILE_WATCH,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_NETWORK,
        .executable_desc = { .executable_handle = 0x54454C01ULL, .module_name = "services/telemetrymgr" },
    },
    {
        .id = INIT_SVC_STORAGEMGR,
        .name = "storagemgr",
        .boot_class = BOOT_CLASS_INFRA,
        .start_deadline_ms = 1200,
        .ready_deadline_ms = 5000,
        .start_fn = spawn_service,
        .probe_fn = NULL,
        .bootstrap_hint_fn = NULL,
        .rollback_fn = stub_rollback,
        .deps = deps_storagemgr,
        .dep_count = 2,
        .retry_limit = 1,
        .policy = INIT_SERVICE_REQUIRED,
        .profile_mask = BHARAT_INIT_PROFILE_DESKTOP | BHARAT_INIT_PROFILE_MOBILE |
                        BHARAT_INIT_PROFILE_TV | BHARAT_INIT_PROFILE_AUTOMOTIVE |
                        BHARAT_INIT_PROFILE_CLOUD | BHARAT_INIT_PROFILE_APPLIANCE,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_STORAGE,
        .executable_desc = { .executable_handle = 0x53544F01ULL, .module_name = "services/storagemgr" },
    },
    {
        .id = INIT_SVC_ACCELMGR,
        .name = "accelmgr",
        .boot_class = BOOT_CLASS_LATE,
        .start_deadline_ms = 800,
        .ready_deadline_ms = 4000,
        .start_fn = spawn_service,
        .probe_fn = NULL,
        .bootstrap_hint_fn = NULL,
        .rollback_fn = stub_rollback,
        .deps = deps_accelmgr,
        .dep_count = 1,
        .retry_limit = 0,
        .policy = INIT_SERVICE_OPTIONAL,
        .profile_mask = BHARAT_INIT_PROFILE_WATCH | BHARAT_INIT_PROFILE_MOBILE |
                        BHARAT_INIT_PROFILE_TV | BHARAT_INIT_PROFILE_DESKTOP,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_SENSORS,
        .executable_desc = { .executable_handle = 0x41434301ULL, .module_name = "services/accelmgr" },
    },
    {
        .id = INIT_SVC_APP_PAYLOAD,
        .name = "app_payload",
        .boot_class = BOOT_CLASS_LATE,
        .start_deadline_ms = 1000,
        .ready_deadline_ms = 5000,
        .start_fn = spawn_service,
        .probe_fn = NULL,
        .bootstrap_hint_fn = NULL,
        .rollback_fn = stub_rollback,
        .deps = NULL,
        .dep_count = 0,
        .retry_limit = 0,
        .policy = INIT_SERVICE_OPTIONAL,
        .profile_mask = BHARAT_INIT_PROFILE_TINY,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_NONE,
        .executable_desc = { .executable_handle = 0x41505001ULL, .module_name = "services/app_payload" },
    }
};

const size_t g_init_manifest_count = sizeof(g_init_manifest) / sizeof(g_init_manifest[0]);
