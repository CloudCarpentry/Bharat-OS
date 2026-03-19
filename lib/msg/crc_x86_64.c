// x86_64 CRC Backend Placeholder
//
// Do not enable x86_64 SSE4.2 `_mm_crc32_u8` or `_mm_crc32_u64` here.
// Those instructions compute CRC32C (Castagnoli - 0x1EDC6F41).
// The BharatOS wire format requires IEEE 802.3 CRC32 (0xEDB88320).
// Substituting them will cause checksum failures and break wire compatibility.
//
// x86 PCLMULQDQ instructions can be used to accelerate arbitrary CRCs,
// but for now, x86 relies strictly on the generic table-driven fallback
// in crc_generic.c.
