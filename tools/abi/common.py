import json
import os
import sys

def load_manifest(path):
    if not os.path.exists(path):
        return None
    with open(path, 'r') as f:
        return json.load(f)

def save_manifest(path, data):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w') as f:
        json.dump(data, f, indent=2, sort_keys=True)
        f.write('\n')

def report_error(msg):
    print(f"ABI ERROR: {msg}", file=sys.stderr)

def check_compatibility(baseline, current, name, check_func):
    if baseline is None:
        print(f"[{name}] No baseline found. Run with --update to create it.")
        return False

    return check_func(baseline, current)
