#!/usr/bin/env python3
"""
Architecture Maturity Gate Checker for Bharat-OS.

Validates delivery/targets/arch_maturity.yaml to ensure that architectures
marked as production candidates meet the minimum required maturity levels.
"""

import sys
import argparse
import os

# Add common locations for PyYAML if not in path
sys.path.append('/usr/lib/python3/dist-packages')

# TODO: Cross-check delivery/targets/target_matrix.json once target metadata is stable.

REQUIRED_ARCHES = ["x86_64", "arm64", "riscv64", "arm32", "riscv32"]
REQUIRED_FIELDS = ["boot", "trap", "syscall", "mmu", "smp", "memory_model"]
METADATA_FIELDS = ["tier", "notes", "production_candidate"]
FORBIDDEN_FOR_PRODUCTION = ["stub", "unsupported", "scaffold"]
FORBIDDEN_MODEL_FOR_PRODUCTION = ["NONE", "PROT_NONE"]

def load_yaml(filepath):
    try:
        import yaml
        with open(filepath, 'r') as f:
            return yaml.safe_load(f)
    except ImportError:
        # Fallback for environments without PyYAML (very basic parser)
        # This is a fallback to ensure the script works in minimal environments.
        # It handles simple key-value YAML used in arch_maturity.yaml.
        data = {}
        current_arch = None
        with open(filepath, 'r') as f:
            for line in f:
                stripped = line.strip()
                if not stripped or stripped.startswith('#'):
                    continue
                if not line.startswith(' '):
                    # Architecture name
                    current_arch = stripped.split(':')[0].strip()
                    data[current_arch] = {}
                elif current_arch:
                    # Field (indented)
                    if ':' not in stripped:
                        continue
                    key, value = stripped.split(':', 1)
                    key = key.strip()
                    value = value.strip()
                    if value.lower() == 'true':
                        value = True
                    elif value.lower() == 'false':
                        value = False
                    data[current_arch][key] = value
        return data

def main():
    parser = argparse.ArgumentParser(description="Check architecture maturity levels.")
    parser.add_argument("yaml_file", help="Path to arch_maturity.yaml")
    args = parser.parse_args()

    if not os.path.exists(args.yaml_file):
        print(f"Error: {args.yaml_file} not found.")
        return 1

    data = load_yaml(args.yaml_file)
    if not data:
        print(f"Error: Failed to load or empty YAML: {args.yaml_file}")
        return 1

    errors = 0
    warnings = 0

    # 1. Check for required architectures
    for arch in REQUIRED_ARCHES:
        if arch not in data:
            print(f"ERROR: Missing required architecture: {arch}")
            errors += 1

    for arch, fields in data.items():
        if arch not in REQUIRED_ARCHES:
            # We only strictly care about our defined architectures for now,
            # but we can validate others if they are present.
            pass

        is_prod_candidate = fields.get("production_candidate", False)
        tier = fields.get("tier", "unknown")

        # 2. Check for minimum required fields presence
        for field in REQUIRED_FIELDS:
            if field not in fields:
                print(f"ERROR: {arch} is missing required field: {field}")
                errors += 1

        # 3. Memory model specific checks
        memory_model = fields.get("memory_model")
        if tier == "full" and memory_model != "MMU_FULL":
            print(f"ERROR: {arch} is tier 'full' but memory_model is '{memory_model}' (expected MMU_FULL)")
            errors += 1

        if is_prod_candidate and memory_model in FORBIDDEN_MODEL_FOR_PRODUCTION:
            print(f"ERROR: {arch} has memory_model='{memory_model}', but production_candidate=true.")
            errors += 1

        # 4. Generic maturity field check
        for field, value in fields.items():
            if field in METADATA_FIELDS or field == "memory_model":
                continue

            # 4a. Forbidden values check for production candidates
            if is_prod_candidate and value in FORBIDDEN_FOR_PRODUCTION:
                print(f"ERROR: {arch}.{field} is '{value}', but production_candidate=true.")
                errors += 1

            # 4b. Warning for 'partial' in production candidates
            if is_prod_candidate and value == "partial":
                print(f"WARNING: {arch}.{field} is partial; production_candidate=true means candidate only, not production-certified.")
                warnings += 1

    if errors > 0:
        print(f"\nFAILED: {errors} error(s), {warnings} warning(s).")
        return 1

    print(f"\nPASSED: {warnings} warning(s).")
    return 0

if __name__ == "__main__":
    sys.exit(main())
