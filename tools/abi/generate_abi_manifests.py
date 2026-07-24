#!/usr/bin/env python3
import sys
import argparse
import os
import subprocess
from pathlib import Path
from check_struct_layouts import check_struct_layouts, generate_struct_manifest
from check_idl_compat import check_idl_compat, generate_idl_manifest
from check_sdk_symbols import check_sdk_symbols, generate_sdk_manifest
import common
from tools.build.path_aliases import resolve_abi_manifest_alias

MANIFEST_DIR_CANDIDATES = [
    "interface/contracts/abi",
    "contracts/abi",
]

def resolve_manifest_dir():
    for path in MANIFEST_DIR_CANDIDATES:
        resolved_path, used_alias = resolve_abi_manifest_alias(Path(path))
        if resolved_path.exists():
            if used_alias:
                print(f"[migration-warning] Using aliased ABI manifest path: {path} -> {resolved_path}")
            return str(resolved_path)
    return MANIFEST_DIR_CANDIDATES[0]

def get_manifest_path(name):
    manifest_dir = resolve_manifest_dir()
    return f"{manifest_dir}/{name}.json"

def main():
    parser = argparse.ArgumentParser(description="ABI Manifest Generator and Checker")
    parser.add_argument('--check', action='store_true', help="Check current tree against baseline manifests")
    parser.add_argument('--update', action='store_true', help="Update baseline manifests with current tree")

    args = parser.parse_args()

    if not args.check and not args.update:
        print("Please specify either --check or --update")
        sys.exit(1)

    # Resolve python interpreter
    python_bin = sys.executable

    # Find the directory of this script to locate syscall_abi.py
    script_dir = os.path.dirname(os.path.abspath(__file__))
    syscall_abi_path = os.path.join(script_dir, "syscall_abi.py")

    if args.update:
        print("Updating manifests...")
        # Update lock file
        res = subprocess.run([python_bin, syscall_abi_path, "--update-lock"])
        if res.returncode != 0:
            print("Failed to update syscall ABI lock file.")
            sys.exit(1)

        structs_curr = generate_struct_manifest()
        idl_curr = generate_idl_manifest()
        sdk_curr = generate_sdk_manifest()

        common.save_manifest(get_manifest_path("carrier_layouts"), structs_curr)
        common.save_manifest(get_manifest_path("idl_compat"), idl_curr)
        common.save_manifest(get_manifest_path("sdk_symbols"), sdk_curr)
        print("Done.")
        sys.exit(0)

    if args.check:
        print("Checking manifests...")
        success = True

        # Run Syscall ABI check
        print("--- Running Syscall ABI check ---")
        res = subprocess.run([python_bin, syscall_abi_path, "--check"])
        if res.returncode != 0:
            success = False

        print("--- Running Struct Layout check ---")
        structs_curr = generate_struct_manifest()
        structs_base = common.load_manifest(get_manifest_path("carrier_layouts"))
        if not check_struct_layouts(structs_base, structs_curr):
            success = False

        print("--- Running IDL Compatibility check ---")
        idl_curr = generate_idl_manifest()
        idl_base = common.load_manifest(get_manifest_path("idl_compat"))
        if not check_idl_compat(idl_base, idl_curr):
            success = False

        print("--- Running SDK Symbols check ---")
        sdk_curr = generate_sdk_manifest()
        sdk_base = common.load_manifest(get_manifest_path("sdk_symbols"))
        if not check_sdk_symbols(sdk_base, sdk_curr):
            success = False

        if success:
            print("All ABI compatibility checks passed.")
            sys.exit(0)
        else:
            print("ABI compatibility checks failed.")
            sys.exit(1)

if __name__ == "__main__":
    main()
