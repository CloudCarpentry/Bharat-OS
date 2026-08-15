#ifndef BHARAT_SDK_ACTUATOR_H
#define BHARAT_SDK_ACTUATOR_H

#include <bharat/types.h>
#include <bharat/uapi/device/actuator.h>

bh_status_t bh_actuator_open(bh_actuator_type_t type,
                             uint32_t instance,
                             bh_actuator_handle_t *actuator);
bh_status_t bh_actuator_write(bh_actuator_handle_t actuator,
                              const bh_actuator_command_t *command);

#endif
