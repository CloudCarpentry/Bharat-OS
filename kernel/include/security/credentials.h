#ifndef BHARAT_CREDENTIALS_H
#define BHARAT_CREDENTIALS_H

#include <stdint.h>

#define BHARAT_MAX_SUPP_GROUPS 8U

typedef struct {
    uint32_t uid;
    uint32_t gid;
    uint32_t euid;
    uint32_t egid;
    uint32_t supp_groups[BHARAT_MAX_SUPP_GROUPS];
    uint32_t supp_group_count;
    uint64_t capability_mask;
} bharat_credentials_t;

int bharat_credentials_init(void);
int bharat_credentials_assign_process(uint32_t process_id,
                                     const bharat_credentials_t* cred);
int bharat_credentials_get_process(uint32_t process_id,
                                  bharat_credentials_t* out_cred);
int bharat_credentials_check_capability(uint32_t process_id,
                                        uint64_t required_capability);

#endif // BHARAT_CREDENTIALS_H
