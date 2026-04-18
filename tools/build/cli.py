import argparse
import sys

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

    args = parser.parse_args()

    # Standard check
    if not getattr(args, "target_yaml", None) and not getattr(args, "target", None):
        parser.error("You must provide either --target-yaml or --target.")

    return args
