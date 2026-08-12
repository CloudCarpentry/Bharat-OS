# Bharat Virtual Robotics Demo

The virtual robotics path supplies five generic sensor types (IMU, GPS,
temperature, battery, and distance/LiDAR) and four normalized motor actuators.
It is deliberately a hardware capability pack rather than a separate Bharat-OS
kernel or a drone-specific API.

## Layering

```text
robotics demo stack
        |
sensormgr / actuator_mgr
        |
generic sensor / actuator UAPI
        |
virtual sensor / motor drivers
        |
QEMU robot platform instance
```

Run the focused hosted data-path test with:

```bash
cmake -S . -B build/robotics-demo -DBHARAT_BUILD_HOST_TESTS=ON
cmake --build build/robotics-demo --target host_test_robotics_demo
ctest --test-dir build/robotics-demo -R host_test_robotics_demo --output-on-failure
```

The test initializes the virtual platform, opens and subscribes to generic
sensors, arms four motors, renders the dashboard, rejects a stale stream and a
replayed actuator command, and verifies explicit safe-state neutralization.

## Scope

This first increment is a deterministic hosted/QEMU device model. It does not
claim physical BSP integration, flight control, sensor fusion, or production IPC
transport. Hardware GPIO, I2C, SPI, PWM, UART, CAN, ADC, capture timer, and
watchdog bindings can implement the same class boundaries incrementally.
