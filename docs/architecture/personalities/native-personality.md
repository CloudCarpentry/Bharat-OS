---
title: Native Personality
status: active
owner: Architecture Team
version: 1.0
---

# Native Personality (Bharat-OS Reference)

The Native Personality is the reference platform for Bharat-OS. It is not an emulation or compatibility layer; it is the raw, unrestricted (save for capability constraints) application platform. It defines the true OS contract.

## 1. App Model (Process & Lifecycle)

*   **No `fork()`:** Applications are created using explicit `spawn` primitives. Address spaces are populated based on the app manifest before thread execution begins.
*   **Lifecycle:** Applications receive explicit capability-mediated messages for `START`, `PAUSE`, `RESUME`, and `STOP`.
*   **Background Jobs:** Native apps do not fork to daemonize. They register background service components in their manifest, which the OS scheduler and service manager invoke based on system policy.

## 2. Manifest Schema

Every Native application is deployed with a strict manifest.
```json
{
  "app_id": "com.bharatos.native.demo",
  "version": "1.0.0",
  "entry": "bin/demo_app",
  "capabilities": {
    "required": [
      "sys.display.window.create",
      "net.socket.bind:8080"
    ],
    "optional": [
      "hw.camera.access"
    ]
  },
  "services": [
    {
      "name": "demo_worker",
      "ipc_contract": "bidl/demo_v1"
    }
  ]
}
```

## 3. IPC and Service Access Model

*   **URPC Native:** All cross-process communication uses the Bharat-OS URPC (Unified RPC) layer over capability-secured endpoints.
*   **Service Discovery:** Apps query a local namespace manager for service endpoints. Endpoints are strictly isolated.
*   **No Global PIDs:** Apps do not send signals to process IDs. They send messages to capability handles representing a target.

## 4. Capability Model Usage

The Native personality is the purest expression of the Bharat-OS capability model.
*   Handles are 64-bit opaque references managed by the kernel.
*   Every UAPI call (map memory, send message, create thread) requires an explicit handle.
*   Capabilities are granted at spawn via the manifest and can be delegated dynamically via IPC.

## 5. Filesystem and Config Conventions

*   **VFS is a Service:** The filesystem is not in the kernel. It is a service accessed via URPC.
*   **Namespaces over Paths:** Global `/` does not exist for apps. An app is granted a capability to a virtual root directory (e.g., its private data directory and a read-only view of its package).
*   **Config:** Configuration is exposed via a structured capability-based key-value service, not flat files in `/etc`.

## 6. Packaging Model

*   Packages are read-only, verifiable bundles (e.g., similar to Flatpak or APEX) signed by a trusted authority.
*   Installed via the `bharat-pkg` tool.
*   Updates are atomic.

## 7. SDK Contract

*   **Languages:** C, C++, and Rust are first-class native languages.
*   **Headers:** Found in `sdk/native/include/`.
*   **Libraries:** Link against `libbharat_native`, which provides safe wrappers around raw `uapi/` syscalls.
