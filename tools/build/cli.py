import argparse
import sys
from pathlib import Path


def parse_args():
    parser = argparse.ArgumentParser(
        description="Bharat-OS Target Delivery Pipeline",
        formatter_class=argparse.RawTextHelpFormatter,
    )

    subparsers = parser.add_subparsers(dest="command", required=True, help="Subcommands")

    # Subcommands
    for cmd in ["configure", "build", "package", "run", "flash", "debug", "all"]:
        subparser = subparsers.add_parser(cmd, help=f"Run the {cmd} stage.")
        subparser.add_argument("--target-yaml", type=str, help="Path to the target YAML spec.")
        subparser.add_argument("--target", type=str, help="Name of the legacy target configuration.")

        if cmd == "flash":
            subparser.add_argument("--dry-run", action="store_true", help="Perform a dry run for flashing.")

    # Legacy fallback handler (for `python tools/build.py <target_name> --run`)
    parser.add_argument("legacy_positional", nargs="?", help=argparse.SUPPRESS)
    parser.add_argument("--run", action="store_true", help=argparse.SUPPRESS)
    parser.add_argument("--build", action="store_true", help=argparse.SUPPRESS)

    # Let's peek at argv before parsing to catch legacy usage early
    args, unknown = parser.parse_known_args()

    # Check for legacy invocation: python tools/build.py <target> [--build] [--run]
    if args.command is None and args.legacy_positional:
        print("[Warning] You are using the legacy positional CLI. This will be removed in the future.", file=sys.stderr)
        cmd_action = "all" if args.run else "build"

        args.command = cmd_action

        # Check if positional is a YAML path
        if args.legacy_positional.endswith(".yaml"):
            args.target_yaml = args.legacy_positional
            args.target = None
        else:
            args.target = args.legacy_positional
            args.target_yaml = None

        args.dry_run = False
        return args

    # Standard check
    if not getattr(args, "target_yaml", None) and not getattr(args, "target", None):
        parser.error("You must provide either --target-yaml or --target.")

    return args
