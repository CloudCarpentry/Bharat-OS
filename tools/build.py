import argparse
import json
import os
import subprocess
import sys
import boot

from targets.loader import get_target
from targets.validate import validate_target

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from tools.build.preset_resolver import resolve_target

from tools.package.packager import package
from tools.run import runner_qemu

def load_manifest():
    manifest_path = 'build_config.json'
    try:
        with open(manifest_path, 'r') as f:
            return json.load(f)
    except Exception as e:
        print(f"Error loading {manifest_path}: {e}")
        sys.exit(1)

def print_list(manifest):
    print("Available builds (legacy):")
    if 'builds' in manifest:
        for name, cfg in manifest['builds'].items():
            print(f"  {name}: {cfg.get('preset')} ({cfg.get('arch')} / {cfg.get('profile')})")
    else:
        print("  No builds configured.")

    print("\nAvailable targets (YAML):")
    qemu_dir = 'tools/targets/qemu'
    if os.path.exists(qemu_dir):
        for f in os.listdir(qemu_dir):
            if f.endswith('.yaml'):
                print(f"  {f[:-5]}")

def check_command(cmd):
    try:
        subprocess.run([cmd, '--version'], capture_output=True, check=True)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False

def doctor():
    print("Checking dependencies...")
    deps = {
        'cmake': check_command('cmake'),
        'ninja': check_command('ninja'),
        'clang': check_command('clang'),
        'lld': check_command('lld')
    }
    for tool, found in deps.items():
        print(f"  {tool}: {'Found' if found else 'Missing'}")

def normalize_csv(val):
    if not val:
        return ""
    return str(val).split(',')[0].strip().upper()

def run_command(cmd, cwd=None):
    print(f"Running: {' '.join(cmd)}")
    result = subprocess.run(cmd, cwd=cwd)
    if result.returncode != 0:
        print(f"Command failed with exit code {result.returncode}")
        sys.exit(result.returncode)

def do_configure(target_spec):
    build_spec = target_spec.get('build', {})
    preset = build_spec.get('cmake_preset')
    defs = build_spec.get('cmake_defs', {})

    cmd = ['cmake', f'--preset={preset}']
    for k, v in defs.items():
        cmd.append(f'-D{k}={v}')

    run_command(cmd)

def do_build(target_spec):
    build_spec = target_spec.get('build', {})
    preset = build_spec.get('cmake_preset')
    run_command(['cmake', '--build', f'--preset={preset}'])

def do_test(target_spec):
    preset = target_spec.get('build', {}).get('cmake_preset')
    build_dir = os.path.join('build', preset)
    if not os.path.exists(build_dir):
         print(f"Build directory {build_dir} not found. Please build first.")
         sys.exit(1)
    run_command(['ctest', '--test-dir', build_dir])

def do_clean(target_spec):
    preset = target_spec.get('build', {}).get('cmake_preset')
    build_dir = os.path.join('build', preset)
    if os.path.exists(build_dir):
        run_command(['cmake', '--build', f'--preset={preset}', '--target', 'clean'])

def do_package(target_spec):
    if target_spec.get('_source') == 'yaml':
        preset = target_spec.get('build', {}).get('cmake_preset')
        build_dir = os.path.join('build', preset)
        return package(target_spec, build_dir)
    return None

