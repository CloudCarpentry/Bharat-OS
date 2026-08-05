import sys
import os
from parser import parse_bidl

TYPE_MAP = {
    "u32": "uint32_t",
    "u64": "uint64_t",
    "bool": "bool",
}

def c_type(t, service):
    if t in TYPE_MAP:
        return TYPE_MAP[t]

    if t.startswith("string<"):
        size = int(t.split("<")[1].split(">")[0])
        return f"struct {{ uint32_t len; char data[{size}]; }}"

    if t.startswith("bytes<"):
        size = int(t.split("<")[1].split(">")[0])
        return f"struct {{ uint32_t len; uint8_t data[{size}]; }}"

    if t == "cap_descriptor":
        return "bharat_cap_wire_t"

    if t in service["enums"]:
        return "uint32_t"

    if t in service["messages"]:
        return f"struct {t}"

    raise Exception(f"Unknown type: {t}")


def gen_types(service, outdir):
    fname = service["name"].replace(".", "_") + "_types.h"
    path = os.path.join(outdir, fname)

    # Check if we need bharat_cap_wire_t
    needs_cap_wire = False
    needs_bool = False
    for msg, fields in service["messages"].items():
        for field in fields:
            t = field["type"]
            if t == "cap_descriptor":
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

        for enum_name, enum_vals in service["enums"].items():
            f.write(f"typedef enum {{\n")
            for val in enum_vals:
                f.write(f"    {val['name']} = {val['value']},\n")
            f.write(f"}} {enum_name};\n\n")

        # forward declarations for structs
        for msg in service["messages"]:
            f.write(f"struct {msg};\n")
        f.write("\n")

        for msg, fields in service["messages"].items():
            f.write(f"struct {msg} {{\n")
            for field in fields:
                t = field["type"]
                name = field["name"]
                f.write(f"    {c_type(t, service)} {name};\n")
            f.write(f"}};\n")
            f.write(f"typedef struct {msg} {service['name'].replace('.', '_')}_{msg}_t;\n\n")


def gen_dispatch(service, outdir):
    fname = service["name"].replace(".", "_") + "_dispatch.c"
    path = os.path.join(outdir, fname)

    with open(path, "w") as f:
        f.write("// Dispatch stub\n\n")

        for i, rpc in enumerate(service["rpcs"], start=1):
            f.write(f"#define OP_{rpc['name'].upper()} {i}\n")

        f.write("\nint dispatch(uint16_t opcode) {\n")
        f.write("    switch(opcode) {\n")

        for rpc in service["rpcs"]:
            f.write(f"    case OP_{rpc['name'].upper()}:\n")
            f.write(f"        // TODO: call {rpc['name']}\n")
            f.write("        break;\n")

        f.write("    }\n")
        f.write("    return 0;\n}\n")


def main():
    if len(sys.argv) < 3:
        print("Usage: bidlc.py <input.bidl> <outdir>")
        return

    service = parse_bidl(sys.argv[1])
    outdir = sys.argv[2]

    os.makedirs(outdir, exist_ok=True)

    if not service["name"]:
        print("[CodeGen] Warning: Skipping unnamed service definition in", sys.argv[1])
        return

    gen_types(service, outdir)
    gen_dispatch(service, outdir)

    print("Generated for service:", service["name"])


if __name__ == "__main__":
    main()
