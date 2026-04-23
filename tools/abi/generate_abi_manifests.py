#!/usr/bin/env python3
import sys
import argparse
from check_syscalls import check_syscalls, generate_syscall_manifest
from check_struct_layouts import check_struct_layouts, generate_struct_manifest
from check_idl_compat import check_idl_compat, generate_idl_manifest
from check_sdk_symbols import check_sdk_symbols, generate_sdk_manifest
import common

MANIFEST_DIR = "contracts/abi"

def get_manifest_path(name):
    return f"{MANIFEST_DIR}/{name}.json"

def main():
    parser = argparse.ArgumentParser(description="ABI Manifest Generator and Checker")
    parser.add_argument('--check', action='store_true', help="Check current tree against baseline manifests")
    parser.add_argument('--update', action='store_true', help="Update baseline manifests with current tree")

    args = parser.parse_args()

    if not args.check and not args.update:
        print("Please specify either --check or --update")
        sys.exit(1)

    syscalls_curr = generate_syscall_manifest()
    structs_curr = generate_struct_manifest()
    idl_curr = generate_idl_manifest()
    sdk_curr = generate_sdk_manifest()

    if args.update:
        print("Updating manifests...")
        common.save_manifest(get_manifest_path("syscalls"), syscalls_curr)
        common.save_manifest(get_manifest_path("carrier_layouts"), structs_curr)
        common.save_manifest(get_manifest_path("idl_compat"), idl_curr)
        common.save_manifest(get_manifest_path("sdk_symbols"), sdk_curr)
        print("Done.")
        sys.exit(0)

    if args.check:
        print("Checking manifests...")
        success = True

        syscalls_base = common.load_manifest(get_manifest_path("syscalls"))
        if not check_syscalls(syscalls_base, syscalls_curr):
            success = False

        structs_base = common.load_manifest(get_manifest_path("carrier_layouts"))
        if not check_struct_layouts(structs_base, structs_curr):
            success = False

        idl_base = common.load_manifest(get_manifest_path("idl_compat"))
        if not check_idl_compat(idl_base, idl_curr):
            success = False

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
