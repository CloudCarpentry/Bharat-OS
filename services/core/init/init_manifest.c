#include "init_manifest.h"
#include <stddef.h>

// Stubs for start functions
static int stub_start(void *ctx) {
    (void)ctx;
    return 0; // Success
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
        .start_fn = stub_start,
        .probe_fn = NULL,
        .bootstrap_hint_fn = NULL,
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
    },
    {
        .id = INIT_SVC_PROCESS_MANAGER,
        .name = "process_manager",
        .boot_class = BOOT_CLASS_CORE,
        .start_deadline_ms = 1000,
        .ready_deadline_ms = 5000,
        .start_fn = stub_start,
        .probe_fn = NULL,
        .bootstrap_hint_fn = NULL,
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
    },
    {
        .id = INIT_SVC_VM_MANAGER,
        .name = "vm_manager",
        .boot_class = BOOT_CLASS_CORE,
        .start_deadline_ms = 1000,
        .ready_deadline_ms = 5000,
        .start_fn = stub_start,
        .probe_fn = NULL,
        .bootstrap_hint_fn = NULL,
        .deps = deps_vm_manager,
        .dep_count = 2,
        .retry_limit = 3,
        .policy = INIT_SERVICE_OPTIONAL,
        .profile_mask = BHARAT_INIT_PROFILE_EMBEDDED_RICH | BHARAT_INIT_PROFILE_MOBILE |
                        BHARAT_INIT_PROFILE_DESKTOP | BHARAT_INIT_PROFILE_CLOUD,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_MMU,
    },
    {
        .id = INIT_SVC_DEVMGR,
        .name = "devmgr",
        .boot_class = BOOT_CLASS_INFRA,
        .start_deadline_ms = 1000,
        .ready_deadline_ms = 5000,
        .start_fn = stub_start,
        .probe_fn = NULL,
        .bootstrap_hint_fn = NULL,
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
    },
    {
        .id = INIT_SVC_FAULTMGR,
        .name = "faultmgr",
        .boot_class = BOOT_CLASS_INFRA,
        .start_deadline_ms = 1000,
        .ready_deadline_ms = 5000,
        .start_fn = stub_start,
        .probe_fn = NULL,
        .bootstrap_hint_fn = NULL,
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
    },
    {
        .id = INIT_SVC_SERVICEMGR,
        .name = "servicemgr",
        .boot_class = BOOT_CLASS_INFRA,
        .start_deadline_ms = 1000,
        .ready_deadline_ms = 5000,
        .start_fn = stub_start,
        .probe_fn = NULL,
        .bootstrap_hint_fn = NULL,
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
    },
    {
        .id = INIT_SVC_BOOT_DISPLAYD,
        .name = "boot_displayd",
        .boot_class = BOOT_CLASS_LATE,
        .start_deadline_ms = 1000,
        .ready_deadline_ms = 5000,
        .start_fn = stub_start,
        .probe_fn = NULL,
        .bootstrap_hint_fn = NULL,
        .deps = deps_boot_displayd,
        .dep_count = 1,
        .retry_limit = 2,
        .policy = INIT_SERVICE_OPTIONAL,
        .profile_mask = BHARAT_INIT_PROFILE_MOBILE | BHARAT_INIT_PROFILE_DESKTOP |
                        BHARAT_INIT_PROFILE_TV | BHARAT_INIT_PROFILE_WATCH,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_DISPLAY,
    },
    {
        .id = INIT_SVC_TELEMETRYMGR,
        .name = "telemetrymgr",
        .boot_class = BOOT_CLASS_OPTIONAL,
        .start_deadline_ms = 1000,
        .ready_deadline_ms = 5000,
        .start_fn = stub_start,
        .probe_fn = NULL,
        .bootstrap_hint_fn = NULL,
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
    },
    {
        .id = INIT_SVC_STORAGEMGR,
        .name = "storagemgr",
        .boot_class = BOOT_CLASS_INFRA,
        .start_deadline_ms = 1200,
        .ready_deadline_ms = 5000,
        .start_fn = stub_start,
        .probe_fn = NULL,
        .bootstrap_hint_fn = NULL,
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
    },
    {
        .id = INIT_SVC_ACCELMGR,
        .name = "accelmgr",
        .boot_class = BOOT_CLASS_LATE,
        .start_deadline_ms = 800,
        .ready_deadline_ms = 4000,
        .start_fn = stub_start,
        .probe_fn = NULL,
        .bootstrap_hint_fn = NULL,
        .deps = deps_accelmgr,
        .dep_count = 1,
        .retry_limit = 0,
        .policy = INIT_SERVICE_OPTIONAL,
        .profile_mask = BHARAT_INIT_PROFILE_WATCH | BHARAT_INIT_PROFILE_MOBILE |
                        BHARAT_INIT_PROFILE_TV | BHARAT_INIT_PROFILE_DESKTOP,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_SENSORS,
    },
    {
        .id = INIT_SVC_APP_PAYLOAD,
        .name = "app_payload",
        .boot_class = BOOT_CLASS_LATE,
        .start_deadline_ms = 1000,
        .ready_deadline_ms = 5000,
        .start_fn = stub_start,
        .probe_fn = NULL,
        .bootstrap_hint_fn = NULL,
        .deps = NULL,
        .dep_count = 0,
        .retry_limit = 0,
        .policy = INIT_SERVICE_OPTIONAL,
        .profile_mask = BHARAT_INIT_PROFILE_TINY,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_NONE,
    }
};

const size_t g_init_manifest_count = sizeof(g_init_manifest) / sizeof(g_init_manifest[0]);
