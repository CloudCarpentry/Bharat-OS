#include "virtual_sensors.h"

#define BH_VIRTUAL_SAMPLE_PERIOD_NS 10000000ull

void bh_virtual_sensors_init(bh_virtual_sensor_bank_t *bank)
{
    if (bank != 0) {
        bank->timestamp_ns = 0u;
        bank->sequence = 0u;
    }
}

bh_status_t bh_virtual_sensor_read(bh_virtual_sensor_bank_t *bank,
                                   bh_sensor_type_t type,
                                   bh_sensor_sample_t *sample)
{
    uint32_t phase;

    if (bank == 0 || sample == 0) {
        return BH_ERR_INVALID_ARGUMENT;
    }
    if (type < BH_SENSOR_IMU || type > BH_SENSOR_DISTANCE) {
        return BH_ERR_NOT_FOUND;
    }

    bank->timestamp_ns += BH_VIRTUAL_SAMPLE_PERIOD_NS;
    bank->sequence++;
    phase = bank->sequence % 20u;
    *sample = (bh_sensor_sample_t){
        .sensor = (bh_sensor_id_t)type,
        .type = (uint32_t)type,
        .timestamp_ns = bank->timestamp_ns,
        .flags = BH_SENSOR_SAMPLE_VALID,
    };

    switch (type) {
    case BH_SENSOR_IMU:
        sample->value_count = 3u;
        sample->values[0] = 3.2f + ((float)phase * 0.01f);
        sample->values[1] = -1.8f;
        sample->values[2] = 128.4f;
        break;
    case BH_SENSOR_GPS:
        sample->value_count = 3u;
        sample->values[0] = 28.6139f;
        sample->values[1] = 77.2090f;
        sample->values[2] = 28.0f;
        break;
    case BH_SENSOR_TEMPERATURE:
        sample->value_count = 1u;
        sample->values[0] = 31.5f;
        break;
    case BH_SENSOR_BATTERY:
        sample->value_count = 2u;
        sample->values[0] = 78.0f - ((float)(bank->sequence / 100u));
        sample->values[1] = 15.2f;
        break;
    case BH_SENSOR_DISTANCE:
        sample->value_count = 1u;
        sample->values[0] = 12.0f + ((float)phase * 0.05f);
        break;
    default:
        return BH_ERR_NOT_FOUND;
    }
    return BH_OK;
}
