#!/usr/bin/env python3
import sys
import argparse

def main():
    parser = argparse.ArgumentParser(description="Assert that a serial log contains required markers.")
    parser.add_argument("logfile", help="Path to the serial log file")
    parser.add_argument("--markers", nargs='+', required=True, help="List of strings that must be present in the log")
    parser.add_argument("--fail-on", nargs='+', default=["[PANIC]", "Fatal boot self-test failure"], help="List of strings that must NOT be present in the log")
    args = parser.parse_args()

    try:
        with open(args.logfile, 'r', encoding='utf-8', errors='replace') as f:
            log_content = f.read()
    except Exception as e:
        print(f"Error reading {args.logfile}: {e}")
        sys.exit(1)

    failed = False

    # Check for forbidden markers
    for fail_marker in args.fail_on:
        if fail_marker in log_content:
            print(f"[FAIL] Forbidden marker found: '{fail_marker}'")
            failed = True

    # Check for required markers
    for marker in args.markers:
        if marker not in log_content:
            print(f"[FAIL] Missing required marker: '{marker}'")
            failed = True
        else:
            print(f"[PASS] Found marker: '{marker}'")

    if failed:
        print("\n--- BEGIN LOG TAIL ---")
        lines = log_content.splitlines()
        for line in lines[-80:]:
            print(line)
        print("--- END LOG TAIL ---")
        sys.exit(1)

    print("[SUCCESS] All markers asserted successfully.")
    sys.exit(0)

if __name__ == "__main__":
    main()
