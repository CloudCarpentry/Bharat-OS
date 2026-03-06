import os
import argparse
import subprocess
import sys

def check_qemu_installed():
    try:
        subprocess.run(["qemu-system-x86_64", "--version"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return True
    except FileNotFoundError:
        return False

def generate_qemu_script(arch, memory_mb, cpu_cores, output_script):
    qemu_cmd = ""
    if arch == "x86_64":
        qemu_cmd = f"qemu-system-x86_64 -m {memory_mb} -smp {cpu_cores} -kernel ../build/kernel/kernel.elf -serial stdio"
    elif arch == "riscv":
        qemu_cmd = f"qemu-system-riscv64 -M virt -m {memory_mb} -smp {cpu_cores} -kernel ../build/kernel/kernel.elf -nographic"
    elif arch == "arm64":
        qemu_cmd = f"qemu-system-aarch64 -M virt -cpu cortex-a57 -m {memory_mb} -smp {cpu_cores} -kernel ../build/kernel/kernel.elf -nographic"
    else:
        print(f"Error: Unsupported architecture '{arch}'")
        sys.exit(1)
        
    # Write the execution script
    with open(output_script, "w") as f:
        # Check platform to generate bash or bat
        if os.name == "nt":
            f.write("@echo off\n")
            f.write(f"echo Starting Bharat-OS ({arch}) Emulation VM...\n")
            f.write(f"{qemu_cmd}\n")
        else:
            f.write("#!/bin/bash\n")
            f.write(f"echo \"Starting Bharat-OS ({arch}) Emulation VM...\"\n")
            f.write(f"{qemu_cmd}\n")
            
    # Make executable on Unix
    if os.name != "nt":
        os.chmod(output_script, 0o755)
        
    print(f"Successfully generated emulation script: {output_script}")
    print(f"Contains Command: {qemu_cmd}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate Bharat-OS Emulation VM Scripts")
    parser.add_argument("--arch", choices=["x86_64", "riscv", "arm64"], default="x86_64", help="Target architecture")
    parser.add_argument("--memory", type=int, default=1024, help="VM Memory in Megabytes")
    parser.add_argument("--cores", type=int, default=2, help="Number of CPU cores")
    
    args = parser.parse_args()
    
    if not check_qemu_installed():
        print("Warning: QEMU is not installed or not in the system PATH.")
        print("The generated scripts will not run until QEMU is installed.")
        
    script_ext = ".bat" if os.name == "nt" else ".sh"
    output_name = f"run_vm_{args.arch}{script_ext}"
    
    generate_qemu_script(args.arch, args.memory, args.cores, output_name)
