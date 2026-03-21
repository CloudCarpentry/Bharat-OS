#!/usr/bin/env python3
import json
import os
import sys
import datetime
import subprocess

def get_git_sha():
    try:
        sha = subprocess.check_output(["git", "rev-parse", "--short", "HEAD"], stderr=subprocess.STDOUT).decode("utf-8").strip()
        return sha
    except Exception:
        return "unknown"

def check_git_dirty():
    try:
        out = subprocess.check_output(["git", "status", "--porcelain"], stderr=subprocess.STDOUT).decode("utf-8").strip()
        return "true" if out else "false"
    except Exception:
        return "false"

def generate_headers(manifest_path, out_dir):
    with open(manifest_path, "r") as f:
        manifest = json.load(f)

    # Prepare directories
    inc_dir = os.path.join(out_dir, "include", "bharat")
    os.makedirs(inc_dir, exist_ok=True)

    git_sha = get_git_sha()
    is_dirty = check_git_dirty()
    build_epoch = int(datetime.datetime.now(datetime.timezone.utc).timestamp())
    build_time = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

    # Generate version.h
    version_h_path = os.path.join(inc_dir, "version.h")
    with open(version_h_path, "w") as f:
        f.write("/* GENERATED FILE - DO NOT EDIT */\n")
        f.write("#ifndef BHARAT_VERSION_H\n")
        f.write("#define BHARAT_VERSION_H\n\n")

        f.write(f'#define BHARAT_OS_NAME "{manifest["product"]["name"]}"\n')
        f.write(f'#define BHARAT_OS_VERSION "{manifest["product"]["version"]}"\n')
        f.write(f'#define BHARAT_OS_CHANNEL "{manifest["product"]["channel"]}"\n')
        f.write(f'#define BHARAT_OS_CODENAME "{manifest["product"]["codename"]}"\n\n')

        f.write(f'#define BHARAT_KERNEL_VERSION "{manifest["kernel"]["version"]}"\n')
        f.write(f'#define BHARAT_KERNEL_ABI {manifest["kernel"]["abi"]}\n')
        f.write(f'#define BHARAT_SYSCALL_ABI {manifest["kernel"]["syscall_abi"]}\n')
        f.write(f'#define BHARAT_CAP_ABI {manifest["kernel"]["cap_abi"]}\n\n')

        f.write("#endif /* BHARAT_VERSION_H */\n")

    # Generate buildinfo.h
    buildinfo_h_path = os.path.join(inc_dir, "buildinfo.h")
    with open(buildinfo_h_path, "w") as f:
        f.write("/* GENERATED FILE - DO NOT EDIT */\n")
        f.write("#ifndef BHARAT_BUILDINFO_H\n")
        f.write("#define BHARAT_BUILDINFO_H\n\n")

        f.write(f'#define BHARAT_GIT_SHA "{git_sha}"\n')
        f.write(f'#define BHARAT_GIT_DIRTY {1 if is_dirty == "true" else 0}\n')
        f.write(f'#define BHARAT_BUILD_EPOCH {build_epoch}ULL\n')
        f.write(f'#define BHARAT_BUILD_TIME_UTC "{build_time}"\n\n')

        f.write("#endif /* BHARAT_BUILDINFO_H */\n")

    # Generate version.json
    version_json_path = os.path.join(out_dir, "version.json")
    out_json = {
        "manifest": manifest,
        "build": {
            "git_sha": git_sha,
            "git_dirty": is_dirty == "true",
            "build_epoch": build_epoch,
            "build_time_utc": build_time
        }
    }
    with open(version_json_path, "w") as f:
        json.dump(out_json, f, indent=2)

    # Generate BharatVersion.cmake
    cmake_path = os.path.join(out_dir, "BharatVersion.cmake")
    with open(cmake_path, "w") as f:
        f.write("# GENERATED FILE - DO NOT EDIT\n")
        f.write(f'set(BHARAT_OS_VERSION "{manifest["product"]["version"]}")\n')
        f.write(f'set(BHARAT_OS_CHANNEL "{manifest["product"]["channel"]}")\n')
        f.write(f'set(BHARAT_KERNEL_VERSION "{manifest["kernel"]["version"]}")\n')
        f.write(f'set(BHARAT_KERNEL_ABI "{manifest["kernel"]["abi"]}")\n')
        f.write(f'set(BHARAT_GIT_SHA "{git_sha}")\n')
        f.write(f'set(BHARAT_GIT_DIRTY "{is_dirty}")\n')
        f.write(f'set(BHARAT_BUILD_EPOCH "{build_epoch}")\n')
        f.write(f'set(BHARAT_BUILD_TIME_UTC "{build_time}")\n\n')

        # Expose service metadata as CMake variables
        for svc_name, svc_info in manifest.get("services", {}).items():
            f.write(f'set(BHARAT_SVC_{svc_name.upper()}_VERSION "{svc_info.get("version", "0.0.0")}")\n')
            f.write(f'set(BHARAT_SVC_{svc_name.upper()}_IFACE "{svc_info.get("iface", 0)}")\n')
            f.write(f'set(BHARAT_SVC_{svc_name.upper()}_CHANNEL "{svc_info.get("channel", "unknown")}")\n')

        for drv_name, drv_info in manifest.get("drivers", {}).items():
            f.write(f'set(BHARAT_DRV_{drv_name.upper()}_VERSION "{drv_info.get("version", "0.0.0")}")\n')
            f.write(f'set(BHARAT_DRV_{drv_name.upper()}_IFACE "{drv_info.get("iface", 0)}")\n')
            f.write(f'set(BHARAT_DRV_{drv_name.upper()}_CHANNEL "{drv_info.get("channel", "unknown")}")\n')

        for subsys_name, subsys_info in manifest.get("subsystems", {}).items():
            f.write(f'set(BHARAT_SUBSYS_{subsys_name.upper()}_VERSION "{subsys_info.get("version", "0.0.0")}")\n')
            f.write(f'set(BHARAT_SUBSYS_{subsys_name.upper()}_IFACE "{subsys_info.get("iface", 0)}")\n')
            f.write(f'set(BHARAT_SUBSYS_{subsys_name.upper()}_CHANNEL "{subsys_info.get("channel", "unknown")}")\n')

        # Provide a helper function to register components
        f.write("""
function(bharat_register_component)
    set(options)
    set(oneValueArgs TARGET KIND VERSION IFACE_VERSION CHANNEL)
    set(multiValueArgs)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_TARGET OR NOT ARG_KIND OR NOT ARG_VERSION OR NOT DEFINED ARG_IFACE_VERSION OR NOT ARG_CHANNEL)
        message(FATAL_ERROR "bharat_register_component missing required arguments")
    endif()

    target_compile_definitions(${ARG_TARGET} PRIVATE
        BHARAT_COMPONENT_NAME=\\"${ARG_TARGET}\\"
        BHARAT_COMPONENT_KIND=\\"${ARG_KIND}\\"
        BHARAT_COMPONENT_VERSION=\\"${ARG_VERSION}\\"
        BHARAT_COMPONENT_IFACE=${ARG_IFACE_VERSION}
        BHARAT_COMPONENT_CHANNEL=\\"${ARG_CHANNEL}\\"
    )

    # Future: generate tiny metadata object file linked into binary or emit <target>.buildinfo.json
endfunction()
""")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: gen_manifest.py <manifest_path> <out_dir>")
        sys.exit(1)

    generate_headers(sys.argv[1], sys.argv[2])
