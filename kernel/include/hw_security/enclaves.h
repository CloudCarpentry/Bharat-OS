#ifndef BHARAT_ENCLAVE_H
#define BHARAT_ENCLAVE_H

#include <stdint.h>
#include "../mm.h"

/*
 * Bharat-OS Hardware Security Enclave Abstraction
 * Wraps Intel SGX, ARM TrustZone, and RISC-V MultiZone to provide a unified
 * API for creating impenetrable memory regions for cryptographic operations
 * and AI Neural Network Weight protection.
 */

// Supported underlying enclave hardware layers
typedef enum {
    ENCLAVE_HARDWARE_NONE = 0,
    ENCLAVE_HARDWARE_INTEL_SGX = 1,
    ENCLAVE_HARDWARE_ARM_TRUSTZONE = 2,
    ENCLAVE_HARDWARE_TPM_BACKED = 3
} enclave_type_t;

typedef struct {
    uint32_t enclave_id;
    enclave_type_t hardware_type;
    phys_addr_t base_physical_address;
    uint32_t size_bytes;
    
    // Remote attestation cryptographic quote confirming hardware authenticity
    uint8_t attestation_quote[256];
} security_enclave_t;

// Request the CPU hardware (e.g. via SGX ENCLS instruction) to carve out 
// a dedicated, encrypted EPC (Enclave Page Cache) memory region.
int enclave_create(uint32_t size, security_enclave_t* out_enclave);

// Load an AI Model Weight file or Cryptographic Key statically into the TrustZone
int enclave_load_data(security_enclave_t* enclave, void* secret_data, uint32_t size);

// Destroy the enclave and securely erase the EPC memory pages
void enclave_destroy(security_enclave_t* enclave);

#endif // BHARAT_ENCLAVE_H
