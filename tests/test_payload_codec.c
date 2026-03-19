#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "bharat/msg/payload.h"

void test_primitives_roundtrip() {
    uint8_t buffer[128];
    bharat_msg_builder_t b;
    bharat_msg_builder_init(&b, buffer, sizeof(buffer), 0);

    bharat_msg_build_u8(&b, 0x42);
    bharat_msg_build_u16(&b, 0x1234);
    bharat_msg_build_u32(&b, 0x87654321);
    bharat_msg_build_u64(&b, 0xDEADBEEFCAFEBAB0ULL);
    bharat_msg_build_bool(&b, true);
    bharat_msg_build_bool(&b, false);

    assert(b.off == 1 + 2 + 4 + 8 + 1 + 1);

    bharat_msg_reader_t r;
    bharat_msg_reader_init(&r, buffer, b.off, 0);

    uint8_t v8;
    uint16_t v16;
    uint32_t v32;
    uint64_t v64;
    bool b1, b2;

    assert(bharat_msg_read_u8(&r, &v8) == BHARAT_MSG_OK);
    assert(v8 == 0x42);

    assert(bharat_msg_read_u16(&r, &v16) == BHARAT_MSG_OK);
    assert(v16 == 0x1234);

    assert(bharat_msg_read_u32(&r, &v32) == BHARAT_MSG_OK);
    assert(v32 == 0x87654321);

    assert(bharat_msg_read_u64(&r, &v64) == BHARAT_MSG_OK);
    assert(v64 == 0xDEADBEEFCAFEBAB0ULL);

    assert(bharat_msg_read_bool(&r, &b1) == BHARAT_MSG_OK);
    assert(b1 == true);

    assert(bharat_msg_read_bool(&r, &b2) == BHARAT_MSG_OK);
    assert(b2 == false);

    assert(r.off == b.off);
}

void test_bounded_strings_roundtrip() {
    uint8_t buffer[128];
    bharat_msg_builder_t b;
    bharat_msg_builder_init(&b, buffer, sizeof(buffer), 0);

    const char* original = "Hello, Bharat-OS!";
    bharat_msg_build_string(&b, original);

    bharat_msg_reader_t r;
    bharat_msg_reader_init(&r, buffer, b.off, 0);

    char decoded[32];
    uint32_t len = 0;

    // Read the string bounded to a capacity of 32
    assert(bharat_msg_read_string_bounded(&r, decoded, 32, &len) == BHARAT_MSG_OK);
    assert(len == strlen(original));
    assert(strcmp(decoded, original) == 0);
}

void test_buffer_overflow_protection() {
    uint8_t buffer[8];
    bharat_msg_builder_t b;
    bharat_msg_builder_init(&b, buffer, sizeof(buffer), 0);

    assert(bharat_msg_build_u64(&b, 0) == BHARAT_MSG_OK);
    // Over the edge!
    assert(bharat_msg_build_u8(&b, 0) == BHARAT_MSG_ERR_BUFFER_OVERFLOW);
}

void test_truncated_decode_protection() {
    uint8_t buffer[4];
    bharat_msg_builder_t b;
    bharat_msg_builder_init(&b, buffer, sizeof(buffer), 0);

    // Encode a u32 length of 100
    bharat_msg_build_u32(&b, 100);

    bharat_msg_reader_t r;
    bharat_msg_reader_init(&r, buffer, 4, 0);

    uint8_t out_buf[128];
    uint32_t out_len = 0;
    // Buffer only has 4 bytes (the length header), but length header claims 100 more bytes follow.
    assert(bharat_msg_read_bytes_bounded(&r, out_buf, sizeof(out_buf), &out_len) == BHARAT_MSG_ERR_TRUNCATED);
}

int main() {
    printf("Running Payload Codec tests...\n");
    test_primitives_roundtrip();
    test_bounded_strings_roundtrip();
    test_buffer_overflow_protection();
    test_truncated_decode_protection();
    printf("All Payload Codec tests passed!\n");
    return 0;
}
