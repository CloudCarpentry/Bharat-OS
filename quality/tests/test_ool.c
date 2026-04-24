#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "bharat/idl/ool.h"

void test_ool_roundtrip() {
    uint8_t buffer[64];
    bharat_msg_builder_t b;
    bharat_msg_builder_init(&b, buffer, sizeof(buffer), 0);

    bharat_ool_desc_t original = {0};
    original.desc_type = BHARAT_OOL_TYPE_SHMEM;
    original.flags = BHARAT_OOL_FLAG_READ_ONLY | BHARAT_OOL_FLAG_VOLATILE;
    original.region_id = 0x8877665544332211ULL;
    original.offset = 4096;
    original.length = 8192;

    assert(bharat_ool_encode(&b, &original) == BHARAT_MSG_OK);
    // 26 bytes: u8 + u8 + u64 + u64 + u64
    assert(b.off == 26);

    bharat_msg_reader_t r;
    bharat_msg_reader_init(&r, buffer, b.off, 0);

    bharat_ool_desc_t decoded = {0};
    assert(bharat_ool_decode(&r, &decoded) == BHARAT_MSG_OK);

    assert(decoded.desc_type == original.desc_type);
    assert(decoded.flags == original.flags);
    assert(decoded.region_id == original.region_id);
    assert(decoded.offset == original.offset);
    assert(decoded.length == original.length);
}

void test_ool_validation() {
    bharat_ool_desc_t desc = {0};
    desc.desc_type = BHARAT_OOL_TYPE_DMA;
    desc.region_id = 100;
    desc.length = 500;
    assert(bharat_ool_validate(&desc) == BHARAT_MSG_OK);

    // Invalid Type
    desc.desc_type = 99;
    assert(bharat_ool_validate(&desc) == BHARAT_MSG_ERR_MALFORMED_PAYLOAD);
    desc.desc_type = BHARAT_OOL_TYPE_DMA;

    // Zero Region ID
    desc.region_id = 0;
    assert(bharat_ool_validate(&desc) == BHARAT_MSG_ERR_MALFORMED_PAYLOAD);
    desc.region_id = 100;

    // Zero length
    desc.length = 0;
    assert(bharat_ool_validate(&desc) == BHARAT_MSG_ERR_MALFORMED_PAYLOAD);
}

int main() {
    printf("Running OOL tests...\n");
    test_ool_roundtrip();
    test_ool_validation();
    printf("All OOL tests passed!\n");
    return 0;
}
