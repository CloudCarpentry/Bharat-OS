# Bharat-OS IDL (BIDL) v1 Grammar

## 1. Overview
The **Bharat-OS Interface Definition Language (BIDL)** is a minimal, language-agnostic schema format used to generate binary serializers, parsers, and RPC dispatch stubs for the Bharat-OS Message Wire Format.

BIDL v1 prioritizes:
- Simplicity (Python stdlib-only parser)
- Determinism (No hidden types or unbounded sizes)
- C-language codegen (Inline structs for bounded buffers)

---

## 2. Basic Syntax

### Services
A `service` block groups related RPC methods and assigns a global `service_id`.

```idl
service <qualified_name> = <service_id> {
  // Methods
}
```

### RPC Methods
An `rpc` definition defines a request message and its corresponding response message. Attributes can be specified within the block to dictate transport behavior (QoS, timeouts).

```idl
  rpc <MethodName>(<RequestType>) -> <ResponseType> {
    qos = reliable | best_effort;
    timeout_ms = <int>;
    idempotent = true | false;
    auth = required | optional;
    criticality = rt_ctl | rt_data | best_effort | bulk | debug;
  }
```

### Messages
A `message` is a struct-like collection of typed fields. It maps directly to an inline C-struct payload format.

```idl
message <MessageName> {
  <type> <field_name>;
}
```

---

## 3. Supported Data Types

BIDL v1 strictly defines sizes and bounds on the wire.

### Primitives
- `u8`, `u16`, `u32`, `u64` (unsigned integers)
- `i8`, `i16`, `i32`, `i64` (signed integers)
- `bool` (boolean, encoded as `u8`)

### Bounded Complex Types
These generate inline bounded structs in C. No heap allocation is required during decode.

- `string<N>`: A UTF-8 string with a maximum length of `N` bytes (excluding length prefix).
- `bytes<N>`: An opaque binary blob with a maximum length of `N` bytes.
- `array<T, N>`: A fixed-size array of exactly `N` elements of type `T`.
- `vec<T, N>`: A variable-sized list (vector) of up to `N` elements of type `T`.

### Capability and OOL Types
- `cap_descriptor`: Represents a delegated capability exported onto the wire.
- `ool_descriptor`: Represents a pointer to an out-of-line memory payload (e.g., shared memory).

---

## 4. Example Schema

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

## 5. Code Generation Target

The generated C structures will flatten dynamically sized items (like `string<64>`) into inline arrays to maintain bounded memory lifetimes and avoid `malloc`.

```c
typedef struct {
    uint32_t protocol_version;
    uint32_t arch;
    uint64_t boot_nonce;
    struct {
        uint32_t len;
        char data[64];
    } node_name;
    struct {
        uint32_t len;
        uint8_t data[128];
    } topology_digest;
} bharat_monitor_v1_node_join_req_t;
```

The codec APIs will iterate field-by-field, copying from the wire buffer into these structured targets while asserting `len <= MAX`.
