#ifndef BHARAT_UAPI_SYSTEM_FAULT_DOMAIN_H
#define BHARAT_UAPI_SYSTEM_FAULT_DOMAIN_H
#include <stdint.h>
typedef struct {
    uint32_t version;
    uint32_t flags;
    char name[32];
} bharat_fault_domain_attr_t;
#endif
