#ifndef BHARAT_CAP_POLICY_H
#define BHARAT_CAP_POLICY_H

#include "capability.h"

// Returns 1 if the capability type and requested rights are legal and a subset of source rights, 0 otherwise.
int cap_can_transfer(cap_type_t type, cap_rights_mask_t src_rights, cap_rights_mask_t requested);

// Returns 1 if the rights requested to be transferred are structurally legal for the given capability type.
int cap_transfer_rights_valid(cap_type_t type, cap_rights_mask_t requested);

#endif // BHARAT_CAP_POLICY_H
