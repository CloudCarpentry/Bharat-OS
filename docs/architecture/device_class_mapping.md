# Device Class Registry Mapping

The `bharat_device_class_t` and `bharat_device_descriptor_t` defined in `include/bharat/interface/uapi/device/device_class.h` provide a normalized classification and descriptor contract across the system.

This document explains how existing components map to this contract to achieve a strict separation of hardware mechanism (drivers) from orchestration/policy (services).

## Mapping Example: Network Device (`netdev_t`)

Existing `netdev_t` drivers reside in `core/drivers/net/` and register via `netdev_register`. When the device manager enumerates or exports these drivers to the network manager (`netmgr`), it populates a standard device descriptor.

```c
// Inside devmgr or a HAL enumerator mapping a netdev_t
bharat_device_descriptor_t desc = {
    .device_id = internal_id,
    .dev_class = BHARAT_DEV_CLASS_NET,
    .vendor_id = pci_vendor,
    .product_id = pci_device,
    .attributes = BHARAT_DEV_ATTR_POWER_MGT
};
strncpy(desc.name, netdev->name, sizeof(desc.name) - 1);
```

The network manager service (`core/services/netmgr`) can then issue a `bharat_device_query_req_t` with `dev_class = BHARAT_DEV_CLASS_NET` to discover the underlying physical device, retrieve its hardware identity, and establish IPC bindings (or map ring buffers) without linking any actual driver code.

## Key Rules
1. **Header-First Contract:** The device classes are purely descriptive data structures. There are no virtual driver function pointers (ops tables) in the UAPI header.
2. **Driver Placement:** Drivers remain in `core/drivers/` and populate these descriptors to export themselves.
3. **Service Logic:** Services (in `core/services/`) consume these descriptors via IPC queries to manage policy. They do not run driver logic.
