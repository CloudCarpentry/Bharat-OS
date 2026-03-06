#ifndef BHARAT_HW_SECURITY_H
#define BHARAT_HW_SECURITY_H

#include <stdint.h>
#include "sched.h"

/*
 * Bharat-OS Advanced Hardware Security Abstraction (Post-2014 Hardware)
 * Enforces memory and execution safety through CPU features to mitigate 
 * ROP/JOP attacks and unauthorized memory access.
 */

typedef struct {
    // x86_64 specific
    int has_cet; // Control-flow Enforcement Technology (Shadow Stack & IBT)
    int has_mpk; // Memory Protection Keys
    int has_smap;// Supervisor Mode Access Prevention
    int has_smep;// Supervisor Mode Execution Prevention
    
    // ARM AArch64 specific
    int has_pac; // Pointer Authentication
    int has_pan; // Privileged Access Never
    int has_pxn; // Privileged Execute Never
    int has_mte; // Memory Tagging Extension
} cpu_security_features_t;

// Discover available security extensions on the current CPU
void hwsec_init_features(cpu_security_features_t* features_out);

// Apply a secure shadow stack to the specified thread (CET)
int hwsec_enable_shadow_stack(kthread_t* thread);

// Enforce Memory Protection Keys (MPK) onto an address space
int hwsec_apply_memory_keys(address_space_t* as, uint32_t pkey, uint32_t access_rights);

// Authenticate an instruction pointer (ARM PAC translation stub)
void* hwsec_authenticate_pointer(void* ptr, uint64_t modifier);

#endif // BHARAT_HW_SECURITY_H
