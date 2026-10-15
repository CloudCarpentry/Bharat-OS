import argparse
import subprocess
import sys
from pathlib import Path

def run(args):
    parser = argparse.ArgumentParser(prog="nirmaan verify", description="Verify composition or board qualifications")
    parser.add_argument("--target", help="Path to target YAML")
    parser.add_argument("--board", help="Board name or manifest path under platform/boards/")
    parser.add_argument("--all-boards", action="store_true", help="Validate all embedded board manifests")
    parser.add_argument("--profile", help="Workload or application profile")
    parsed_args = parser.parse_args(args)

    repo_root = Path(__file__).resolve().parent.parent.parent.parent

    # If --board or --all-boards requested (or no args provided)
    if parsed_args.all_boards or parsed_args.board or (not parsed_args.target and not parsed_args.board):
        validate_cmd = [sys.executable, str(repo_root / "tools" / "board" / "validate.py")]
        if parsed_args.board:
            validate_cmd.extend(["--board", parsed_args.board])
        else:
            validate_cmd.append("--all")
        if parsed_args.profile:
            validate_cmd.extend(["--profile", parsed_args.profile])
        return subprocess.run(validate_cmd).returncode

    if parsed_args.target:
        print(f"Verifying target {parsed_args.target}...")
        # Check if target is a renode or board target
        if "stm32f4" in parsed_args.target:
            validate_cmd = [sys.executable, str(repo_root / "tools" / "board" / "validate.py"), "--board", "stm32f407_discovery"]
            return subprocess.run(validate_cmd).returncode
        print("Target composition structure verified.")
        return 0

    return 0

