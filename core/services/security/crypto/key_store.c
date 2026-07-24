#include "bharat/stacks/storage/persistent/keystore.h"
#include <lib/base/string.h>

#define MAX_MOCK_CAPS 32U

static keystore_key_cap_t g_mock_caps[MAX_MOCK_CAPS];
static uint32_t g_mock_cap_count = 0U;

static uint32_t simple_hash(const uint8_t *data, size_t len) {
    uint32_t hash = 5381U;
    for (size_t i = 0; i < len; ++i) {
        hash = ((hash << 5) + hash) + data[i];
    }
    return hash;
}

void key_store_init(void) {
    memset(g_mock_caps, 0, sizeof(g_mock_caps));
    g_mock_cap_count = 0U;
}

void test_keystore_clear(void) {
    key_store_init();
}

void test_keystore_register_cap(const keystore_key_cap_t *cap) {
    if (!cap) return;
    for (uint32_t i = 0; i < MAX_MOCK_CAPS; ++i) {
        if (g_mock_caps[i].cap_handle == cap->cap_handle) {
            g_mock_caps[i] = *cap;
            return;
        }
    }
    if (g_mock_cap_count < MAX_MOCK_CAPS) {
        g_mock_caps[g_mock_cap_count++] = *cap;
    }
}

void test_keystore_revoke_cap(capability_handle_t cap_handle) {
    for (uint32_t i = 0; i < g_mock_cap_count; ++i) {
        if (g_mock_caps[i].cap_handle == cap_handle) {
            g_mock_caps[i].revoked = true;
        }
    }
}

static const keystore_key_cap_t* find_cap(capability_handle_t cap_handle) {
    for (uint32_t i = 0; i < g_mock_cap_count; ++i) {
        if (g_mock_caps[i].cap_handle == cap_handle) {
            return &g_mock_caps[i];
        }
    }
    return NULL;
}

static void zeroize_buffer(void *buf, size_t len) {
    if (!buf) return;
    volatile uint8_t *p = (volatile uint8_t *)buf;
    while (len--) {
        *p++ = 0U;
    }
}

keystore_status_t keystore_aead_seal(
    capability_handle_t key_cap,
    const keystore_aead_request_t *request,
    keystore_aead_response_t *response) {

    if (!request || !response) return KEYSTORE_STATUS_ERR_PARAMETER;

    const keystore_key_cap_t *cap = find_cap(key_cap);
    if (!cap) return KEYSTORE_STATUS_ERR_CAPABILITY;
    if (cap->revoked) return KEYSTORE_STATUS_ERR_REVOKED;

    // Verify operation is allowed (e.g. seal is allowed)
    if (cap->allowed_op != 1 && cap->allowed_op != 3) {
        return KEYSTORE_STATUS_ERR_CAPABILITY;
    }

    // Verify partition UUID matches cap if specified
    bool uuid_all_zero = true;
    for (int i = 0; i < 16; ++i) {
        if (cap->partition_uuid[i] != 0) {
            uuid_all_zero = false;
            break;
        }
    }
    if (!uuid_all_zero) {
        // Find configuration binding context in request AAD to verify match
        if (request->aad_len >= sizeof(config_binding_context_t)) {
            const config_binding_context_t *ctx = (const config_binding_context_t *)request->aad;
            if (memcmp(ctx->partition_uuid, cap->partition_uuid, 16) != 0) {
                return KEYSTORE_STATUS_ERR_CAPABILITY;
            }
        }
    }

    // Allocate key-derived stream
    uint8_t keystream[256];
    uint32_t seed = simple_hash((const uint8_t*)&cap->key_id, sizeof(cap->key_id));
    seed ^= simple_hash(request->nonce, 16);
    seed ^= simple_hash(request->aad, request->aad_len);

    for (uint32_t i = 0; i < request->plaintext_len; ++i) {
        seed = seed * 1103515245U + 12345U;
        keystream[i % 256] = (uint8_t)(seed >> 16);
        response->ciphertext[i] = request->plaintext[i] ^ keystream[i % 256];
    }
    response->ciphertext_len = request->plaintext_len;

    // Compute tag over AAD + plaintext
    uint32_t tag_hash = simple_hash(request->aad, request->aad_len);
    tag_hash ^= simple_hash(request->plaintext, request->plaintext_len);
    tag_hash ^= cap->key_id;

    memset(response->tag, 0, 16);
    memcpy(response->tag, &tag_hash, sizeof(tag_hash));

    // Zeroize sensitive buffers
    zeroize_buffer(keystream, sizeof(keystream));
    seed = 0U;

    return KEYSTORE_STATUS_OK;
}

keystore_status_t keystore_aead_open(
    capability_handle_t key_cap,
    const keystore_aead_request_t *request,
    keystore_aead_response_t *response) {

    if (!request || !response) return KEYSTORE_STATUS_ERR_PARAMETER;

    const keystore_key_cap_t *cap = find_cap(key_cap);
    if (!cap) return KEYSTORE_STATUS_ERR_CAPABILITY;
    if (cap->revoked) return KEYSTORE_STATUS_ERR_REVOKED;

    // Verify open is allowed
    if (cap->allowed_op != 2 && cap->allowed_op != 3) {
        return KEYSTORE_STATUS_ERR_CAPABILITY;
    }

    // Verify partition UUID matches cap if specified
    bool uuid_all_zero = true;
    for (int i = 0; i < 16; ++i) {
        if (cap->partition_uuid[i] != 0) {
            uuid_all_zero = false;
            break;
        }
    }
    if (!uuid_all_zero) {
        if (request->aad_len >= sizeof(config_binding_context_t)) {
            const config_binding_context_t *ctx = (const config_binding_context_t *)request->aad;
            if (memcmp(ctx->partition_uuid, cap->partition_uuid, 16) != 0) {
                return KEYSTORE_STATUS_ERR_CAPABILITY;
            }
        }
    }

    // Decrypt ciphertext to a temporary buffer first to verify authentication tag
    uint8_t temp_plaintext[1024];
    if (request->plaintext_len > 1024) return KEYSTORE_STATUS_ERR_PARAMETER;

    uint8_t keystream[256];
    uint32_t seed = simple_hash((const uint8_t*)&cap->key_id, sizeof(cap->key_id));
    seed ^= simple_hash(request->nonce, 16);
    seed ^= simple_hash(request->aad, request->aad_len);

    for (uint32_t i = 0; i < request->plaintext_len; ++i) {
        seed = seed * 1103515245U + 12345U;
        keystream[i % 256] = (uint8_t)(seed >> 16);
        temp_plaintext[i] = request->plaintext[i] ^ keystream[i % 256];
    }

    // Verify tag
    uint32_t expected_hash = simple_hash(request->aad, request->aad_len);
    expected_hash ^= simple_hash(temp_plaintext, request->plaintext_len);
    expected_hash ^= cap->key_id;

    uint32_t provided_hash;
    memcpy(&provided_hash, response->tag, sizeof(provided_hash));

    if (expected_hash != provided_hash) {
        zeroize_buffer(temp_plaintext, sizeof(temp_plaintext));
        zeroize_buffer(keystream, sizeof(keystream));
        return KEYSTORE_STATUS_ERR_AUTH;
    }

    // Tag matches, write output
    memcpy(response->ciphertext, temp_plaintext, request->plaintext_len);
    response->ciphertext_len = request->plaintext_len;

    // Zeroize sensitive buffers
    zeroize_buffer(temp_plaintext, sizeof(temp_plaintext));
    zeroize_buffer(keystream, sizeof(keystream));
    seed = 0U;

    return KEYSTORE_STATUS_OK;
}
