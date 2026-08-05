import os
import json
import sys
from pathlib import Path

# Add repo root to sys.path so we can import from tools.*
REPO_ROOT = Path(__file__).resolve().parent.parent.parent
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

import tools.abi.common as common
from tools.build.path_aliases import resolve_idl_alias
from tools.bidl.parser import parse_bidl

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
    curr = generate_idl_manifest()
    print(json.dumps(curr, indent=2))
