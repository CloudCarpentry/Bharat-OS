#!/usr/bin/env python3
import os
import sys
import json
import re
import argparse

# Traits translation to C flags
TRAIT_FLAGS = {
    "fast": "BH_SYSCALL_F_FAST",
    "blocking": "BH_SYSCALL_F_BLOCKING",
    "user_read": "BH_SYSCALL_F_USER_READ",
    "user_write": "BH_SYSCALL_F_USER_WRITE",
    "audit": "BH_SYSCALL_F_AUDIT",
    "service-call": "BH_SYSCALL_F_SERVICE_CALL"
}

CLASS_MAPPING = {
    "system": "BH_SYS_CLASS_SYSTEM",
    "process": "BH_SYS_CLASS_PROCESS",
    "memory": "BH_SYS_CLASS_MEMORY",
    "ipc": "BH_SYS_CLASS_IPC",
    "io": "BH_SYS_CLASS_IO",
    "capability": "BH_SYS_CLASS_CAPABILITY"
}

KIND_MAPPING = {
    "register": "BH_SYS_CAP_SOURCE_REGISTER",
    "struct_field": "BH_SYS_CAP_SOURCE_STRUCT_FIELD",
    "implicit_current_process": "BH_SYS_CAP_SOURCE_IMPLICIT_PROCESS",
    "implicit_current_thread": "BH_SYS_CAP_SOURCE_IMPLICIT_THREAD",
}

VAL_PHASE_MAPPING = {
    "before_handler": "BH_SYS_CAP_VAL_BEFORE_HANDLER",
    "after_usercopy": "BH_SYS_CAP_VAL_AFTER_USERCOPY",
}

def load_json(path):
    if not os.path.exists(path):
        return None
    with open(path, 'r') as f:
        return json.load(f)

def save_json(path, data):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w') as f:
        json.dump(data, f, indent=2)
        f.write('\n')

def validate_schema(manifest):
    if "version" not in manifest or "syscalls" not in manifest:
        print("Schema Error: Missing top-level fields 'version' or 'syscalls'")
        return False

    for sc in manifest["syscalls"]:
        required_keys = ["number", "symbol", "name", "status", "class", "handler", "arguments", "traits"]
        for key in required_keys:
            if key not in sc:
                print(f"Schema Error: Syscall {sc.get('symbol', 'unknown')} missing required key '{key}'")
                return False

        # Validate arguments schema
        for arg in sc["arguments"]:
            arg_keys = ["name", "kind", "type", "direction"]
            for ak in arg_keys:
                if ak not in arg:
                    print(f"Schema Error: Argument in {sc['symbol']} missing key '{ak}'")
                    return False
            if arg["kind"] == "pointer":
                if "size_source" not in arg:
                    print(f"Schema Error: Pointer argument '{arg['name']}' in {sc['symbol']} must define 'size_source'")
                    return False

        # Validate traits and capability
        if sc["capability"] is not None:
            cap = sc["capability"]
            cap_keys = ["source", "object_type", "rights", "scope"]
            for ck in cap_keys:
                if ck not in cap:
                    print(f"Schema Error: Capability for {sc['symbol']} missing key '{ck}'")
                    return False

            source = cap["source"]
            if isinstance(source, dict):
                if "kind" not in source:
                    print(f"Schema Error: Capability source object in {sc['symbol']} must have 'kind'")
                    return False
                if source["kind"] in ["register", "struct_field"]:
                    if "argument" not in source:
                        print(f"Schema Error: Capability source object {source['kind']} in {sc['symbol']} must have 'argument'")
                        return False
                if source["kind"] == "struct_field":
                    if "field" not in source:
                        print(f"Schema Error: Capability source object struct_field in {sc['symbol']} must have 'field'")
                        return False

    return True

