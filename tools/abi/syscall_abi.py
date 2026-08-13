#!/usr/bin/env python3
import os
import sys
import json
import re
import argparse
import hashlib
import tempfile

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

ALLOWED_TOP_LEVEL_KEYS = {"version", "syscalls"}
ALLOWED_SYSCALL_KEYS = {
    "number", "symbol", "name", "status", "class", "handler",
    "arguments", "capability", "traits",
}
ALLOWED_ARGUMENT_KEYS = {"name", "kind", "type", "direction", "size_source"}
ALLOWED_ARGUMENT_KINDS = {"scalar", "pointer", "user_struct"}
ALLOWED_DIRECTIONS = {"in", "out", "in_out"}
ALLOWED_STATUSES = {"stable", "experimental", "deprecated"}
ALLOWED_CAPABILITY_KEYS = {
    "source", "validation_phase", "object_type", "rights", "scope",
}
ALLOWED_CAPABILITY_SOURCE_KEYS = {"kind", "argument", "field"}
ALLOWED_CAPABILITY_SCOPES = {"current_process", "current_thread", "system"}
IDENTIFIER_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
TYPE_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_ ]*(?:\s*\*)?$")
SIZEOF_RE = re.compile(r"^sizeof\([A-Za-z_][A-Za-z0-9_]*\)$")


def schema_error(message):
    print(f"Schema Error: {message}")
    return False


def exact_keys(value, allowed, context):
    unknown = set(value) - allowed
    if unknown:
        return schema_error(f"{context} has unknown key(s): {', '.join(sorted(unknown))}")
    return True

VAL_PHASE_MAPPING = {
    "before_handler": "BH_SYS_CAP_VAL_BEFORE_HANDLER",
    "after_usercopy": "BH_SYS_CAP_VAL_AFTER_USERCOPY",
}

