import json

with open("tools/boards/boards.json", "r") as f:
    data = json.load(f)

# Add shakti-e
data["boards"]["shakti-e"] = {
  "arch": "riscv64",
  "mmu_mode": "sv39",
  "granule": "4k",
  "pa_bits": 56,
  "has_iommu": False,
  "quirks": ["svpbmt_optional"],
  "machine": "virt",
  "cpu": "rv64",
  "qemu_system": "qemu-system-riscv64",
  "default_toolchain": "shakti-elf",
  "toolchains": {
    "gcc": "cmake/toolchains/riscv64-gcc.cmake",
    "llvm": "cmake/toolchains/riscv64-llvm.cmake",
    "shakti-elf": "cmake/toolchains/riscv64-shakti-elf.cmake"
  },
  "hardware_profile": "EDGE",
  "tier": "LINUX_LIKE",
  "boot_hw": "edge",
  "payload_format": "kernel-elf",
  "flash_script_dir": "tools/boards/qemu-virt-riscv64",
  "supports_flash": False,
  "memory_mb": 256,
  "smp": 1
}

# Add shakti-c
data["boards"]["shakti-c"] = {
  "arch": "riscv64",
  "mmu_mode": "sv39",
  "granule": "4k",
  "pa_bits": 56,
  "has_iommu": False,
  "quirks": ["svpbmt_optional"],
  "machine": "virt",
  "cpu": "rv64",
  "qemu_system": "qemu-system-riscv64",
  "default_toolchain": "shakti-elf",
  "toolchains": {
    "gcc": "cmake/toolchains/riscv64-gcc.cmake",
    "llvm": "cmake/toolchains/riscv64-llvm.cmake",
    "shakti-elf": "cmake/toolchains/riscv64-shakti-elf.cmake"
  },
  "hardware_profile": "EDGE",
  "tier": "LINUX_LIKE",
  "boot_hw": "edge",
  "payload_format": "kernel-elf",
  "flash_script_dir": "tools/boards/qemu-virt-riscv64",
  "supports_flash": False,
  "memory_mb": 1024,
  "smp": 4
}

with open("tools/boards/boards.json", "w") as f:
    json.dump(data, f, indent=2)