def validate_semantics(manifest):
    # Uniqueness checks
    numbers = []
    symbols = []
    names = []

    for sc in manifest["syscalls"]:
        num = sc["number"]
        sym = sc["symbol"]
        name = sc["name"]

        if num in numbers:
            print(f"Semantic Error: Duplicate syscall number {num}")
            return False
        numbers.append(num)

        if sym in symbols:
            print(f"Semantic Error: Duplicate syscall symbol {sym}")
            return False
        symbols.append(sym)

        if name in names:
            print(f"Semantic Error: Duplicate syscall name {name}")
            return False
        names.append(name)

        # Enforce reserved range
        if num < 0 or num > 1024:
            print(f"Semantic Error: Syscall number {num} out of allowed range (0-1024)")
            return False

        # Trait compatibility
        traits = sc["traits"]
        if "fast" in traits and "blocking" in traits:
            print(f"Semantic Error: Syscall {sym} cannot be both 'fast' and 'blocking'")
            return False
        if "fast" in traits and ("user_read" in traits or "user_write" in traits):
            print(f"Semantic Error: Syscall {sym} cannot be 'fast' and use usercopy (user_read/user_write)")
            return False

        # Capability index matching
        if sc["capability"] is not None:
            source = sc["capability"]["source"]
            if isinstance(source, dict):
                kind = source["kind"]
                if kind in ["register", "struct_field"]:
                    arg_name = source["argument"]
                else:
                    arg_name = None
            else:
                kind = "register"
                arg_name = source

            if arg_name is not None:
                arg_names = [a["name"] for a in sc["arguments"]]
                if arg_name not in arg_names:
                    print(f"Semantic Error: Syscall {sym} capability source '{arg_name}' not in arguments list")
                    return False

    return True

def check_raw_numbers_in_source():
    success = True
    dirs = ['experience', 'services', 'tests', 'lib', 'core', 'interface', 'quality']
    pattern_syscall = re.compile(r'\bbharat_syscall\s*\(\s*[0-9]+\b')
    pattern_dummy = re.compile(r'\b1001\b')

    for d in dirs:
        if not os.path.exists(d):
            continue
        for root, _, files in os.walk(d):
            for file in files:
                if not (file.endswith('.c') or file.endswith('.h') or file.endswith('.S') or file.endswith('.cpp')):
                    continue
                filepath = os.path.join(root, file)
                if "syscall_abi.py" in filepath or "check_syscalls.py" in filepath:
                    continue
                try:
                    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                        for line_idx, line in enumerate(f, 1):
                            if pattern_syscall.search(line):
                                print(f"Raw Syscall Code Error: Raw numeric syscall invocation in {filepath}:{line_idx}: {line.strip()}")
                                success = False
                            if pattern_dummy.search(line):
                                if "1001" in line and not line.strip().startswith("//") and not line.strip().startswith("/*"):
                                    print(f"Raw Syscall Code Error: Dummy syscall number 1001 in {filepath}:{line_idx}: {line.strip()}")
                                    success = False
                except Exception as e:
                    print(f"Warning: Could not read {filepath}: {e}")
    return success

