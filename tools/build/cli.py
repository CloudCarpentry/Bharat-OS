import argparse
import sys


def parse_args():
    parser = argparse.ArgumentParser(
        description="Bharat-OS Target Delivery Pipeline",
        formatter_class=argparse.RawTextHelpFormatter,
    )

    subparsers = parser.add_subparsers(
        dest="command", required=True, help="Subcommands"
    )

    for cmd in ["configure", "build", "package", "run", "flash", "debug", "all"]:
        subparser = subparsers.add_parser(cmd, help=f"Run the {cmd} stage.")
        subparser.add_argument(
            "--target-yaml", type=str, help="Path to the target YAML spec."
        )
        subparser.add_argument(
            "--target", type=str, help="Name of the legacy target configuration."
        )

        if cmd == "flash":
            subparser.add_argument(
                "--dry-run", action="store_true", help="Perform a dry run for flashing."
            )

    # peek at the first argument to see if it's a valid command
    valid_commands = ["configure", "build", "package", "run", "flash", "debug", "all"]

    # If no arguments or first arg is a flag or a valid command, use standard parsing
    if (
        len(sys.argv) > 1
        and not sys.argv[1].startswith("-")
        and sys.argv[1] not in valid_commands
    ):
        # Legacy positional invocation detected
        print(
            "[Warning] You are using the legacy positional CLI. This will be removed in the future.",
            file=sys.stderr,
        )

        normalized_args = list(sys.argv[2:])
        normalized_flags = set(normalized_args)
        filtered_args = []

        # Be tolerant of users typing "-- run" / "-- all" in legacy mode.
        i = 0
        while i < len(normalized_args):
            arg = normalized_args[i]
            if arg == "--" and i + 1 < len(normalized_args):
                maybe_flag = f"--{normalized_args[i + 1]}"
                if maybe_flag in ("--run", "--build", "--all"):
                    normalized_flags.add(maybe_flag)
                    i += 2
                    continue
            filtered_args.append(arg)
            i += 1

        # We'll create a dummy 'all' or 'build' command internally
        mock_argv = ["all" if "--run" in normalized_flags or "--all" in normalized_flags else "build"]
        # Add target spec
        if sys.argv[1].endswith(".yaml"):
            mock_argv.extend(["--target-yaml", sys.argv[1]])
        else:
            mock_argv.extend(["--target", sys.argv[1]])

        # Keep other flags
        for arg in filtered_args:
            if arg not in ("--run", "--build", "--all"):
                mock_argv.append(arg)

        # Replace sys.argv for the parser
        sys.argv = [sys.argv[0]] + mock_argv

    args = parser.parse_args()

    # Standard check for subcommand invocation
    if not getattr(args, "target_yaml", None) and not getattr(args, "target", None):
        parser.error(
            "You must provide either --target-yaml or --target for the chosen command."
        )

    return args
