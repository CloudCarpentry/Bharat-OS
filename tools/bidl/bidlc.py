import sys
import os
from pathlib import Path

# Add repo root to sys.path so we can import from tools.*
REPO_ROOT = Path(__file__).resolve().parent.parent.parent
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from tools.bidl.parser import parse_bidl, BidlParseError, SkipDialectError

TYPE_MAP = {
    "u32": "uint32_t",
    "u64": "uint64_t",
    "bool": "bool",
}

def c_type(t, service):
    # 1. primitive types
    if t in TYPE_MAP:
        return TYPE_MAP[t]

    if t.startswith("string<"):
        size = int(t.split("<")[1].split(">")[0])
        return f"struct {{ uint32_t len; char data[{size}]; }}"

    if t.startswith("bytes<"):
        size = int(t.split("<")[1].split(">")[0])
        return f"struct {{ uint32_t len; uint8_t data[{size}]; }}"

    # 2. local enum
    if t in service["enums"]:
        return "uint32_t"

    # 3. local message/struct
    if t in service["messages"]:
        return f"struct {t}"

    # 4. explicitly registered external/builtin types
    if t == "cap_descriptor":
        return "bharat_cap_wire_t"

    # 5. error
    raise Exception(f"Unknown type: {t}")


def gen_types(service, outdir):
    service_clean_name = service["name"].replace(".", "_")
    fname = f"{service_clean_name}_types.h"
    path = os.path.join(outdir, fname)

    # Check if we need bharat_cap_wire_t
    needs_cap_wire = False
    needs_bool = False
    for msg in sorted(service["messages"].keys()):
        for field in service["messages"][msg]:
            t = field["type"]
            # It's only the external type if it's NOT locally defined
            if t == "cap_descriptor" and "cap_descriptor" not in service["messages"]:
                needs_cap_wire = True
            if t == "bool":
                needs_bool = True

    with open(path, "w") as f:
        f.write("#pragma once\n")
        f.write("#include <stdint.h>\n")
        if needs_bool:
            f.write("#include <stdbool.h>\n")
        f.write("\n")

        if needs_cap_wire:
            f.write("#include \"bharat/msg/wire_types.h\"\n\n")

        f.write("typedef enum {\n")
        f.write("    BH_BIDL_DISPATCH_OK = 0,\n")
        f.write("    BH_BIDL_DISPATCH_UNKNOWN_OPCODE = -1,\n")
        f.write("} bh_bidl_dispatch_status_t;\n\n")

        for enum_name in sorted(service["enums"].keys()):
            enum_vals = service["enums"][enum_name]
            f.write(f"typedef enum {{\n")
            for val in enum_vals:
                f.write(f"    {val['name']} = {val['value']},\n")
            f.write(f"}} {enum_name};\n\n")

        # forward declarations for structs
        for msg in sorted(service["messages"].keys()):
            f.write(f"struct {msg};\n")
        f.write("\n")

        for msg in sorted(service["messages"].keys()):
            fields = service["messages"][msg]
            f.write(f"struct {msg} {{\n")
            for field in fields:
                t = field["type"]
                name = field["name"]
                f.write(f"    {c_type(t, service)} {name};\n")
            f.write(f"}};\n")
            f.write(f"typedef struct {msg} {service_clean_name}_{msg}_t;\n\n")


def gen_dispatch(service, outdir):
    service_clean_name = service["name"].replace(".", "_")
    fname = f"{service_clean_name}_dispatch.c"
    types_fname = f"{service_clean_name}_types.h"
    path = os.path.join(outdir, fname)

    prefix = service_clean_name.upper()

    with open(path, "w") as f:
        f.write(f'#include "{types_fname}"\n')
        f.write("#include <stdint.h>\n\n")

        f.write("// Dispatch stub\n\n")

        for i, rpc in enumerate(service["rpcs"], start=1):
            f.write(f"#define BH_{prefix}_OP_{rpc['name'].upper()} {i}\n")

        f.write(f"\nint {service_clean_name}_dispatch(uint16_t opcode) {{\n")
        f.write("    switch(opcode) {\n")

        for rpc in service["rpcs"]:
            f.write(f"    case BH_{prefix}_OP_{rpc['name'].upper()}:\n")
            f.write(f"        // TODO: call {rpc['name']}\n")
            f.write("        return BH_BIDL_DISPATCH_OK;\n")

        f.write("    default:\n")
        f.write("        return BH_BIDL_DISPATCH_UNKNOWN_OPCODE;\n")
        f.write("    }\n")
        f.write("}\n")


def main():
    if len(sys.argv) < 3:
        print("Usage: bidlc.py <input.bidl> <outdir>")
        sys.exit(1)

    try:
        service = parse_bidl(sys.argv[1])
    except BidlParseError as e:
        print(f"[CodeGen Error] {e}", file=sys.stderr)
        sys.exit(1)
    except SkipDialectError as e:
        print(e.msg)
        sys.exit(0)  # We can exit successfully when explicitly skipping

    outdir = sys.argv[2]
    os.makedirs(outdir, exist_ok=True)

    if not service["name"]:
        print(f"[CodeGen Error] Missing or unnamed service definition in {sys.argv[1]}", file=sys.stderr)
        sys.exit(1)

    try:
        gen_types(service, outdir)
        gen_dispatch(service, outdir)
    except Exception as e:
        print(f"[CodeGen Error] {e}", file=sys.stderr)
        sys.exit(1)

    print("Generated for service:", service["name"])


if __name__ == "__main__":
    main()
