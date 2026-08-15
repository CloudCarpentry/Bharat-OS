#include "actuator_mgr.h"

void bh_actuator_mgr_init(bh_actuator_mgr_t *manager, bh_virtual_motor_bank_t *devices)
{
    uint32_t index;
    if (manager == 0) {
        return;
    }
    manager->devices = devices;
    for (index = 0u; index < BH_VIRTUAL_MOTOR_COUNT; index++) {
        manager->grants[index] = 1u;
    }
}

bh_status_t bh_actuator_mgr_open(bh_actuator_mgr_t *manager,
                             bh_actuator_type_t type,
                             uint32_t instance,
                             bh_actuator_handle_t *actuator)
{
    if (manager == 0 || manager->devices == 0 || actuator == 0) {
        return BH_ERR_INVALID_ARGUMENT;
    }
    if (type != BH_ACTUATOR_MOTOR || instance >= BH_VIRTUAL_MOTOR_COUNT) {
        return BH_ERR_NOT_FOUND;
    }
    if (manager->grants[instance] == 0u) {
        return BH_ERR_INSUFFICIENT_RIGHTS;
    }
    *actuator = instance + 1u;
    return BH_OK;
}

bh_status_t bh_actuator_mgr_write(bh_actuator_mgr_t *manager,
                              bh_actuator_handle_t actuator,
                              const bh_actuator_command_t *command)
{
    uint32_t instance;
    if (manager == 0 || manager->devices == 0 || actuator == 0u) {
        return BH_ERR_INVALID_ARGUMENT;
    }
    instance = actuator - 1u;
    if (instance >= BH_VIRTUAL_MOTOR_COUNT || manager->grants[instance] == 0u) {
        return BH_ERR_INSUFFICIENT_RIGHTS;
    }
    return bh_virtual_motor_write(manager->devices, instance, command);
}

void bh_actuator_mgr_enter_safe_state(bh_actuator_mgr_t *manager)
{
    uint32_t index;
    if (manager == 0 || manager->devices == 0) {
        return;
    }
    for (index = 0u; index < BH_VIRTUAL_MOTOR_COUNT; index++) {
        (void)bh_virtual_motor_safe(manager->devices, index);
    }
}
