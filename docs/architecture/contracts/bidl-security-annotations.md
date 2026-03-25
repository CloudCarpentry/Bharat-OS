# BIDL Security Annotations

BIDL serves as the declarative authority contract for Bharat-OS. The generated dispatcher runtimes rely on security annotations to enforce capability and lease boundaries automatically. Manual dispatcher bypasses are forbidden.

## Supported Security Annotations

- `@requires(CAP_TYPE_*)`: Requires the caller to present a valid capability of the specified type.
- `@transport(TYPE)`: Specifies the transport mechanism (e.g., uRPC, local IPC).
- `@lease_required(DMA_GRANT)`: Requires an active, non-expired lease for the memory buffer.
- `@fault_domain(DOMAIN)`: The fault domain constraint for the service call.

## Examples

```
@requires(CAP_TYPE_ACCEL_DEVICE)
@lease_required(DMA_GRANT)
rpc SubmitJob(JobDesc req) -> (JobStatus res);
```