GENERATED_OUTPUT_KEYS = {
    "numbers": "numbers.h",
    "table_def": "table.def",
    "metadata_table": "native_syscall_table.inc",
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
    if not isinstance(manifest, dict) or "version" not in manifest or "syscalls" not in manifest:
        return schema_error("Missing top-level fields 'version' or 'syscalls'")
    if not exact_keys(manifest, ALLOWED_TOP_LEVEL_KEYS, "Manifest"):
        return False
    if not isinstance(manifest["version"], int) or not isinstance(manifest["syscalls"], list):
        print("Schema Error: 'version' must be an integer and 'syscalls' must be a list")
        return False

    for sc in manifest["syscalls"]:
        if not isinstance(sc, dict):
            print("Schema Error: Every syscall entry must be an object")
            return False
        required_keys = ["number", "symbol", "name", "status", "class", "handler", "arguments", "capability", "traits"]
        for key in required_keys:
            if key not in sc:
                print(f"Schema Error: Syscall {sc.get('symbol', 'unknown')} missing required key '{key}'")
                return False
        if not exact_keys(sc, ALLOWED_SYSCALL_KEYS, f"Syscall {sc['symbol']}"):
            return False
        if not isinstance(sc["arguments"], list) or not isinstance(sc["traits"], list):
            return schema_error(f"Syscall {sc['symbol']} arguments and traits must be lists")

        # Validate arguments schema
        for arg in sc["arguments"]:
            if not isinstance(arg, dict):
                return schema_error(f"Argument in {sc['symbol']} must be an object")
            arg_keys = ["name", "kind", "type", "direction"]
            for ak in arg_keys:
                if ak not in arg:
                    print(f"Schema Error: Argument in {sc['symbol']} missing key '{ak}'")
                    return False
            if not exact_keys(arg, ALLOWED_ARGUMENT_KEYS,
                              f"Argument '{arg['name']}' in {sc['symbol']}"):
                return False
            if arg["kind"] not in ALLOWED_ARGUMENT_KINDS:
                return schema_error(
                    f"Argument '{arg['name']}' in {sc['symbol']} has invalid kind '{arg['kind']}'")
            if arg["direction"] not in ALLOWED_DIRECTIONS:
                return schema_error(
                    f"Argument '{arg['name']}' in {sc['symbol']} has invalid direction '{arg['direction']}'")
            if not isinstance(arg["name"], str) or not IDENTIFIER_RE.fullmatch(arg["name"]):
                return schema_error(f"Argument name in {sc['symbol']} is not a C identifier")
            if not isinstance(arg["type"], str) or not TYPE_RE.fullmatch(arg["type"]):
                return schema_error(f"Argument '{arg['name']}' in {sc['symbol']} has an invalid type")
            if arg["kind"] == "pointer":
                if "size_source" not in arg:
                    print(f"Schema Error: Pointer argument '{arg['name']}' in {sc['symbol']} must define 'size_source'")
                    return False
                if not isinstance(arg["size_source"], str) or not arg["size_source"]:
                    return schema_error(
                        f"Pointer argument '{arg['name']}' in {sc['symbol']} has an invalid size_source")
            elif "size_source" in arg:
                return schema_error(
                    f"Non-pointer argument '{arg['name']}' in {sc['symbol']} cannot define size_source")

        # Validate traits and capability
        if sc["capability"] is not None:
            cap = sc["capability"]
            if not isinstance(cap, dict):
                return schema_error(f"Capability for {sc['symbol']} must be an object or null")
            cap_keys = ["source", "validation_phase", "object_type", "rights", "scope"]
            for ck in cap_keys:
                if ck not in cap:
                    print(f"Schema Error: Capability for {sc['symbol']} missing key '{ck}'")
                    return False
            if not exact_keys(cap, ALLOWED_CAPABILITY_KEYS, f"Capability for {sc['symbol']}"):
                return False

            source = cap["source"]
            if not isinstance(source, dict):
                return schema_error(f"Capability source in {sc['symbol']} must be an object")
            if "kind" not in source:
                return schema_error(f"Capability source object in {sc['symbol']} must have 'kind'")
            if not exact_keys(source, ALLOWED_CAPABILITY_SOURCE_KEYS,
                              f"Capability source for {sc['symbol']}"):
                return False
            kind = source["kind"]
            if kind not in KIND_MAPPING:
                return schema_error(f"Capability source in {sc['symbol']} has invalid kind '{kind}'")
            required_source_keys = {"kind"}
            if kind in ["register", "struct_field"]:
                required_source_keys.add("argument")
            if kind == "struct_field":
                required_source_keys.add("field")
            if set(source) != required_source_keys:
                return schema_error(
                    f"Capability source {kind} in {sc['symbol']} must contain exactly "
                    f"{', '.join(sorted(required_source_keys))}")
            if cap["validation_phase"] not in VAL_PHASE_MAPPING:
                return schema_error(f"Capability for {sc['symbol']} has invalid validation_phase")
            if cap["scope"] not in ALLOWED_CAPABILITY_SCOPES:
                return schema_error(f"Capability for {sc['symbol']} has invalid scope")
            if not isinstance(cap["rights"], list) or not cap["rights"]:
                return schema_error(f"Capability for {sc['symbol']} must require at least one right")

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

        if not IDENTIFIER_RE.fullmatch(sym) or not IDENTIFIER_RE.fullmatch(name):
            print(f"Semantic Error: Syscall {sym} symbol and name must be C identifiers")
            return False
        if sc["status"] not in ALLOWED_STATUSES or sc["class"] not in CLASS_MAPPING:
            print(f"Semantic Error: Syscall {sym} has an unsupported status or class")
            return False
        if not IDENTIFIER_RE.fullmatch(sc["handler"]):
            print(f"Semantic Error: Syscall {sym} handler must be a C identifier")
            return False

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
        if len(traits) != len(set(traits)) or any(t not in TRAIT_FLAGS for t in traits):
            print(f"Semantic Error: Syscall {sym} has duplicate or unknown traits")
            return False
        if "fast" in traits and "blocking" in traits:
            print(f"Semantic Error: Syscall {sym} cannot be both 'fast' and 'blocking'")
            return False

        arg_names = [a["name"] for a in sc["arguments"]]
        if len(arg_names) != len(set(arg_names)):
            print(f"Semantic Error: Syscall {sym} has duplicate argument names")
            return False
        for arg in sc["arguments"]:
            if arg["kind"] != "pointer":
                continue
            size_source = arg["size_source"]
            if size_source not in arg_names and not SIZEOF_RE.fullmatch(size_source):
                print(f"Semantic Error: Pointer {arg['name']} in {sym} has unresolved size_source '{size_source}'")
                return False
            if size_source in arg_names:
                size_arg = sc["arguments"][arg_names.index(size_source)]
                if size_arg["kind"] != "scalar" or size_arg["direction"] != "in":
                    print(f"Semantic Error: Pointer {arg['name']} in {sym} size_source must be an input scalar")
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
                if arg_name not in arg_names:
                    print(f"Semantic Error: Syscall {sym} capability source '{arg_name}' not in arguments list")
                    return False
                source_arg = sc["arguments"][arg_names.index(arg_name)]
                if kind == "register" and source_arg["kind"] != "scalar":
                    print(f"Semantic Error: Register capability source in {sym} must reference a scalar")
                    return False
                if kind == "struct_field" and source_arg["kind"] != "user_struct":
                    print(f"Semantic Error: Struct-field capability source in {sym} must reference a user_struct")
                    return False
            phase = sc["capability"]["validation_phase"]
            if kind == "struct_field" and phase != "after_usercopy":
                print(f"Semantic Error: Struct-field capability source in {sym} must validate after_usercopy")
                return False
            if kind != "struct_field" and phase != "before_handler":
                print(f"Semantic Error: Capability source in {sym} must validate before_handler")
                return False
            rights = sc["capability"]["rights"]
            if len(rights) != len(set(rights)) or any(not IDENTIFIER_RE.fullmatch(r) for r in rights):
                print(f"Semantic Error: Capability rights in {sym} must be unique C identifiers")
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

    lock_entries = lock_data.get("syscalls", [])
    if lock_data.get("syscall_count") != len(lock_entries):
        print("ABI Lock Error: Locked syscall metadata count disagrees with its syscall table.")
        return False
    if len(lock_entries) != len(manifest["syscalls"]):
        print("ABI Lock Error: Manifest syscall count differs from the lock; "
              "intentional additions require --update-lock.")
        return False

    lock_syscalls = {sc["symbol"]: sc for sc in lock_entries}
    manifest_syscalls = {sc["symbol"]: sc for sc in manifest["syscalls"]}

    for symbol, l_sc in lock_syscalls.items():
        if symbol not in manifest_syscalls:
            print(f"ABI Breakage Error: Syscall {symbol} ({l_sc['number']}) was removed or renamed. Syscall deletions and renames are forbidden.")
            return False

        m_sc = manifest_syscalls[symbol]

        # Check basic properties
        if m_sc["number"] != l_sc["number"]:
            print(f"ABI Breakage Error: Syscall {symbol} changed number from {l_sc['number']} to {m_sc['number']}. Renumbering is forbidden.")
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

def sha256_file(path):
    digest = hashlib.sha256()
    with open(path, "rb") as generated_file:
        for chunk in iter(lambda: generated_file.read(65536), b""):
            digest.update(chunk)
    return digest.hexdigest()

def generate_to_directory(manifest, output_dir):
    paths = {
        key: os.path.join(output_dir, filename)
        for key, filename in GENERATED_OUTPUT_KEYS.items()
    }
    generate_headers(manifest, paths["metadata_table"], paths["table_def"], paths["numbers"])
    return paths

def generated_hashes(manifest):
    with tempfile.TemporaryDirectory(prefix="bharat-syscall-abi-") as output_dir:
        first_paths = generate_to_directory(manifest, os.path.join(output_dir, "first"))
        second_paths = generate_to_directory(manifest, os.path.join(output_dir, "second"))
        first = {key: sha256_file(path) for key, path in first_paths.items()}
        second = {key: sha256_file(path) for key, path in second_paths.items()}
        if first != second:
            print("Generation Error: syscall outputs are not reproducible.")
            return None
        return first

def verify_generated(manifest, lock_data):
    expected = lock_data.get("generated_sha256")
    if not isinstance(expected, dict) or set(expected) != set(GENERATED_OUTPUT_KEYS):
        print("ABI Lock Error: generated output hashes are missing or incomplete; run --update-lock intentionally.")
        return False

    actual = generated_hashes(manifest)
    if actual is None:
        return False
    success = True
    for key in GENERATED_OUTPUT_KEYS:
        if actual[key] != expected[key]:
            print(f"Generated Output Drift Error: {GENERATED_OUTPUT_KEYS[key]} differs from the locked authority.")
            success = False
    return success

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
    actions = parser.add_mutually_exclusive_group(required=True)
    actions.add_argument("--generate", action="store_true", help="generate build-tree outputs")
    actions.add_argument("--check", action="store_true", help="validate the manifest, lock, and reproducible generated-output hashes")
    actions.add_argument("--update-lock", action="store_true", help="intentionally replace the compatibility lock and generated-output hashes")
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

        if not verify_generated(manifest, lock_data):
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
        hashes = generated_hashes(manifest)
        if hashes is None:
            sys.exit(1)
        save_json(args.lock, {
            "version": manifest["version"],
            "syscall_count": len(lock_syscalls),
            "generated_sha256": hashes,
            "syscalls": lock_syscalls,
        })
        print(f"Updated lock file: {args.lock}")
        sys.exit(0)

if __name__ == "__main__":
    main()
