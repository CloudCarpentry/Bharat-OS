#include "sensormgr.h"

#define BH_STREAM_INDEX_MASK 0xffffu
#define BH_STREAM_GENERATION_SHIFT 16u

static bh_sensor_stream_t make_stream(uint32_t index, uint16_t generation)
{
    return ((uint32_t)generation << BH_STREAM_GENERATION_SHIFT) | (index + 1u);
}

static bh_sensormgr_stream_slot_t *lookup_stream(bh_sensormgr_t *manager,
                                                  bh_sensor_stream_t stream)
{
    uint32_t encoded_index = stream & BH_STREAM_INDEX_MASK;
    uint16_t generation = (uint16_t)(stream >> BH_STREAM_GENERATION_SHIFT);
    bh_sensormgr_stream_slot_t *slot;

    if (manager == 0 || encoded_index == 0u || encoded_index > BH_SENSORMGR_MAX_STREAMS) {
        return 0;
    }
    slot = &manager->streams[encoded_index - 1u];
    if (slot->active == 0u || slot->generation != generation) {
        return 0;
    }
    return slot;
}

void bh_sensormgr_init(bh_sensormgr_t *manager, bh_virtual_sensor_bank_t *devices)
{
    uint32_t index;
    if (manager == 0) {
        return;
    }
    manager->devices = devices;
    for (index = 0u; index < BH_SENSORMGR_MAX_STREAMS; index++) {
        manager->streams[index] = (bh_sensormgr_stream_slot_t){.generation = 1u};
    }
}

bh_status_t bh_sensormgr_open(bh_sensormgr_t *manager,
                           bh_sensor_type_t type,
                           bh_sensor_handle_t *sensor)
{
    if (manager == 0 || manager->devices == 0 || sensor == 0) {
        return BH_ERR_INVALID_ARGUMENT;
    }
    if (type < BH_SENSOR_IMU || type > BH_SENSOR_DISTANCE) {
        return BH_ERR_NOT_FOUND;
    }
    *sensor = (bh_sensor_handle_t)type;
    return BH_OK;
}

bh_status_t bh_sensormgr_subscribe(bh_sensormgr_t *manager,
                                bh_sensor_handle_t sensor,
                                uint32_t rate_hz,
                                bh_sensor_stream_t *stream)
{
    uint32_t index;
    if (manager == 0 || stream == 0 || sensor < BH_SENSOR_IMU || sensor > BH_SENSOR_DISTANCE ||
        rate_hz < BH_SENSORMGR_MIN_RATE_HZ || rate_hz > BH_SENSORMGR_MAX_RATE_HZ) {
        return BH_ERR_INVALID_ARGUMENT;
    }
    for (index = 0u; index < BH_SENSORMGR_MAX_STREAMS; index++) {
        if (manager->streams[index].active == 0u) {
            manager->streams[index].active = 1u;
            manager->streams[index].type = (bh_sensor_type_t)sensor;
            manager->streams[index].rate_hz = rate_hz;
            *stream = make_stream(index, manager->streams[index].generation);
            return BH_OK;
        }
    }
    return BH_ERR_BUFFER_FULL;
}

bh_status_t bh_sensormgr_read(bh_sensormgr_t *manager,
                           bh_sensor_stream_t stream,
                           bh_sensor_sample_t *sample)
{
    bh_sensormgr_stream_slot_t *slot = lookup_stream(manager, stream);
    if (sample == 0) {
        return BH_ERR_INVALID_ARGUMENT;
    }
    if (slot == 0) {
        return BH_ERR_STALE_CAPABILITY;
    }
    return bh_virtual_sensor_read(manager->devices, slot->type, sample);
}

bh_status_t bh_sensormgr_unsubscribe(bh_sensormgr_t *manager, bh_sensor_stream_t stream)
{
    bh_sensormgr_stream_slot_t *slot = lookup_stream(manager, stream);
    if (slot == 0) {
        return BH_ERR_STALE_CAPABILITY;
    }
    slot->active = 0u;
    slot->generation++;
    if (slot->generation == 0u) {
        slot->generation = 1u;
    }
    return BH_OK;
}
