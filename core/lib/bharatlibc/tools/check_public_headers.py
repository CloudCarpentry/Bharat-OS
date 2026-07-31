#!/usr/bin/env python3
import os
import sys
import argparse
import subprocess
import tempfile

def parse_args():
    parser = argparse.ArgumentParser(description="Check public headers for self-containment.")
    parser.format_help()
    parser.add_argument("--build-dir", help="Path to the active build directory")
    return parser.parse_known_args()

def test_headers():
    args, unknown = parse_args()
    include_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "include"))

    # Check generated includes under build-dir if provided
    gen_dirs = []
    if args.build_dir:
        gen_dirs.append(os.path.abspath(os.path.join(args.build_dir, "generated", "include")))

    # Common/fallback generated directories
    gen_dirs.extend([
        os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "build", "generated", "include")),
        os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "..", "..", "build", "generated", "include")),
        os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "build-relocated", "generated", "include")),
    ])

    headers = []
    for root, _, files in os.walk(include_dir):
        for f in files:
            if f.endswith(".h"):
                rel_path = os.path.relpath(os.path.join(root, f), include_dir)
                headers.append(rel_path)

    compiler = "clang" if subprocess.run(["which", "clang"], capture_output=True).returncode == 0 else "gcc"

    print(f"Using compiler: {compiler}")
    print(f"Include directory: {include_dir}")
    if args.build_dir:
        print(f"Build directory supplied: {args.build_dir}")

    success = True
    for h in headers:
        if h.endswith(".in"):
            continue

        with tempfile.NamedTemporaryFile(suffix=".c", mode="w", delete=False) as temp_f:
            temp_f.write(f'#include <{h}>\n')
            temp_f.write('int main(void) { return 0; }\n')
            temp_name = temp_f.name

        try:
            cmd = [
                compiler,
                "-std=c11",
                "-Wall", "-Wextra", "-Werror", "-pedantic",
                "-ffreestanding", "-c", temp_name,
                "-I", include_dir,
                "-I", os.path.join(include_dir, "standard"),
                "-o", os.devnull
            ]
            for gd in gen_dirs:
                if os.path.isdir(gd):
                    cmd.extend(["-I", gd])

            res = subprocess.run(cmd, capture_output=True, text=True)
            if res.returncode != 0:
                print(f"[-] Header {h} failed self-containment check!")
                print(res.stderr)
                success = False
            else:
                print(f"[+] Header {h} is self-contained.")
        finally:
            if os.path.exists(temp_name):
                os.remove(temp_name)

    if not success:
        sys.exit(1)
    print("All public headers passed self-containment checks!")

if __name__ == "__main__":
    test_headers()
