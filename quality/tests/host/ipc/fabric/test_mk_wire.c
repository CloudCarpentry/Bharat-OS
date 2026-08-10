#include <assert.h>
#include <stdio.h>
#include "ipc/mk_proto.h"
#include "fake_hal.h"

static void test_mk_wire_golden_vector(void) {
    bh_mk_wire_header_v1_t hdr = {0};
    hdr.abi_version = 1;
    hdr.header_size = 96;
    hdr.message_class = 2;
    hdr.opcode = 3;
    hdr.flags = 4;
    hdr.payload_size = 5;
    hdr.src_core = 6;
    hdr.dst_core = 7;
    hdr.src_endpoint = 8;
    hdr.dst_endpoint = 9;
    hdr.sequence = 10;
    hdr.deadline_ticks = 11;

    // Encode
    bh_mk_wire_encode(&hdr);

    // Verify raw byte vector in little endian!
    uint8_t *bytes = (uint8_t*)&hdr;
    assert(bytes[0] == 0x01); // abi_version
    assert(bytes[1] == 0x00);
    assert(bytes[2] == 96);   // header_size
    assert(bytes[3] == 0x00);
    assert(bytes[4] == 0x02); // message_class
    assert(bytes[5] == 0x00);
    assert(bytes[6] == 0x03); // opcode
    assert(bytes[7] == 0x00);

    // Decode and verify symmetric recovery
    bh_mk_wire_decode(&hdr);
    assert(hdr.abi_version == 1);
    assert(hdr.header_size == 96);
    assert(hdr.message_class == 2);
    assert(hdr.opcode == 3);

    printf("  -> Golden-byte-vector PASSED\n");
}

static void test_mk_handle_packing(void) {
    uint64_t handle;
    kstatus_t st = bh_mk_handle_pack(12, 34, 56, 4, &handle);
    assert(st == K_OK);

    uint32_t core_id, core_gen, ep_gen, slot;
    st = bh_mk_handle_unpack(handle, &core_id, &core_gen, &ep_gen, &slot);
    assert(st == K_OK);
    assert(core_id == 12);
    assert(core_gen == 34);
    assert(ep_gen == 56);
    assert(slot == 4);

    // Test truncation or boundary values
    st = bh_mk_handle_pack(4096, 1, 1, 1, &handle); // Core ID too big
    assert(st == K_ERR_INVALID_ARG);

    printf("  -> Handle packing/unpacking PASSED\n");
}

int main(void) {
    printf("Running test_mk_wire...\n");

    assert(sizeof(bh_mk_wire_header_v1_t) == 96);
    assert(__builtin_offsetof(bh_mk_wire_header_v1_t, abi_version) == 0);
    assert(__builtin_offsetof(bh_mk_wire_header_v1_t, header_size) == 2);
    assert(__builtin_offsetof(bh_mk_wire_header_v1_t, sequence) == 64);
    assert(__builtin_offsetof(bh_mk_wire_header_v1_t, reserved) == 92);

    test_mk_wire_golden_vector();
    test_mk_handle_packing();

    printf("test_mk_wire PASSED\n");
    return 0;
}
