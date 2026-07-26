#include "../../../include/kernel.h"
#include "../../../services/core/subsysmgr/include/bharat/msg/transport.h"
#include "bharat_monitor_v1_types.h"
#include "../../../services/core/subsysmgr/include/bharat/msg/wire.h"
#include "hal/hal.h"

// The opcode for TlbInvalidate
#define OP_TLBINVALIDATE 3
// The service ID for bharat.monitor.v1
#define SERVICE_ID_MONITOR_V1 1

int bharat_monitor_v1_send_tlb_invalidate(
    bharat_transport_t* t,
    int dst,
    const bharat_monitor_v1_TlbInvalidateReq_t* req,
    uint64_t reqid,
    void* ctx)
{
    (void)ctx;
    if (!t || !t->ops || !t->ops->send) {
        return BHARAT_MSG_ERR_TRANSPORT_FAIL;
    }

    uint8_t buffer[256];
    bharat_msg_header_t hdr = {0};

    hdr.version_major = BHARAT_MSG_VERSION_MAJOR;
    hdr.version_minor = BHARAT_MSG_VERSION_MINOR;
    hdr.header_len    = BHARAT_MSG_HEADER_MIN_LEN;
    hdr.service_id    = SERVICE_ID_MONITOR_V1;
    hdr.opcode        = OP_TLBINVALIDATE;
    hdr.flags         = BHARAT_MSG_FLAG_REQUEST;
    hdr.request_id    = reqid; // Use correct transaction request_id
    hdr.src_node      = hal_cpu_get_id();
    hdr.dst_node      = dst;
    hdr.total_len     = BHARAT_MSG_HEADER_MIN_LEN + sizeof(bharat_monitor_v1_TlbInvalidateReq_t);

    if (hdr.total_len > sizeof(buffer)) {
        return BHARAT_MSG_ERR_TOO_LARGE;
    }

    int ret = bharat_msg_header_encode(&hdr, buffer, sizeof(buffer));
    if (ret != BHARAT_MSG_OK) {
        return ret;
    }

    // Since we don't have a generated encoder, we manually copy the struct payload.
    uint8_t* payload = buffer + BHARAT_MSG_HEADER_MIN_LEN;

    // Manual packing
    bharat_store_le64(payload + 0, req->aspace_id);
    bharat_store_le64(payload + 8, req->va_start);
    bharat_store_le64(payload + 16, req->length);
    bharat_store_le32(payload + 24, req->type);
    bharat_store_le32(payload + 28, req->generation);

    ret = t->ops->send(t, buffer, hdr.total_len);
    if (ret < 0) {
        return ret;
    }

    return 0; // Success
}

int bharat_monitor_v1_call_tlb_invalidate(
    bharat_transport_t* t,
    int dst,
    const bharat_monitor_v1_TlbInvalidateReq_t* req,
    void* ctx)
{
    // Keeping for backward compatibility wrapper if needed, using req->generation as reqid
    return bharat_monitor_v1_send_tlb_invalidate(t, dst, req, (uint64_t)req->generation, ctx);
}
