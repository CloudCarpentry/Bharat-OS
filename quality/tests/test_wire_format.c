#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "bharat/msg/wire.h"
#include "bharat/msg/validate.h"

// Forward-declare CRC for tests
extern uint32_t bharat_msg_crc32(const uint8_t *data, size_t len);

void test_endian_helpers() {
    uint8_t buf[8];
    bharat_store_le16(buf, 0x1234);
    assert(buf[0] == 0x34 && buf[1] == 0x12);
    assert(bharat_load_le16(buf) == 0x1234);

    bharat_store_le32(buf, 0x12345678);
    assert(buf[0] == 0x78 && buf[1] == 0x56 && buf[2] == 0x34 && buf[3] == 0x12);
    assert(bharat_load_le32(buf) == 0x12345678);

    bharat_store_le64(buf, 0x1122334455667788ULL);
    assert(buf[0] == 0x88 && buf[7] == 0x11);
    assert(bharat_load_le64(buf) == 0x1122334455667788ULL);
}

void test_crc32() {
    const char* text = "123456789";
    uint32_t crc = bharat_msg_crc32((const uint8_t*)text, 9);
    assert(crc == 0xCBF43926);
}

void test_header_encode_decode() {
    bharat_msg_header_t in_hdr = {0};
    in_hdr.version_major = BHARAT_MSG_VERSION_MAJOR;
    in_hdr.version_minor = 1;
    in_hdr.header_len    = BHARAT_MSG_HEADER_MIN_LEN;
    in_hdr.service_id    = 42;
    in_hdr.opcode        = 7;
    in_hdr.flags         = BHARAT_MSG_FLAG_REQUEST | BHARAT_MSG_FLAG_RELIABLE;
    in_hdr.total_len     = BHARAT_MSG_HEADER_MIN_LEN + 100;
    in_hdr.request_id    = 0xDEADBEEFCAFEBABEULL;
    in_hdr.src_node      = 1001;
    in_hdr.dst_node      = 1002;
    in_hdr.cap_count     = 0;
    in_hdr.desc_count    = 0;
    in_hdr.header_crc    = 0; // Filled later

    uint8_t buf[128];
    memset(buf, 0xAA, sizeof(buf));

    int rc = bharat_msg_header_encode(&in_hdr, buf, sizeof(buf));
    assert(rc == BHARAT_MSG_OK);

    // Compute CRC on raw wire bytes (excluding the CRC field itself at offset 0x28)
    in_hdr.header_crc = bharat_msg_crc32(buf, 0x28);
    bharat_store_le32(buf + 0x28, in_hdr.header_crc);

    bharat_msg_header_t out_hdr = {0};
    rc = bharat_msg_header_decode(buf, sizeof(buf), &out_hdr);
    assert(rc == BHARAT_MSG_OK);

    assert(out_hdr.version_major == in_hdr.version_major);
    assert(out_hdr.header_len == in_hdr.header_len);
    assert(out_hdr.service_id == in_hdr.service_id);
    assert(out_hdr.flags == in_hdr.flags);
    assert(out_hdr.request_id == in_hdr.request_id);
    assert(out_hdr.header_crc == in_hdr.header_crc);

    rc = bharat_msg_header_validate(&out_hdr, 4096);
    assert(rc == BHARAT_MSG_OK);
}

void test_header_validation_failures() {
    bharat_msg_header_t hdr = {0};
    hdr.version_major = BHARAT_MSG_VERSION_MAJOR;
    hdr.header_len = BHARAT_MSG_HEADER_MIN_LEN;
    hdr.flags = BHARAT_MSG_FLAG_REQUEST;
    hdr.total_len = BHARAT_MSG_HEADER_MIN_LEN + 50;

    // Valid baseline
    assert(bharat_msg_header_validate(&hdr, 4096) == BHARAT_MSG_OK);

    // Test MTU overflow
    assert(bharat_msg_header_validate(&hdr, 32) == BHARAT_MSG_ERR_TOO_LARGE);

    // Test header larger than total_len
    hdr.total_len = BHARAT_MSG_HEADER_MIN_LEN - 10;
    assert(bharat_msg_header_validate(&hdr, 4096) == BHARAT_MSG_ERR_MALFORMED_PAYLOAD);
    hdr.total_len = BHARAT_MSG_HEADER_MIN_LEN + 50;

    // Test missing message kind flag
    hdr.flags = BHARAT_MSG_FLAG_RELIABLE;
    assert(bharat_msg_header_validate(&hdr, 4096) == BHARAT_MSG_ERR_MALFORMED_HEADER);

    // Test multiple message kind flags
    hdr.flags = BHARAT_MSG_FLAG_REQUEST | BHARAT_MSG_FLAG_RESPONSE;
    assert(bharat_msg_header_validate(&hdr, 4096) == BHARAT_MSG_ERR_MALFORMED_HEADER);
    hdr.flags = BHARAT_MSG_FLAG_REQUEST;

    // Test cap bounds exceeding payload
    hdr.flags |= BHARAT_MSG_FLAG_HAS_CAPS;
    hdr.cap_count = 10; // 10 * 34 = 340 bytes
    hdr.total_len = BHARAT_MSG_HEADER_MIN_LEN + 50; // Not enough room!
    assert(bharat_msg_header_validate(&hdr, 4096) == BHARAT_MSG_ERR_MALFORMED_PAYLOAD);
}

int main() {
    printf("Running Wire Format tests...\n");
    test_endian_helpers();
    test_crc32();
    test_header_encode_decode();
    test_header_validation_failures();
    printf("All wire format tests passed!\n");
    return 0;
}
