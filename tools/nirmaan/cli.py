#!/usr/bin/env python3
import argparse
import sys
import importlib
import os
from pathlib import Path

# Add repo root to sys.path so we can import from tools.*
REPO_ROOT = Path(__file__).resolve().parent.parent.parent
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

def main():
    if hasattr(sys.stdout, "reconfigure"):
        try:
            sys.stdout.reconfigure(encoding="utf-8", errors="replace")
        except Exception:
            pass

    parser = argparse.ArgumentParser(
        description="Nirmaan - Bharat-OS Developer CLI",
        formatter_class=argparse.RawTextHelpFormatter,
    )

    subparsers = parser.add_subparsers(
        dest="command", required=True, help="Available commands"
    )

    # We will dynamically load the commands but for simplicity in help text we define them here.
    commands = {
        "build": "Build a target.",
        "run": "Run a target.",
        "test": "Test a target.",
        "debug": "Debug a target.",
        "package": "Package a target.",
        "doctor": "Check development environment.",
        "targets": "List available targets.",
    }

    for cmd, desc in commands.items():
        subparser = subparsers.add_parser(cmd, help=desc)
        # We use parse_known_args in the main cli and pass the rest to the command
        # So we don't strictly define arguments here except a help workaround if needed.

    # Actually just parse known args to delegate to subcommand
    args, unknown = parser.parse_known_args()

    try:
        command_module = importlib.import_module(f"tools.nirmaan.commands.{args.command}")
        # Call a run() or main() function in the module
        if hasattr(command_module, "run"):
            sys.exit(command_module.run(unknown))
        else:
            print(f"Error: Command module for '{args.command}' is missing a run() function.")
            sys.exit(1)
    except ImportError as e:
        print(f"Error loading command '{args.command}': {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
