---
title: "BIDL Versioning Policy"
status: "Draft"
version: "0.1"
last_updated: "2024-03-24"
tags: ["architecture", "ipc", "bidl", "contracts", "versioning"]
---

# BIDL Versioning Policy

This document outlines the contract compatibility rules for the Bharat Interface Definition Language (BIDL) to prevent ABI breakage and ensure backward compatibility during system updates.

## 1. Interface Version vs. Implementation Version

Bharat-OS distinguishes between a component's semantic version (its implementation) and its interface version (its IPC contract). BIDL specifically models and enforces the **interface version**.

Every `.bidl` file must declare a version at the top:

```bidl
// namesvc.bidl
version 1.2;
```

This version dictates the layout of the generated structs, opcodes, and capability requirements. The implementation version of the service may change independently of the interface version.

## 2. Backward Compatibility Rules

To ensure a service update does not break existing clients (e.g., an older application calling a newer `netmgr`), BIDL enforces strict rules on what changes are considered additive versus breaking.

### 2.1 Additive Changes (Allowed without Major Bump)

An additive change does not alter the memory layout or semantics of existing fields and opcodes. It requires a **minor** version bump (e.g., `1.2` to `1.3`).

* **Adding a new method or event:** The BIDL compiler will assign a new opcode at the end of the enumeration.
* **Adding a new enum value:** The new value is appended to the enum.
* **Adding new optional fields to a request/response (using reserved space):** If a struct was designed with explicit reserved padding, those fields can be converted into typed fields.

### 2.2 Breaking Changes (Requires Major Bump)

A breaking change alters the ABI or semantics in a way that older clients cannot understand. It requires a **major** version bump (e.g., `1.2` to `2.0`).

* **Removing or renaming a method/event:** Opcode mappings change.
* **Changing the type or bound of a parameter:** For example, increasing `string<64>` to `string<128>` breaks fixed-size buffer assumptions in older clients.
* **Reordering fields in a struct:** Changes the C struct memory layout.
* **Adding new required capabilities to an existing method:** A client that previously succeeded will now fail with `K_ERR_DENIED`.
* **Adding fields without reserved space:** Changes the overall size of the struct, potentially overflowing older endpoint buffers.

## 3. Reserved Fields Strategy

To support additive changes without breaking fixed-size struct assumptions or requiring major version bumps, BIDL supports explicit reserved padding.

```bidl
struct EndpointInfo {
    string<64> name;
    u32 protocol_version;
    handle<endpoint> connection_port;

    // Future expansion space
    u8 reserved[16];
}
```

When an additive change is needed, the developer can replace a portion of the `reserved` array with a new, explicitly sized field.

```bidl
struct EndpointInfo {
    string<64> name;
    u32 protocol_version;
    handle<endpoint> connection_port;

    // Additive change in v1.3
    u32 flags;

    // Remaining expansion space
    u8 reserved[12];
}
```

## 4. Deprecation Guidance

When an interface or method is no longer recommended for use, it should be marked with the `@deprecated` annotation. The BIDL compiler will emit standard C `__attribute__((deprecated))` warnings on the generated stubs.

```bidl
@deprecated(version="2.0", reason="Use v2_resolve instead")
method resolve(string<64> name) -> (handle<endpoint> port, Status status);
```

The method must remain in the `.bidl` file until a major version bump completely removes it.

## 5. Discovery and Version Negotiation

When a service registers with `namesvc` or `servicemgr`, the generated manifest automatically includes the interface version.

A client can request a specific version or a version range during resolution. `namesvc` ensures that the returned endpoint capability belongs to a service that implements a compatible version (i.e., same major version, equal or greater minor version).

```c
// Client requests a compatible interface version
sys_namesvc_resolve("bharat.namesvc", 1, 2, &endpoint); // Requires v1.2 or higher
```
