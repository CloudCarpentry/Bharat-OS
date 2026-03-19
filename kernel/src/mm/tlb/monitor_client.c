#include "../../../include/kernel.h"
#include "../../../subsys/include/bharat/msg/transport.h"
#include "../../../services/monitor/generated/bharat_monitor_v1_types.h"
#include "../../../subsys/include/bharat/msg/wire.h"

// The opcode for TlbInvalidate
#define OP_TLBINVALIDATE 3
// The service ID for bharat.monitor.v1
#define SERVICE_ID_MONITOR_V1 1

int bharat_monitor_v1_call_tlb_invalidate(
    bharat_transport_t* t,
    int dst,
    const bharat_monitor_v1_TlbInvalidateReq_t* req,
    void* ctx)
{
    if (!t || !t->ops || !t->ops->send || !t->ops->recv) {
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
    hdr.request_id    = req->generation; // Use generation as sequence
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
    // Assuming little-endian local architecture for this prototype, or just flat copy.
    // In a strict implementation, we'd use bharat_store_leXX.
    // For simplicity of this minimal required path, we do a raw copy.
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

    // Wait for response synchronously
    uint32_t wait_count = 0;
    while (wait_count < 100000) { // simple bounded wait / timeout
        if (t->ops->poll) {
            t->ops->poll(t, 1);
        }

        size_t rx_len = 0;
        ret = t->ops->recv(t, buffer, sizeof(buffer), &rx_len);
        if (ret == BHARAT_MSG_OK && rx_len >= BHARAT_MSG_HEADER_MIN_LEN) {
            bharat_msg_header_t rx_hdr;
            int dec_ret = bharat_msg_header_decode(buffer, rx_len, &rx_hdr);
            if (dec_ret == BHARAT_MSG_OK) {
                // Check if it's the response for our request
                if (rx_hdr.service_id == SERVICE_ID_MONITOR_V1 &&
                    rx_hdr.opcode == OP_TLBINVALIDATE &&
                    bharat_msg_is_response(rx_hdr.flags) &&
                    rx_hdr.request_id == req->generation) {

                    // Decode payload
                    if (rx_len >= BHARAT_MSG_HEADER_MIN_LEN + sizeof(bharat_monitor_v1_TlbInvalidateResp_t)) {
                        uint8_t* rx_payload = buffer + BHARAT_MSG_HEADER_MIN_LEN;
                        bharat_monitor_v1_TlbInvalidateResp_t resp;
                        resp.status = bharat_load_le32(rx_payload);

                        // We got our success
                        return resp.status;
                    }
                }
            }
        }

        // Let CPU relax
        __asm__ volatile("nop");
        wait_count++;
    }

    // Timeout
    return BHARAT_MSG_ERR_TIMEOUT;
}
