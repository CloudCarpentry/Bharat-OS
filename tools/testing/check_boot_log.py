#!/usr/bin/env python3

import argparse
import re
import sys
import yaml
from pathlib import Path

# Default suspicious patterns for --strict mode
STRICT_SUSPICIOUS_PATTERNS = [
    r"\bPANIC\b",
    r"\bASSERT\b",
    r"\bFAULT\b",
    r"General Protection Fault",
    r"page fault",
    r"triple fault",
    r"Unhandled exception",
    r"kernel oops",
    r"segmentation fault",
]

class BootLogParser:
    def __init__(self, contract, target_name, strict=False):
        self.contract = contract
        self.target_name = target_name
        self.strict = strict
        self.target_config = self._get_target_config()
        self.required_markers = self._parse_markers(self.target_config.get("required", []))
        self.allowed_skip_markers = self._parse_markers(self.target_config.get("allowed_skip", []))
        self.forbidden_markers = self._parse_markers(self.target_config.get("forbidden", []))
        self.summary_config = self.target_config.get("summary", {"required": False})

        if self.strict:
            self._validate_strict()

    def _get_target_config(self):
        targets = self.contract.get("targets", {})
        if self.target_name not in targets:
            print(f"Error: Target '{self.target_name}' not found in contract.")
            sys.exit(2)

        config = targets[self.target_name]
        # Handle alias_of
        if "alias_of" in config:
            alias_target = config["alias_of"]
            if alias_target not in targets:
                print(f"Error: Alias target '{alias_target}' not found in contract.")
                sys.exit(2)
            config = targets[alias_target]

        return config

    def _parse_markers(self, marker_list):
        parsed = []
        for m in marker_list:
            if isinstance(m, str):
                parsed.append({"marker": m, "match": "substring", "type": "string"})
            elif isinstance(m, dict):
                if "marker" not in m and "pattern" not in m:
                    print(f"Error: Invalid marker definition: {m}")
                    sys.exit(2)

                marker_type = m.get("type", "string")
                match_type = m.get("match", "substring")

                if marker_type == "regex":
                    parsed.append({
                        "pattern": m.get("pattern"),
                        "match": "regex",
                        "type": "regex",
                        "description": m.get("description")
                    })
                else:
                    parsed.append({
                        "marker": m.get("marker"),
                        "match": match_type,
                        "type": "string",
                        "description": m.get("description")
                    })
            else:
                print(f"Error: Unsupported marker type: {type(m)}")
                sys.exit(2)
        return parsed

    def _validate_strict(self):
        # 3. Strict mode: Duplicate marker definitions fail.
        def check_duplicates(markers):
            seen = set()
            for m in markers:
                key = m.get("marker") or m.get("pattern")
                if key in seen:
                    print(f"Strict Error: Duplicate marker definition: {key}")
                    sys.exit(1)
                seen.add(key)

        check_duplicates(self.required_markers)
        check_duplicates(self.allowed_skip_markers)
        check_duplicates(self.forbidden_markers)

        # 3. Strict mode: Empty required list fails unless allow_empty_required=true
        if not self.required_markers and not self.target_config.get("allow_empty_required", False):
            print(f"Strict Error: Empty required list for target '{self.target_name}'.")
            sys.exit(1)

    def _matches(self, line, marker_def):
        if marker_def["type"] == "regex":
            return bool(re.search(marker_def["pattern"], line))

        marker = marker_def["marker"]
        match_type = marker_def["match"]

        if match_type == "substring":
            return marker in line
        elif match_type == "exact":
            return marker == line.strip()

        return False

    def parse(self, log_lines):
        results = {
            "required": {idx: False for idx in range(len(self.required_markers))},
            "forbidden_found": [],
            "skips_found": [],
            "suspicious_found": [],
            "summary_found": False,
            "summary_pass": False
        }

        pass_patterns = [self._parse_markers([p])[0] for p in self.summary_config.get("pass_patterns", [])]
        fail_patterns = [self._parse_markers([p])[0] for p in self.summary_config.get("fail_patterns", [])]

        for line in log_lines:
            line = line.strip()
            if not line:
                continue

            # Check required
            for idx, m in enumerate(self.required_markers):
                if not results["required"][idx] and self._matches(line, m):
                    results["required"][idx] = True

            # Check forbidden
            for m in self.forbidden_markers:
                if self._matches(line, m):
                    results["forbidden_found"].append({"line": line, "marker": m})

            # Check allowed skip
            for m in self.allowed_skip_markers:
                if self._matches(line, m):
                    results["skips_found"].append({"line": line, "marker": m})

            # Check summary
            for p in pass_patterns:
                if self._matches(line, p):
                    results["summary_found"] = True
                    results["summary_pass"] = True
            for p in fail_patterns:
                if self._matches(line, p):
                    results["summary_found"] = True
                    results["summary_pass"] = False

            # Strict: suspicious lines
            if self.strict:
                is_allowed_skip = any(self._matches(line, m) for m in self.allowed_skip_markers)
                if not is_allowed_skip:
                    for pattern in STRICT_SUSPICIOUS_PATTERNS:
                        if re.search(pattern, line):
                            # Ensure it's not one of the explicitly forbidden ones we already caught
                            if not any(self._matches(line, m) for m in self.forbidden_markers):
                                results["suspicious_found"].append(line)

        return results

    def report(self, results):
        success = True

        print(f"Target: {self.target_name}")

        # Required markers
        print("Required markers:")
        for idx, m in enumerate(self.required_markers):
            status = "[OK]" if results["required"][idx] else "[MISSING]"
            marker_text = m.get("marker") or m.get("pattern")
            print(f"  {status} {marker_text}")
            if not results["required"][idx]:
                success = False

        # Forbidden markers
        if results["forbidden_found"]:
            success = False
            print("Forbidden markers found:")
            for found in results["forbidden_found"]:
                print(f"  [FOUND] {found['line']} (matched: {found['marker'].get('marker') or found['marker'].get('pattern')})")

        # Suspicious markers (Strict mode)
        if self.strict and results["suspicious_found"]:
            success = False
            print("Suspicious markers found (Strict mode):")
            for line in results["suspicious_found"]:
                print(f"  [SUSPICIOUS] {line}")

        # Skips
        if results["skips_found"]:
            print("Allowed skips found:")
            for skip in results["skips_found"]:
                print(f"  [OK] {skip['line']}")

        # Summary
        if self.summary_config.get("required", False):
            if not results["summary_found"]:
                print("  [MISSING] Test summary")
                success = False
            else:
                status = "[OK]" if results["summary_pass"] else "[FAIL]"
                print(f"  {status} Test summary")
                if not results["summary_pass"]:
                    success = False
        elif results["summary_found"]:
            status = "[OK]" if results["summary_pass"] else "[FAIL]"
            print(f"  {status} Test summary")
            if not results["summary_pass"]:
                success = False

        # Timeout metadata
        timeout = self.target_config.get("timeout_seconds")
        if timeout:
            print(f"Timeout setting: {timeout}s (metadata)")

        if success:
            print("Result: PASS")
            return 0
        else:
            print("Result: FAIL")
            return 1

