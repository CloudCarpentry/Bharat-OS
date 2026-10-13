# Performance and architecture

- **io_uring-style shared-memory async rings:** Your uRPC/MPSC rings are the right idea. Benchmark against seL4 and Zircon IPC.
- **eBPF-like safe extensibility:** A verified bytecode sandbox for telemetry and policy hooks, without loading kernel modules.
- **Wasm component model runtime:** (WASI 0.2/0.3, wasmtime or WAMR) as a first-class personality. It is a better compatibility story than Android and lighter than Linux.
- **Unikernel and microVM modes:** Booting under KVM, Firecracker or Cloud Hypervisor with virtio would give you cloud CI, a cheap demo path and a deployment option for the Bharat Network profile.
- **Mixed-criticality partitioning:** Hypervisor-based or seL4-style static partitioning (Jailhouse and Xen-style, Zephyr coexistence) for the industrial and automotive profiles.
- **Linux compatibility:** via a library OS or a user-space Linux personality. Fuchsia (Starnix), Asterinas (Linux ABI) and gVisor show how to do this without putting Linux in the TCB.
