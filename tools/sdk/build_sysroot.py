#!/usr/bin/env python3
import os
import shutil
import subprocess

def build_sysroot():
    build_dir = "build/sdk"
    sysroot_dir = "sysroot"

    if os.path.exists(build_dir):
        shutil.rmtree(build_dir)
    if os.path.exists(sysroot_dir):
        shutil.rmtree(sysroot_dir)

    os.makedirs(build_dir)

    subprocess.run(["cmake", "../..", "-DCMAKE_INSTALL_PREFIX=../../sysroot"], cwd=build_dir, check=True)
    subprocess.run(["make", "install"], cwd=build_dir, check=True)

    print(f"SDK sysroot built at {os.path.abspath(sysroot_dir)}")

if __name__ == "__main__":
    build_sysroot()
