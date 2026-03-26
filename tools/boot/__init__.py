import importlib

def get_bootloader(arch):
    if arch == "x86_64":
        return importlib.import_module("boot.x86_boot").get_boot_config
    elif arch == "arm64":
        return importlib.import_module("boot.arm64_boot").get_boot_config
    elif arch == "riscv64":
        return importlib.import_module("boot.riscv_boot").get_boot_config

    # Return a default fallback
    def default_config(kernel_path):
        return {
            "kernel_path": kernel_path,
            "qemu_flags": []
        }
    return default_config
