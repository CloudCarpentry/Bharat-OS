import re
import sys
import os

TYPE_MAP = {
    "u32": "uint32_t",
    "u64": "uint64_t",
}

def parse_bidl(path):
    with open(path) as f:
        lines = f.readlines()

    service = {"name": "", "id": 0, "rpcs": [], "messages": {}}

    current_msg = None

    for line in lines:
        line = line.strip()

        if not line or line.startswith("//"):
            continue

        # service
        m = re.match(r"service\s+([\w\.]+)\s*=\s*(\d+)\s*\{", line)
        if m:
            service["name"] = m.group(1)
            service["id"] = int(m.group(2))
            continue

        # rpc
        m = re.match(r"rpc\s+(\w+)\((\w+)\)\s*->\s*(\w+)", line)
        if m:
            service["rpcs"].append({
                "name": m.group(1),
                "req": m.group(2),
                "resp": m.group(3)
            })
            continue

        # message start
        m = re.match(r"message\s+(\w+)\s*\{", line)
        if m:
            current_msg = m.group(1)
            service["messages"][current_msg] = []
            continue

        # message end
        if line == "}":
            current_msg = None
            continue

        # fields
        if current_msg:
            m = re.match(r"([\w<>]+)\s+(\w+);", line)
            if m:
                service["messages"][current_msg].append(
                    (m.group(1), m.group(2))
                )

    return service


def c_type(t):
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

    raise Exception(f"Unknown type: {t}")


def gen_types(service, outdir):
    fname = service["name"].replace(".", "_") + "_types.h"
    path = os.path.join(outdir, fname)

    # Check if we need bharat_cap_wire_t
    needs_cap_wire = False
    for msg, fields in service["messages"].items():
        for t, name in fields:
            if t == "cap_descriptor":
                needs_cap_wire = True
                break

    with open(path, "w") as f:
        f.write("#pragma once\n")
        f.write("#include <stdint.h>\n\n")

        if needs_cap_wire:
            f.write("#include \"bharat/msg/wire_types.h\"\n\n")

        for msg, fields in service["messages"].items():
            f.write(f"typedef struct {{\n")
            for t, name in fields:
                f.write(f"    {c_type(t)} {name};\n")
            f.write(f"}} {service['name'].replace('.', '_')}_{msg}_t;\n\n")


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

    gen_types(service, outdir)
    gen_dispatch(service, outdir)

    print("Generated for service:", service["name"])


if __name__ == "__main__":
    main()
