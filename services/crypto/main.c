#include <stdint.h>
//
#include <stdbool.h>

//#define printf(...)

void *memset(void *s, int c, unsigned long n) {
    unsigned char *p = s;
    while(n--) *p++ = (unsigned char)c;
    return s;
}

#include "crypto/crypto_uapi.h"
#include "crypto_service.h"

// Stubs for IPC
static int register_crypto_endpoint(void) {
    // In a real system, invoke a kernel syscall to create and register an endpoint cap
    return 0;
}

static int receive_crypto_request(crypto_request_msg_t *req, uint32_t *len) {
    // Stub: simulate waiting for message
    (void)req;
    (void)len;
    return -1; // Keep it blocked/failing for the stub loop
}

static void send_crypto_response(const crypto_resp_common_t *resp) {
    // Stub: send response back over URPC/IPC
    (void)resp;
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    //printf("Starting Crypto Service Domain...\n");

    drbg_init();
    key_store_init();

    if (register_crypto_endpoint() != 0) {
        //printf("Failed to register crypto endpoint capability.\n");
        return -1;
    }

    //printf("Crypto service listening.\n");

    while (true) {
        crypto_request_msg_t req;
        uint32_t req_len = 0;

        if (receive_crypto_request(&req, &req_len) != 0) {
            // Wait for next message
            continue;
        }

        crypto_resp_common_t resp = {0};
        resp.header.request_id = req.header.request_id;
        resp.header.opcode = req.header.opcode;

        crypto_status_t status = crypto_protocol_validate(&req, req_len);
        if (status != CRYPTO_STATUS_OK) {
            resp.status = status;
        } else {
            resp.status = crypto_dispatch_request(&req, &resp);
        }

        send_crypto_response(&resp);
    }

    return 0;
}
