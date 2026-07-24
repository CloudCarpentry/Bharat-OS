### ADAS / Autonomous Driving

Bharat-OS is a good fit for future ADAS and autonomous-driving control platforms because the architecture separates safety-critical control paths from policy-heavy user-space services.

Potential value:

- **Mixed-criticality partitioning:** perception, planning, control, telemetry, diagnostics, and UI can run in isolated domains with explicit capabilities.
- **Deterministic IPC/URPC:** low-latency message paths can connect camera/radar/lidar preprocessing, fusion, planning, and actuator-control services without making the kernel policy-heavy.
- **Restartable user-space drivers:** sensor or accelerator drivers can fail and restart without forcing whole-system failure.
- **AI/accelerator-aware scheduling roadmap:** AI inference workloads can be placed by a user-space governor while the kernel keeps bounded deterministic fallback behavior.
- **Fault-domain design path:** perception failure, sensor timeout, control-path deadline miss, and driver crash can be classified into fault domains and routed to safe-state policy.

Example mapping:

| ADAS function | Bharat-OS mapping |
| --- | --- |
| Camera/radar/lidar input | User-space sensor drivers + capability-gated DMA/MMIO |
| Perception pipeline | Accelerator/NPU/GPU service domain |
| Sensor fusion | RT or latency-sensitive service domain |
| Planning/control | RT profile with deadline-aware scheduling |
| Vehicle communication | CAN/Ethernet gateway service |
| Diagnostics | Telemetry + fault-domain service |
| Safe fallback | Safety manager + watchdog + safe-state transition |