def compare_lock(manifest, lock_data):
    if not lock_data:
        print("Lock Check Error: Lock file data is missing.")
        return False

    lock_syscalls = {sc["number"]: sc for sc in lock_data.get("syscalls", [])}
    manifest_syscalls = {sc["number"]: sc for sc in manifest["syscalls"]}

    for num, l_sc in lock_syscalls.items():
        if num not in manifest_syscalls:
            print(f"ABI Breakage Error: Syscall {l_sc['symbol']} ({num}) was removed from the manifest. Syscall deletions are forbidden.")
            return False

        m_sc = manifest_syscalls[num]

        # Check basic properties
        if m_sc["symbol"] != l_sc["symbol"]:
            print(f"ABI Breakage Error: Syscall number {num} changed its symbol from {l_sc['symbol']} to {m_sc['symbol']}. Renaming or renumbering is forbidden.")
            return False

        # Validate arguments list
        l_args = l_sc.get("arguments", [])
        m_args = m_sc.get("arguments", [])
        if len(l_args) != len(m_args):
            print(f"ABI Breakage Error: Syscall {m_sc['symbol']} changed argument count from {len(l_args)} to {len(m_args)}.")
            return False

        for i, (l_arg, m_arg) in enumerate(zip(l_args, m_args)):
            for field in ["name", "kind", "type", "direction", "size_source"]:
                if l_arg.get(field) != m_arg.get(field):
                    print(f"ABI Breakage Error: Syscall {m_sc['symbol']} argument {i} changed field '{field}' from '{l_arg.get(field)}' to '{m_arg.get(field)}'.")
                    return False

        # Validate capability
        l_cap = l_sc.get("capability")
        m_cap = m_sc.get("capability")
        if (l_cap is None) != (m_cap is None):
            print(f"ABI Breakage Error: Syscall {m_sc['symbol']} changed capability presence.")
            return False

        if l_cap is not None:
            # Check fields
            for field in ["object_type", "scope", "validation_phase"]:
                if l_cap.get(field) != m_cap.get(field):
                    print(f"ABI Breakage Error: Syscall {m_sc['symbol']} capability changed field '{field}' from '{l_cap.get(field)}' to '{m_cap.get(field)}'.")
                    return False
            # Check source
            if l_cap.get("source") != m_cap.get("source"):
                print(f"ABI Breakage Error: Syscall {m_sc['symbol']} capability source changed from '{l_cap.get('source')}' to '{m_cap.get('source')}'.")
                return False
            # Check rights (order-independent comparison)
            if sorted(l_cap.get("rights", [])) != sorted(m_cap.get("rights", [])):
                print(f"ABI Breakage Error: Syscall {m_sc['symbol']} capability rights changed from {l_cap.get('rights')} to {m_cap.get('rights')}.")
                return False

        # Validate traits
        if sorted(l_sc.get("traits", [])) != sorted(m_sc.get("traits", [])):
            print(f"ABI Breakage Error: Syscall {m_sc['symbol']} traits changed from {l_sc.get('traits')} to {m_sc.get('traits')}.")
            return False

    return True

def generate_headers(manifest, output_inc, output_def, output_numbers):
    os.makedirs(os.path.dirname(output_inc), exist_ok=True)
    os.makedirs(os.path.dirname(output_def), exist_ok=True)
    os.makedirs(os.path.dirname(output_numbers), exist_ok=True)

    # 1. Generate numbers.h
    with open(output_numbers, 'w') as f:
        f.write("/* Generated - do not edit. Handled by tools/abi/syscall_abi.py */\n")
        f.write("#ifndef BHARAT_UAPI_SYSCALL_GENERATED_NUMBERS_H\n")
        f.write("#define BHARAT_UAPI_SYSCALL_GENERATED_NUMBERS_H\n\n")

        for sc in manifest["syscalls"]:
            f.write(f"#define {sc['symbol']:<30} {sc['number']}\n")

        f.write(f"\n#define BH_SYSCALL_COUNT               {len(manifest['syscalls'])}\n")
        f.write("#define BHARAT_SYSCALL_ABI_VERSION     1\n\n")
        f.write("#endif /* BHARAT_UAPI_SYSCALL_GENERATED_NUMBERS_H */\n")

    # 2. Generate table.def
    with open(output_def, 'w') as f:
        f.write("/* Generated - do not edit. Handled by tools/abi/syscall_abi.py */\n")
        for sc in manifest["syscalls"]:
            f.write(f"SYSCALL_DEF({sc['symbol']}, {sc['number']})\n")

    # 3. Generate native_syscall_table.inc
    with open(output_inc, 'w') as f:
        f.write("/* Generated - do not edit. Handled by tools/abi/syscall_abi.py */\n")
        for sc in manifest["syscalls"]:
            sym = sc["symbol"]
            name = sc["name"]
            num = sc["number"]
            class_name = CLASS_MAPPING.get(sc["class"], "BH_SYS_CLASS_NONE")
            arg_count = len(sc["arguments"])
            handler = sc["handler"]

            # Capability metadata
            if sc["capability"] is not None:
                cap = sc["capability"]
                # Get index of source
                source_idx = "BH_SYS_CAP_INDEX_NONE"
                source = cap["source"]
                if isinstance(source, dict):
                    kind = source["kind"]
                    arg_name = source.get("argument")
                    field_name = source.get("field", "")
                    val_phase = cap.get("validation_phase", "before_handler")
                else:
                    kind = "register"
                    arg_name = source
                    field_name = ""
                    val_phase = "before_handler"

                if arg_name is not None:
                    for i, arg in enumerate(sc["arguments"]):
                        if arg["name"] == arg_name:
                            source_idx = str(i)
                            break
                rights_str = " | ".join(cap["rights"]) if cap["rights"] else "0"
                obj_type = cap["object_type"]
                cap_kind_str = KIND_MAPPING.get(kind, "BH_SYS_CAP_SOURCE_NONE")
                cap_field_str = f'"{field_name}"' if field_name else "NULL"
                cap_phase_str = VAL_PHASE_MAPPING.get(val_phase, "BH_SYS_CAP_VAL_NONE")
            else:
                source_idx = "BH_SYS_CAP_INDEX_NONE"
                rights_str = "0"
                obj_type = "CAP_TYPE_NONE"
                cap_kind_str = "BH_SYS_CAP_SOURCE_NONE"
                cap_field_str = "NULL"
                cap_phase_str = "BH_SYS_CAP_VAL_NONE"
                val_phase = "none"

            # Compute flags
            flags = []
            if sc["capability"] is not None and val_phase == "before_handler":
                flags.append("BH_SYSCALL_F_CAP_REQUIRED")
            for t in sc["traits"]:
                if t in TRAIT_FLAGS:
                    flags.append(TRAIT_FLAGS[t])
            flags_str = " | ".join(flags) if flags else "0"

            f.write(f"[{sym}] = {{\n")
            f.write(f"    .nr = {sym},\n")
            f.write(f"    .name = \"{name}\",\n")
            f.write(f"    .class_id = {class_name},\n")
            f.write(f"    .arg_count = {arg_count},\n")
            f.write(f"    .flags = {flags_str},\n")
            f.write(f"    .required_rights = {rights_str},\n")
            f.write(f"    .cap_arg_index = {source_idx},\n")
            f.write(f"    .required_cap_type = {obj_type},\n")
            f.write(f"    .cap_source_kind = {cap_kind_str},\n")
            f.write(f"    .cap_source_field = {cap_field_str},\n")
            f.write(f"    .cap_val_phase = {cap_phase_str},\n")
            f.write(f"    .handler = {handler}\n")
            f.write("},\n")

