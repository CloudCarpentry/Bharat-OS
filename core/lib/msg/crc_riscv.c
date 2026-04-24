// RISC-V CRC Backend Placeholder
//
// RISC-V Zbc extension (Carry-less multiplication) can be used to accelerate
// IEEE 802.3 CRC32, but it requires inline assembly or compiler intrinsics
// dependent on specific compiler versions and `-march=rv64gc_zbc` flags.
//
// For now, RISC-V relies strictly on the generic table-driven fallback
// in crc_generic.c.