def main():
    parser = argparse.ArgumentParser(description="Parse QEMU/headless boot logs against a contract.")
    parser.add_argument("--log", required=True, help="Path to the log file")
    parser.add_argument("--contract", required=True, help="Path to the YAML contract file")
    parser.add_argument("--target", required=True, help="Target name in the contract")
    parser.add_argument("--summary", action="store_true", help="Print summary (handled by default in this implementation)")
    parser.add_argument("--strict", action="store_true", help="Enable strict mode")

    args = parser.parse_args()

    contract_path = Path(args.contract)
    if not contract_path.exists():
        print(f"Error: Contract file '{args.contract}' not found.")
        sys.exit(2)

    try:
        with open(contract_path, "r") as f:
            contract = yaml.safe_load(f)
    except Exception as e:
        print(f"Error: Failed to parse contract YAML: {e}")
        sys.exit(2)

    log_path = Path(args.log)
    if not log_path.exists():
        print(f"Error: Log file '{args.log}' not found.")
        sys.exit(2)

    try:
        with open(log_path, "r") as f:
            log_lines = f.readlines()
    except Exception as e:
        print(f"Error: Failed to read log file: {e}")
        sys.exit(2)

    parser_obj = BootLogParser(contract, args.target, args.strict)
    results = parser_obj.parse(log_lines)
    exit_code = parser_obj.report(results)
    sys.exit(exit_code)

if __name__ == "__main__":
    main()