def main():
    parser = argparse.ArgumentParser(description="Canonical Syscall ABI Tool")
    parser.add_argument("--manifest", default="interface/contracts/abi/native_syscalls.json")
    parser.add_argument("--lock", default="interface/contracts/abi/native_syscalls.lock.json")
    parser.add_argument("--generate", action="store_true")
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--update-lock", action="store_true")
    parser.add_argument("--output-inc", default="build/generated/kernel/syscall/native_syscall_table.inc")
    parser.add_argument("--output-def", default="build/generated/include/bharat/uapi/syscall/generated/table.def")
    parser.add_argument("--output-numbers", default="build/generated/include/bharat/uapi/syscall/generated/numbers.h")

    args = parser.parse_args()

    # Always load and validate the rich manifest
    manifest = load_json(args.manifest)
    if not manifest:
        print(f"Error: Manifest file '{args.manifest}' not found.")
        sys.exit(1)

    if not validate_schema(manifest):
        sys.exit(1)

    if not validate_semantics(manifest):
        sys.exit(1)

    if args.generate:
        generate_headers(manifest, args.output_inc, args.output_def, args.output_numbers)
        print("Successfully generated all build artifacts.")
        sys.exit(0)

    if args.check:
        lock_data = load_json(args.lock)
        if not lock_data:
            print(f"Error: Lock file '{args.lock}' not found. Cannot perform check.")
            sys.exit(1)

        if not compare_lock(manifest, lock_data):
            sys.exit(1)

        if not check_raw_numbers_in_source():
            sys.exit(1)

        print("Syscall ABI Check Passed cleanly.")
        sys.exit(0)

    if args.update_lock:
        lock_syscalls = [
            {
                "number": sc["number"],
                "symbol": sc["symbol"],
                "arguments": sc["arguments"],
                "capability": sc["capability"],
                "traits": sc["traits"]
            }
            for sc in manifest["syscalls"]
        ]
        save_json(args.lock, {"version": manifest["version"], "syscalls": lock_syscalls})
        print(f"Updated lock file: {args.lock}")
        sys.exit(0)

if __name__ == "__main__":
    main()
