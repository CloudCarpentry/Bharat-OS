#ifndef BHARAT_CAP_POLICY_H
#define BHARAT_CAP_POLICY_H

#include "capability.h"

// Returns 1 if the capability type is transferable via IPC, 0 otherwise.
int cap_can_transfer(cap_type_t type);

// Returns 1 if the rights requested to be transferred are valid for the given capability type.
int cap_transfer_rights_valid(cap_type_t type, uint32_t transfer_rights);

#endif // BHARAT_CAP_POLICY_H
