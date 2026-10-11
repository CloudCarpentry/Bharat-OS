# Service Health Integration

## Supervisor (Agent B) and PM/VM (Agent D) Instructions

When integrating the Bharat-OS Service Health Contract (`sm_req_health_update_t`), consumers must abide by these principles to ensure robustness:

### Capability & Caller Identity Checks
- Endpoints processing `SM_OP_HEALTH_UPDATE` must verify the caller has the necessary capability rights.
- The `service_id` within the payload must be validated against the caller's capability or PM registration to prevent spoofing.

### Stale Sequence Detection
- The `incarnation_id` and `heartbeat_sequence` fields must be monotonic per service.
- If an update arrives with an older or equal `heartbeat_sequence` for the same `incarnation_id`, the update **MUST** be rejected as stale.
- A higher `incarnation_id` always resets the `heartbeat_sequence`.

### Payload Bounding and Struct Size
- Consumers must validate the message length against `SM_HEALTH_UPDATE_MAX_PAYLOAD_SIZE` (which is equal to `sizeof(sm_req_health_update_t)`).
- The `struct_size` field provided inside the payload can be used for future ABI versioning. It must exactly match the `sizeof` expectation for the current iteration.
