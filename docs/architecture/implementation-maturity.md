# Runtime implementation maturity contract

`interface/contracts/implementation_maturity.json` is the build authority for whether linked
runtime areas are `REAL`, `PARTIAL`, `TEST_ONLY`, `STUB`, or `UNSUPPORTED`. The classification is
about implementation truth, not planned architecture. Source paths and short evidence make each
registration reviewable. The JSON schema defines the registration format.

Before configuring or linking, `tools/build.py` runs `tools/check_implementation_maturity.py`.
Targets declare their maturity policy profile explicitly; legacy targets fall back to the preset name.
`RELEASE` and `HARDENED` profiles reject every `STUB` and `TEST_ONLY` declaration. Other profiles
also reject those declarations unless their target YAML explicitly lists the maturity under
`implementation_maturity.allow`. Production ignores allowances. `PARTIAL` and `UNSUPPORTED` are
reported truthfully but are not prohibited by this initial policy.

The gate is fail-closed: malformed manifests, unknown maturity values, duplicate IDs, and missing
declared source paths are errors. It deliberately does not scan arbitrary `return 0` statements;
the manifest is authoritative and source auditing is supporting evidence only.

## Initial synthetic-success audit

The following synthetic-success implementations were found during PROD-P0-FAST-001:

| Area | Discovered behavior | Resolution |
|---|---|---|
| Process-manager default authority | Fabricated process ID `4200`, VM-space ID `9900`, main-thread ID `1100`, and returned success from create/start/terminate/reap operations without a kernel operation. | Classified `STUB`; all default authority operations now return `BHARAT_IPC_STATUS_ERR_UNSUPPORTED` without publishing fabricated output. |
| VM-manager default authority | Fabricated space ID `9911`, fabricated a mapped 4 KiB query result, and returned success from create/destroy/map/unmap/protect without a VM operation. | Classified `STUB`; all default authority operations now return `BHARAT_IPC_STATUS_ERR_UNSUPPORTED` without publishing fabricated output. |
| Filesystem layout helpers | Desktop, tiny-IoT, Android, and Linux layout initialization currently returns success without creating the documented mounts. | Classified as part of the `PARTIAL` filesystem service; unchanged because callers may currently use these helpers as optional layout templates. |
| Filesystem service request path | The request handler constructs a synthetic read and `main` exits instead of serving requests; the default device ID falls back to synthetic ID `42`. | Classified `PARTIAL`; unchanged because implementing the service/backend is out of scope. |
| devmgr service | Initialization and event loop do no device-management work, then `main` returns success. | Classified `STUB`; unchanged because changing process exit semantics requires service-runtime integration. |

Compatibility/personality placeholders are registered in the manifest. No compatibility service was
implemented by this change.

The `x86_64_desktop_gui` and `x86_64_desktop_headless` targets are explicitly
`DEVELOPMENT` targets and allow registered `STUB` implementations so that x86
integration can be exercised in QEMU consistently with the Arm64 and RISC-V64
development targets. This allowance does not change implementation truth and
cannot satisfy a `RELEASE` or `HARDENED` gate; those profiles remain fail-closed
until the registered authority and service implementations are replaced and
reclassified with evidence.
