# Hidden Global State Audit

This report highlights architectural weaknesses related to the usage of file-local `static` arrays to manage state, which limits multi-tenancy and scalability.

## 1. VM Manager Static State
- **`core/services/vm_manager/vm_manager.c`**: Relies on massive static arrays such as `g_vm_spaces`, `g_vm_regions`, and `region_table`.
- **Risk**: Static memory forces a hard limit on the number of concurrent processes and regions. If the system supports dynamic workloads or virtual machines, this memory cannot be dynamically scaled at boot.
- **Action**: Transition to dynamic slab allocation using the standard memory APIs (e.g. `kmalloc` equivalent in userspace) or boot-time pool sizing.

## 2. Device Manager State
- **`core/services/device/devmgr/device_manager.c`**: Similar static allocations for device drivers, windows, and handles.
- **Risk**: Prevents hot-plugging devices beyond an arbitrary limit.
- **Action**: Implement dynamic linked lists for device registrations.

## 3. Power Mode State
- **`core/services/power_mode/state_machine.c`**: Stores callbacks in `g_clients`. While bounded correctly, relying on static tracking means the service cannot be easily restarted or isolated across multiple sub-domains (like hypervisor guests).
