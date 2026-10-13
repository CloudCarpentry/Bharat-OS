#ifndef DEVMGR_CORE_H
#define DEVMGR_CORE_H

#include <bharat/uapi/service_status.h>
#include <bharat/service/service_runtime.h>

void devmgr_registry_init(void);
bharat_status_t devmgr_handle_message(bh_service_ctx_t *ctx, const bh_msg_t *msg);

#endif // DEVMGR_CORE_H