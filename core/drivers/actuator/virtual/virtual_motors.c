#include "virtual_motors.h"

void bh_virtual_motors_init(bh_virtual_motor_bank_t *bank)
{
    uint32_t index;
    if (bank == 0) {
        return;
    }
    for (index = 0u; index < BH_VIRTUAL_MOTOR_COUNT; index++) {
        bank->motors[index] = (bh_virtual_motor_t){0};
    }
}

bh_status_t bh_virtual_motor_write(bh_virtual_motor_bank_t *bank,
                                   uint32_t instance,
                                   const bh_actuator_command_t *command)
{
    bh_virtual_motor_t *motor;

    if (bank == 0 || command == 0 || instance >= BH_VIRTUAL_MOTOR_COUNT ||
        command->abi_version != BH_ACTUATOR_ABI_VERSION || command->reserved != 0u) {
        return BH_ERR_INVALID_ARGUMENT;
    }
    motor = &bank->motors[instance];
    if (command->request_id <= motor->last_request_id) {
        return BH_ERR_STALE_CAPABILITY;
    }
    motor->last_request_id = command->request_id;
    if ((command->flags & BH_ACTUATOR_COMMAND_DISARM) != 0u) {
        motor->armed = 0u;
        motor->output = 0.0f;
        return BH_OK;
    }
    if ((command->flags & BH_ACTUATOR_COMMAND_ARM) != 0u) {
        motor->armed = 1u;
    }
    if (motor->armed == 0u || (command->flags & BH_ACTUATOR_COMMAND_NORMALIZED) == 0u ||
        command->value < 0.0f || command->value > 1.0f) {
        motor->output = 0.0f;
        return BH_ERR_ACCESS_DENIED;
    }
    motor->output = command->value;
    return BH_OK;
}

bh_status_t bh_virtual_motor_safe(bh_virtual_motor_bank_t *bank, uint32_t instance)
{
    if (bank == 0 || instance >= BH_VIRTUAL_MOTOR_COUNT) {
        return BH_ERR_INVALID_ARGUMENT;
    }
    bank->motors[instance].armed = 0u;
    bank->motors[instance].output = 0.0f;
    return BH_OK;
}
