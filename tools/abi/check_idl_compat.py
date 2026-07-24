import os
import re
from pathlib import Path

import common
from tools.build.path_aliases import resolve_idl_alias

IDL_DIR_CANDIDATES = (
    "interface/idl",
    "idl",
)


def resolve_idl_dir():
    for path in IDL_DIR_CANDIDATES:
        resolved_path, used_alias = resolve_idl_alias(Path(path))
        if resolved_path.exists():
            if used_alias:
                print(f"[migration-warning] Using aliased IDL path: {path} -> {resolved_path}")
            return str(resolved_path)
    return IDL_DIR_CANDIDATES[0]

def parse_bidl(path):
    with open(path, 'r') as f:
        lines = f.readlines()

    service = {"name": "", "id": 0, "rpcs": [], "messages": {}, "enums": {}}

    current_msg = None
    current_enum = None

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

        m = re.match(r"service\s+([\w\.]+)\s*\{", line)
        if m:
            service["name"] = m.group(1)
            # Default ID if none specified
            service["id"] = 0
            continue

        # rpc
        m = re.match(r"rpc\s+(\w+)\s*\(\s*([\w\.]+)\s*\)\s*->\s*([\w\.]+)", line)
        if m:
            service["rpcs"].append({
                "name": m.group(1),
                "req": m.group(2),
                "resp": m.group(3)
            })
            continue

        # message start
        m = re.match(r"struct\s+(\w+)\s*\{", line)
        if not m:
            m = re.match(r"message\s+(\w+)\s*\{", line)

        if m:
            current_msg = m.group(1)
            service["messages"][current_msg] = []
            continue

        # enum start
        m = re.match(r"enum\s+(\w+)\s*\{", line)
        if m:
            current_enum = m.group(1)
            service["enums"][current_enum] = []
            continue

        # block end
        if line == "}":
            current_msg = None
            current_enum = None
            continue

        # message fields
        if current_msg:
            # Try to match 'type name;'
            m = re.match(r"([\w<>\.]+)\s+(\w+);", line)
            if m:
                service["messages"][current_msg].append(
                    {"type": m.group(1), "name": m.group(2)}
                )

        # enum fields
        if current_enum:
            # Try to match 'NAME = VALUE;'
            m = re.match(r"(\w+)\s*=\s*(\d+);", line)
            if m:
                service["enums"][current_enum].append(
                    {"name": m.group(1), "value": int(m.group(2))}
                )

    return service

def generate_idl_manifest():
    manifest = {}
    idl_dir = resolve_idl_dir()

    for root, dirs, files in os.walk(idl_dir):
        for file in files:
            if not file.endswith('.bidl'):
                continue

            filepath = os.path.join(root, file)
            service = parse_bidl(filepath)
            if service["name"]:
                manifest[service["name"]] = service

    return manifest

def check_idl_compat(baseline, current):
    if baseline is None:
        return False

    success = True

    for srv_name, base_srv in baseline.items():
        if srv_name not in current:
            common.report_error(f"Service {srv_name} was removed. IDL deletions are forbidden.")
            success = False
            continue

        curr_srv = current[srv_name]

        # Check service ID
        if base_srv["id"] != curr_srv["id"]:
            common.report_error(f"Service {srv_name} ID changed from {base_srv['id']} to {curr_srv['id']}.")
            success = False

        # Check RPCs
        base_rpcs = {rpc["name"]: rpc for rpc in base_srv["rpcs"]}
        curr_rpcs = {rpc["name"]: rpc for rpc in curr_srv["rpcs"]}

        # Order matters for ABI stability (usually). We will check append-only.
        if len(curr_srv["rpcs"]) < len(base_srv["rpcs"]):
            common.report_error(f"Service {srv_name} has fewer RPCs. RPCs can only be appended.")
            success = False

        for i, b_rpc in enumerate(base_srv["rpcs"]):
            if i >= len(curr_srv["rpcs"]):
                break
            c_rpc = curr_srv["rpcs"][i]
            if b_rpc["name"] != c_rpc["name"]:
                common.report_error(f"Service {srv_name} RPC {i} changed from {b_rpc['name']} to {c_rpc['name']}. Reordering/renaming is forbidden.")
                success = False
            if b_rpc["req"] != c_rpc["req"]:
                common.report_error(f"Service {srv_name} RPC {b_rpc['name']} req changed from {b_rpc['req']} to {c_rpc['req']}.")
                success = False
            if b_rpc["resp"] != c_rpc["resp"]:
                common.report_error(f"Service {srv_name} RPC {b_rpc['name']} resp changed from {b_rpc['resp']} to {c_rpc['resp']}.")
                success = False

        # Check Enums
        for enum_name, b_enum_vals in base_srv.get("enums", {}).items():
            if enum_name not in curr_srv.get("enums", {}):
                common.report_error(f"Enum {enum_name} in service {srv_name} was removed.")
                success = False
                continue

            c_enum_vals = curr_srv["enums"][enum_name]
            b_val_dict = {v["name"]: v["value"] for v in b_enum_vals}
            c_val_dict = {v["name"]: v["value"] for v in c_enum_vals}

            for k, v in b_val_dict.items():
                if k not in c_val_dict:
                    common.report_error(f"Enum value {k} was removed from {enum_name}.")
                    success = False
                elif c_val_dict[k] != v:
                    common.report_error(f"Enum value {k} changed from {v} to {c_val_dict[k]} in {enum_name}.")
                    success = False

        # Check Messages/Structs
        for msg_name, b_fields in base_srv.get("messages", {}).items():
            if msg_name not in curr_srv.get("messages", {}):
                common.report_error(f"Message/Struct {msg_name} in service {srv_name} was removed.")
                success = False
                continue

            c_fields = curr_srv["messages"][msg_name]

            if len(c_fields) < len(b_fields):
                common.report_error(f"Message {msg_name} has fewer fields. Fields can only be appended.")
                success = False

            for i, b_field in enumerate(b_fields):
                if i >= len(c_fields):
                    break
                c_field = c_fields[i]
                if b_field["name"] != c_field["name"]:
                    common.report_error(f"Message {msg_name} field {i} name changed from {b_field['name']} to {c_field['name']}.")
                    success = False
                if b_field["type"] != c_field["type"]:
                    common.report_error(f"Message {msg_name} field {b_field['name']} type changed from {b_field['type']} to {c_field['type']}.")
                    success = False

    return success

if __name__ == "__main__":
    import json
    curr = generate_idl_manifest()
    print(json.dumps(curr, indent=2))
