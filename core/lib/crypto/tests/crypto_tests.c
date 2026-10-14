#include <psa/crypto.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

static int tests_run = 0;
static int tests_passed = 0;

#define RUN_TEST(test) do { \
    tests_run++; \
    if (test()) { \
        tests_passed++; \
    } \
} while(0)

bool test_sha256(void) {
    psa_status_t status;
    const uint8_t msg[] = "abc";
    uint8_t hash[32];
    size_t hash_len;
    const uint8_t expected_hash[] = {
        0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
        0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
        0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
        0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
    };
    status = psa_hash_compute(PSA_ALG_SHA_256, msg, sizeof(msg)-1, hash, sizeof(hash), &hash_len);
    if (status != PSA_SUCCESS || hash_len != 32 || memcmp(hash, expected_hash, 32) != 0) return false;
    return true;
}

bool test_hmac_sha256(void) {
    psa_status_t status;
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_key_id_t key_id;
    const uint8_t key_data[20] = { 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b };
    const uint8_t msg[] = "Hi There";
    uint8_t mac[32];
    size_t mac_len;
    const uint8_t expected_mac[] = {
        0xb0, 0x34, 0x4c, 0x61, 0xd8, 0xdb, 0x38, 0x53,
        0x5c, 0xa8, 0xaf, 0xce, 0xaf, 0x0b, 0xf1, 0x2b,
        0x88, 0x1d, 0xc2, 0x00, 0xc9, 0x83, 0x3d, 0xa7,
        0x26, 0xe9, 0x37, 0x6c, 0x2e, 0x32, 0xcf, 0xf7
    };
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_VERIFY_MESSAGE);
    psa_set_key_algorithm(&attributes, PSA_ALG_HMAC(PSA_ALG_SHA_256));
    psa_set_key_type(&attributes, PSA_KEY_TYPE_HMAC);
    psa_set_key_bits(&attributes, sizeof(key_data) * 8);
    status = psa_import_key(&attributes, key_data, sizeof(key_data), &key_id);
    if (status != PSA_SUCCESS) return false;
    status = psa_mac_compute(key_id, PSA_ALG_HMAC(PSA_ALG_SHA_256), msg, sizeof(msg)-1, mac, sizeof(mac), &mac_len);
    if (status != PSA_SUCCESS || mac_len != 32 || memcmp(mac, expected_mac, 32) != 0) {
        psa_destroy_key(key_id);
        return false;
    }
    psa_destroy_key(key_id);
    return true;
}

bool test_aes_gcm(void) {
    psa_status_t status;
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_key_id_t key_id;
    const uint8_t key_data[16] = {0};
    const uint8_t nonce[12] = {0};
    const uint8_t plain[] = {0, 0, 0, 0};
    uint8_t cipher_and_tag[4 + 16];
    size_t out_len;
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
    psa_set_key_algorithm(&attributes, PSA_ALG_GCM);
    psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attributes, 128);
    status = psa_import_key(&attributes, key_data, sizeof(key_data), &key_id);
    if (status != PSA_SUCCESS) return false;
    status = psa_aead_encrypt(key_id, PSA_ALG_GCM, nonce, sizeof(nonce), NULL, 0, plain, sizeof(plain), cipher_and_tag, sizeof(cipher_and_tag), &out_len);
    if (status != PSA_SUCCESS) {
        psa_destroy_key(key_id);
        return false;
    }
    psa_destroy_key(key_id);
    return true;
}

bool test_missing_entropy(void) {
    psa_status_t status;
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_key_id_t key_id;
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
    psa_set_key_algorithm(&attributes, PSA_ALG_GCM);
    psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attributes, 128);
    status = psa_generate_key(&attributes, &key_id);
    if (status != PSA_ERROR_INSUFFICIENT_ENTROPY) {
        if (status == PSA_SUCCESS) psa_destroy_key(key_id);
        return false;
    }
    return true;
}

int main(void) {
    psa_status_t status = psa_crypto_init();
    if (status != PSA_SUCCESS) return 1;

    RUN_TEST(test_sha256);
    RUN_TEST(test_hmac_sha256);
    RUN_TEST(test_aes_gcm);
    RUN_TEST(test_missing_entropy);

    return (tests_passed == tests_run) ? 0 : 1;
}
