#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "bharat/idl/capwire.h"

void test_capwire_roundtrip() {
    uint8_t buffer[128];
    bharat_msg_builder_t b;
    bharat_msg_builder_init(&b, buffer, sizeof(buffer), 0);

    bharat_capwire_desc_t original = {0};
    original.cap_type = 42; // arbitrary
    original.transfer_mode = BHARAT_CAP_XFER_BORROW;
    original.rights_mask = 0xFFFF;
    original.origin_node = 1;
    original.issuer_id = 999;
    original.object_id = 0x1234567812345678ULL;
    original.nonce = 0xABCDEFABCDEFABCDULL;
    original.generation = 5;

    assert(bharat_capwire_encode(&b, &original) == BHARAT_MSG_OK);
    assert(b.off == 34);

    bharat_msg_reader_t r;
    bharat_msg_reader_init(&r, buffer, b.off, 0);

    bharat_capwire_desc_t decoded = {0};
    assert(bharat_capwire_decode(&r, &decoded) == BHARAT_MSG_OK);

    assert(decoded.cap_type == original.cap_type);
    assert(decoded.transfer_mode == original.transfer_mode);
    assert(decoded.rights_mask == original.rights_mask);
    assert(decoded.origin_node == original.origin_node);
    assert(decoded.issuer_id == original.issuer_id);
    assert(decoded.object_id == original.object_id);
    assert(decoded.nonce == original.nonce);
    assert(decoded.generation == original.generation);
}

void test_capwire_validation() {
    bharat_capwire_desc_t desc = {0};
    desc.cap_type = 1;
    desc.transfer_mode = BHARAT_CAP_XFER_COPY;
    desc.origin_node = 1;
    desc.object_id = 1;
    desc.nonce = 1;

    // Baseline OK
    assert(bharat_capwire_validate(&desc) == BHARAT_MSG_OK);

    // Invalid Transfer Mode
    desc.transfer_mode = 99;
    assert(bharat_capwire_validate(&desc) == BHARAT_MSG_ERR_INVALID_CAP);
    desc.transfer_mode = BHARAT_CAP_XFER_COPY;

    // Missing Origin
    desc.origin_node = 0;
    assert(bharat_capwire_validate(&desc) == BHARAT_MSG_ERR_INVALID_CAP);
    desc.origin_node = 1;

    // Missing Object
    desc.object_id = 0;
    assert(bharat_capwire_validate(&desc) == BHARAT_MSG_ERR_INVALID_CAP);
    desc.object_id = 1;

    // Missing Nonce
    desc.nonce = 0;
    assert(bharat_capwire_validate(&desc) == BHARAT_MSG_ERR_UNAUTHORIZED);
}

void test_capwire_attenuation() {
    bharat_capwire_desc_t desc = {0};
    desc.rights_mask = 0x0F;

    // Strip rights 0x0C, keeping 0x03
    bharat_capwire_attenuate(&desc, 0x03);

    assert(desc.rights_mask == 0x03);
}

int main() {
    printf("Running Capwire tests...\n");
    test_capwire_roundtrip();
    test_capwire_validation();
    test_capwire_attenuation();
    printf("All Capwire tests passed!\n");
    return 0;
}
