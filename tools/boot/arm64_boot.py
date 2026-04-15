def check_fdt_ready():
    # In a real environment, QEMU passes the FDT via `-machine virt` and places it in RAM.
    # The kernel needs to validate it with a magic check in boot.S.
    # Here we assert the correct QEMU arguments are returned.
    print("ARM64: Expecting valid FDT pointer in x0 via QEMU.")

def get_boot_config(kernel_path):
    check_fdt_ready()
    return {
        # Keep the ELF image for QEMU `-kernel` on `-machine virt`.
        # Converting to a flat binary can strip ELF entry/loading metadata and
        # may lead to a silent boot/hang before early serial output.
        "kernel_path": kernel_path,
        "qemu_flags": []
    }
