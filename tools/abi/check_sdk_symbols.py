import os
import subprocess
import json
import common

# The SDK is currently mostly placeholders.
# We will define this script to run `nm` on specific library paths
# if they exist, or just return an empty manifest for now.
# Users can add paths to SDK_LIBS when the SDK is fleshed out.

SDK_LIBS = [
    # e.g., "build/sdk/lib/libbharat.a"
]

def get_symbols_from_binary(filepath):
    # This is a stub implementation that would invoke `nm -g <file>`
    # and parse out defined symbols (T, D, R, etc).
    try:
        result = subprocess.run(["nm", "-g", "--defined-only", filepath], capture_output=True, text=True, check=True)
        symbols = []
        for line in result.stdout.split('\n'):
            line = line.strip()
            if not line:
                continue
            parts = line.split()
            if len(parts) >= 3:
                # address type name
                symbols.append({"name": parts[2], "type": parts[1]})
            elif len(parts) == 2:
                symbols.append({"name": parts[1], "type": parts[0]})
        return symbols
    except Exception as e:
        common.report_error(f"Failed to extract symbols from {filepath}: {e}")
        return []

def generate_sdk_manifest():
    manifest = {}
    for lib in SDK_LIBS:
        if os.path.exists(lib):
            manifest[os.path.basename(lib)] = get_symbols_from_binary(lib)
    return manifest

def check_sdk_symbols(baseline, current):
    if baseline is None:
        return False

    success = True

    for lib_name, base_symbols in baseline.items():
        if lib_name not in current:
            # We don't necessarily fail here if the library isn't built in this CI job,
            # but for strict ABI checks, we assume the user builds the SDK first.
            common.report_error(f"SDK library {lib_name} missing from current build.")
            success = False
            continue

        curr_symbols = {s["name"]: s["type"] for s in current[lib_name]}

        for b_sym in base_symbols:
            if b_sym["name"] not in curr_symbols:
                common.report_error(f"Symbol {b_sym['name']} was removed from {lib_name}. Symbol removal is forbidden.")
                success = False
            elif curr_symbols[b_sym["name"]] != b_sym["type"]:
                common.report_error(f"Symbol {b_sym['name']} in {lib_name} changed type from {b_sym['type']} to {curr_symbols[b_sym['name']]}.")
                success = False

    return success

if __name__ == "__main__":
    curr = generate_sdk_manifest()
    print(json.dumps(curr, indent=2))
