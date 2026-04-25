# Bharat-OS IDL (BIDL) v1 Grammar (Current Implementation Profile)

## 1. Purpose and Scope

This document defines the **currently implemented BIDL v1 profile** used by the in-repo tooling (`tools/binterface/idl/bidlc.py` and `tools/abi/check_idl_compat.py`).

It is intentionally narrower than the broader architectural language drafts in `docs/architecture/contracts/`.

Use this document when you need to know what parses and code-generates **today**.

---

## 2. Implementation Status (as of April 22, 2026)

Bharat-OS currently has multiple IDL-style syntaxes in `interface/idl/` (e.g., `service/message`, `package+struct`, `namespace/interface/method`).

Only the `service ... rpc ... message/struct ...` family is handled by the active Python tooling used for codegen/compat checks.

- `tools/binterface/idl/bidlc.py` parses:
  - `service <qualified_name> = <service_id> { ... }`
  - `rpc <Method>(<Req>) -> <Resp>`
  - `message <Name> { ... }`
- `tools/abi/check_idl_compat.py` additionally accepts:
  - `service <qualified_name> { ... }` (implicit `id = 0`)
  - `struct <Name> { ... }` as an alias for `message`
  - `enum <Name> { ... }` with explicit integer assignments

### Practical guidance

If your `.bidl` must be consumed by **both** codegen and ABI compatibility checks, prefer this conservative subset:

1. `service <qualified_name> = <service_id> { ... }`
2. `rpc <Method>(<ReqType>) -> <RespType>`
3. `message <TypeName> { <field_type> <field_name>; }`
4. Avoid relying on parser support for attributes/annotations as semantic inputs (they are tolerated as text but not interpreted for generation).

---

## 3. Core Grammar (Implemented Subset)

### 3.1 Service

```idl
service <qualified_name> = <service_id> {
  // rpc definitions
}
```

- `qualified_name` allows letters, digits, underscore, and dot.
- `service_id` is an integer.

### 3.2 RPC Method

```idl
rpc <MethodName>(<RequestType>) -> <ResponseType> {
  // optional metadata lines (not consumed by codegen today)
}
```

- RPC order is ABI-relevant for the compatibility checker (append-only evolution).
- Dispatch codegen currently assigns opcodes incrementally in declaration order, starting at `1`.

### 3.3 Message / Struct

```idl
message <MessageName> {
  <type> <field_name>;
}
```

Compatibility tooling also accepts:

```idl
struct <MessageName> {
  <type> <field_name>;
}
```

Field order is ABI-relevant and must be append-only for non-breaking evolution.

### 3.4 Enum (compatibility checker)

```idl
enum <EnumName> {
  <NAME> = <int>;
}
```

Enum values must remain stable once published.

---

## 4. Data Types in the Current Code Generator

`bidlc.py` currently generates C for:

- `u32` → `uint32_t`
- `u64` → `uint64_t`
- `string<N>` → inline bounded `{ uint32_t len; char data[N]; }`
- `bytes<N>` → inline bounded `{ uint32_t len; uint8_t data[N]; }`
- `cap_descriptor` → `bharat_cap_wire_t`

### Important limitation

The parser accepts field text for many additional spellings found in repository IDLs (for example `uint32`, `int32`, `bool`), but **C code generation in `bidlc.py` only supports the type list above**. Unsupported types will fail with `Unknown type` during generation.

---

## 5. Metadata Blocks and Method Attributes

IDL files in the repo often include per-RPC metadata such as:

```idl
qos = reliable;
timeout_ms = 100;
idempotent = true;
auth = required;
criticality = rt_ctl;
```

This metadata is currently useful as declarative documentation and policy intent, and is parsed permissively as part of the surrounding syntax. However, current `bidlc.py` codegen does **not** convert these keys into generated enforcement logic.

---

## 6. ABI Compatibility Rules (Current CI Contract)

The compatibility checker enforces these stability rules for baseline vs current IDL:

- Service removal is forbidden.
- Service ID changes are forbidden.
- RPC list is append-only (no delete/reorder/rename/signature change in existing prefix).
- Message/struct fields are append-only (no delete/reorder/rename/type-change in existing prefix).
- Enums cannot remove members or change existing numeric values.

When evolving contracts, treat existing declaration order as stable ABI.

---

## 7. Example (Safe Profile)

```idl
service bharat.monitor.v1 = 1 {
  rpc Heartbeat(HeartbeatReq) -> HeartbeatResp {
    qos = reliable;
    timeout_ms = 100;
    idempotent = true;
    criticality = rt_ctl;
  }

  rpc NodeJoin(NodeJoinReq) -> NodeJoinResp {
    qos = reliable;
    timeout_ms = 500;
    auth = required;
    criticality = rt_ctl;
  }
}

message HeartbeatReq {
  u32 node_id;
  u64 monotonic_time_ns;
  u32 health_flags;
}

message HeartbeatResp {
  u32 accepted;
}

message NodeJoinReq {
  u32 protocol_version;
  u32 arch;
  u64 boot_nonce;
  string<64> node_name;
  bytes<128> topology_digest;
}

message NodeJoinResp {
  u32 accepted;
  u32 assigned_node_id;
  u64 session_nonce;
}
```

---

## 8. Relationship to Broader BIDL Docs

For future-direction language design (annotations, richer type system, transport semantics), see:

- `docs/architecture/contracts/bidl-language-spec.md`
- `docs/architecture/contracts/bidl-runtime-mapping.md`
- `docs/architecture/contracts/bidl-versioning-policy.md`

Those documents describe target-state architecture. This `bidl-v1.md` describes the **currently implemented subset and constraints**.
