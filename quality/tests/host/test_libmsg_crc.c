#include "crc.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#define CRC_VECTOR(text, expected)                                             \
  {(const uint8_t *)(text), sizeof(text) - 1u, (expected)}

typedef struct {
  const uint8_t *data;
  size_t length;
  uint32_t expected;
} crc_vector_t;

static const crc_vector_t test_vectors[] = {
    CRC_VECTOR("", 0x00000000u),
    CRC_VECTOR("123456789", 0xCBF43926u),
    CRC_VECTOR("a", 0xE8B7BE43u),
    CRC_VECTOR("abc", 0x352441C2u),
    CRC_VECTOR("abcdefgh", 0xAEEF2A50u),
    CRC_VECTOR("abcdefghijklmnop", 0x943AC093u),
};

int main(void) {
  uint8_t unaligned[64];

  for (size_t i = 0; i < sizeof(test_vectors) / sizeof(test_vectors[0]); ++i) {
    assert(bharat_msg_crc32(test_vectors[i].data, test_vectors[i].length) ==
           test_vectors[i].expected);
    assert(bharat_msg_crc32_generic(test_vectors[i].data,
                                    test_vectors[i].length) ==
           test_vectors[i].expected);
  }

  for (size_t i = 0; i < sizeof(unaligned); ++i) {
    unaligned[i] = (uint8_t)i;
  }
  for (size_t offset = 0; offset < 4u; ++offset) {
    for (size_t length = 1; length < 32u; ++length) {
      assert(bharat_msg_crc32(unaligned + offset, length) ==
             bharat_msg_crc32_generic(unaligned + offset, length));
    }
  }

  return 0;
}
