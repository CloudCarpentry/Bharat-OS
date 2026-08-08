#include "kernel/status.h"
#include "../../lib/msg/crc.h"
#include "console/console_core.h"
#include <string.h>
#include "tests/ktest.h"

extern uint32_t bharat_msg_crc32_generic(const uint8_t *data, size_t len);

static const struct {
    const char* data;
    uint32_t expected_crc;
} test_vectors[] = {
    {"", 0x00000000},
    {"123456789", 0xCBF43926},
    {"a", 0xE8B7BE43},
    {"ab", 0x9E83486D},
    {"abc", 0x352441C2},
    {"abcd", 0xED82CD11},
    {"abcde", 0x8587D865},
    {"abcdef", 0x4B8E39EF},
    {"abcdefg", 0x312A6AA6},
    {"abcdefgh", 0xAEF2A478},
    {"abcdefghi", 0x8DA988AF},
    {"abcdefghij", 0x3981703A},
    {"abcdefghijk", 0x6B9CDFE7},
    {"abcdefghijkl", 0x22DCBCA0},
    {"abcdefghijklm", 0x762025D8},
    {"abcdefghijklmn", 0x8AA40D55},
    {"abcdefghijklmno", 0xD6F4FA68},
    {"abcdefghijklmnop", 0xB7D09E6D}
};

int test_crc32_correctness(void) {
    int failures = 0;
    for (size_t i = 0; i < sizeof(test_vectors) / sizeof(test_vectors[0]); i++) {
        const uint8_t* data = (const uint8_t*)test_vectors[i].data;
        size_t len = strlen(test_vectors[i].data);
        uint32_t crc_hw = bharat_msg_crc32(data, len);
        uint32_t crc_sw = bharat_msg_crc32_generic(data, len);

        if (crc_hw != test_vectors[i].expected_crc) {
            console_log(CONSOLE_LEVEL_ERROR, "CRC32 failed for '%s'. Expected %x, got HW %x\n",
                test_vectors[i].data, test_vectors[i].expected_crc, crc_hw);
            failures++;
        }

        if (crc_hw != crc_sw) {
             console_log(CONSOLE_LEVEL_ERROR, "CRC32 mismatch for '%s'. HW %x != SW %x\n",
                test_vectors[i].data, crc_hw, crc_sw);
             failures++;
        }
    }

    // Also test unaligned inputs
    uint8_t buffer[64];
    for (int i=0; i<64; i++) buffer[i] = i;

    for (int offset = 0; offset < 4; offset++) {
        for (int len = 1; len < 32; len++) {
            uint32_t hw = bharat_msg_crc32(buffer + offset, len);
            uint32_t sw = bharat_msg_crc32_generic(buffer + offset, len);
            if (hw != sw) {
                 console_log(CONSOLE_LEVEL_ERROR, "CRC32 mismatch unaligned offset=%d len=%d. HW %x != SW %x\n",
                    offset, len, hw, sw);
                 failures++;
            }
        }
    }

    return failures == 0 ? 0 : -1;
}

REGISTER_KERNEL_TEST(
    "crc32",
    "msg",
    test_crc32_correctness,
    1,
    0
)
