#ifndef BHARAT_SDK_DEVICE_H
#define BHARAT_SDK_DEVICE_H
#include <bharat/types.h>
typedef struct bh_device_info { uint32_t struct_size; uint32_t device_class; uint64_t device_id; char name[32]; } bh_device_info_t;
bh_status_t bh_device_find(uint32_t device_class, uint32_t index, bh_device_info_t *info);
bh_status_t bh_device_open(uint64_t device_id, bh_cap_t authority, bh_handle_t *device);
bh_status_t bh_device_query(bh_handle_t device, bh_device_info_t *info);
bh_status_t bh_device_ioctl(bh_handle_t device, uint32_t operation, const void *input, size_t input_size, void *output, size_t output_size);
#endif