def do_run(target_spec, packaged_output=None, cpus=1):
    if target_spec.get('_source') == 'yaml':
        run_manifest_path = packaged_output.get('run_manifest')
        if not run_manifest_path or not os.path.exists(run_manifest_path):
            print("Error: Run manifest not found. Did packaging succeed?")
            sys.exit(1)

        with open(run_manifest_path, 'r') as f:
            run_manifest = json.load(f)

        runner_type = run_manifest.get('runner_type', 'qemu')
        if runner_type == 'qemu':
            runner_qemu.run(run_manifest)
        else:
            print(f"Error: Unknown runner type '{runner_type}'")
            sys.exit(1)
    else:
        # Legacy run path
        print("[Run] Falling back to legacy runner path for build_config.json target...")
        legacy_cfg = target_spec.get('_legacy_cfg')
        arch = legacy_cfg.get('arch')
        preset = legacy_cfg.get('preset')
        board = legacy_cfg.get('board')
        build_dir = os.path.join('build', preset)

        kernel_path = os.path.join(build_dir, 'kernel', 'kernel.elf')
        boot_config = boot.get_bootloader(arch)(kernel_path)
        run_kernel = boot_config.get('kernel_path', kernel_path)

        RUN_MATRIX = {
            ("x86_64", "qemu-virt"): {"runner": "qemu-system-x86_64", "machine": "q35", "smp_supported": True},
            ("arm64", "virt"): {"runner": "qemu-system-aarch64", "machine": "virt", "cpu": "cortex-a57", "smp_supported": True},
            ("riscv64", "virt"): {"runner": "qemu-system-riscv64", "machine": "virt", "smp_supported": True},
            ("arm32", "avh-corstone310"): {"runner": "VHT_Corstone_SSE-310", "smp_supported": False},
            ("arm32", "virt"): {"runner": "qemu-system-arm", "machine": "virt", "cpu": "cortex-a15", "smp_supported": True},
            ("riscv32", "virt"): {"runner": "qemu-system-riscv32", "machine": "virt", "smp_supported": True},
        }

        run_config = RUN_MATRIX.get((arch, board))
        if not run_config:
            print(f"Error: No legacy run configuration for {arch} / {board}")
            sys.exit(1)

        runner = run_config.get('runner')
        cmd = [runner]
        if runner != 'VHT_Corstone_SSE-310':
            cmd.extend(['-kernel', run_kernel, '-m', '512'])
            if 'machine' in run_config:
                cmd.extend(['-machine', run_config['machine']])
            if 'cpu' in run_config:
                cmd.extend(['-cpu', run_config['cpu']])
            if arch == 'arm64':
                cmd.extend(['-bios', 'none'])

            if run_config.get("smp_supported", False) and cpus > 1:
                cmd.extend(['-smp', str(cpus)])

        display_cfg = legacy_cfg.get("display", {})
        gui_enabled = display_cfg.get("enabled", legacy_cfg.get("gui", False))
        if not gui_enabled:
            cmd.append('-nographic')

        run_command(cmd)

def main():
    parser = argparse.ArgumentParser(
        description='Bharat-OS Build Wrapper (Manifest-driven Pipeline)\n\n'
                    'Examples:\n'
                    '  python tools/build.py arm64_desktop_headless --run\n'
                    '  python tools/build.py tools/targets/qemu/x86_64_desktop_headless.yaml --build --run\n'
                    '  python tools/build.py default_dev\n'
                    '  python tools/build.py --list',
        formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument('build_name', nargs='?', default='default_dev', help='Name of the build target or path to YAML')
    parser.add_argument('--list', action='store_true', help='List available builds')
    parser.add_argument('--doctor', action='store_true', help='Check environment')
    parser.add_argument('--configure', action='store_true', help='Only configure the project')
    parser.add_argument('--build', action='store_true', help='Build the project')
    parser.add_argument('--run', action='store_true', help='Run the project in emulator')
    parser.add_argument('--clean', action='store_true', help='Clean the build directory')
    parser.add_argument('--test', action='store_true', help='Run tests (host)')
    # Keep some legacy flags for compatibility wrappers
    parser.add_argument('--run-tests', action='store_true', help='Run tests in emulator (legacy)')
    parser.add_argument('--dual-serial', action='store_true', help='Legacy dual serial flag')
    parser.add_argument('--cpus', type=int, default=1, help='Legacy CPUs flag')
    parser.add_argument('--matrix', action='store_true', help='Legacy matrix flag')

    args = parser.parse_args()

    if args.doctor:
        doctor()
        return

    if args.list:
        manifest = load_manifest()
        print_list(manifest)
        return

    target_spec = resolve_target(args.build_name)
    if not target_spec:
        print(f"Error: Target '{args.build_name}' not found as YAML or in build_config.json.")
        sys.exit(1)

    # Default action if no specific action provided
    if not (args.configure or args.build or args.run or args.clean or args.test):
        args.configure = True
        args.build = True
        if target_spec.get('_source') == 'legacy' and target_spec.get('_legacy_cfg', {}).get('run', False):
            args.run = True
        elif target_spec.get('_source') == 'yaml' and target_spec.get('run'):
            # Only run if a run block exists
            pass # Keep default to build only

    if args.clean:
        do_clean(target_spec)

    if args.configure:
        do_configure(target_spec)

    if args.build:
        do_build(target_spec)

    if args.test:
        do_test(target_spec)

    if args.run or args.run_tests:
        if target_spec.get('_source') == 'yaml':
            packaged_output = do_package(target_spec)
        else:
            packaged_output = None
        do_run(target_spec, packaged_output, args.cpus)

if __name__ == '__main__':
    main()
