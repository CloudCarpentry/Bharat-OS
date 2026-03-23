import json

with open("CMakePresets.json", "r") as f:
    data = json.load(f)

new_configure_presets = [
    {
      "name": "arm64-automobile-debug",
      "displayName": "Cross ARM64 Automobile (Debug)",
      "description": "Cross-compile ARM64 Automobile profile on Linux",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/arm64-automobile-debug",
      "toolchainFile": "${sourceDir}/cmake/toolchains/aarch64-elf-clang.cmake",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "BHARAT_DEVICE_PROFILE": "AUTOMOBILE",
        "BHARAT_TARGET_BOARD": "virt",
        "BHARAT_BOOT_GUI": "ON"
      }
    },
    {
      "name": "arm64-automobile-release",
      "displayName": "Cross ARM64 Automobile (Release)",
      "description": "Cross-compile ARM64 Automobile profile on Linux",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/arm64-automobile-release",
      "toolchainFile": "${sourceDir}/cmake/toolchains/aarch64-elf-clang.cmake",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "BHARAT_DEVICE_PROFILE": "AUTOMOBILE",
        "BHARAT_TARGET_BOARD": "virt",
        "BHARAT_BOOT_GUI": "ON"
      }
    },
    {
      "name": "riscv64-ev-automobile-debug",
      "displayName": "Cross RISC-V64 EV Automobile (Debug)",
      "description": "Cross-compile RISC-V64 EV Automobile profile on Linux",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/riscv64-ev-automobile-debug",
      "toolchainFile": "${sourceDir}/cmake/toolchains/riscv64-elf-clang.cmake",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "BHARAT_DEVICE_PROFILE": "EV_AUTOMOBILE",
        "BHARAT_TARGET_BOARD": "virt",
        "BHARAT_BOOT_GUI": "ON"
      }
    },
    {
      "name": "riscv64-ev-automobile-release",
      "displayName": "Cross RISC-V64 EV Automobile (Release)",
      "description": "Cross-compile RISC-V64 EV Automobile profile on Linux",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/riscv64-ev-automobile-release",
      "toolchainFile": "${sourceDir}/cmake/toolchains/riscv64-elf-clang.cmake",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "BHARAT_DEVICE_PROFILE": "EV_AUTOMOBILE",
        "BHARAT_TARGET_BOARD": "virt",
        "BHARAT_BOOT_GUI": "ON"
      }
    },
    {
      "name": "arm32-drone-debug",
      "displayName": "Cross ARM32 Drone (Debug)",
      "description": "Cross-compile ARM32 Drone profile on Linux",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/arm32-drone-debug",
      "toolchainFile": "${sourceDir}/cmake/toolchains/arm32-elf.cmake",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "BHARAT_DEVICE_PROFILE": "DRONE",
        "BHARAT_TARGET_BOARD": "avh-corstone310",
        "BHARAT_BOOT_GUI": "OFF"
      }
    },
    {
      "name": "arm32-drone-release",
      "displayName": "Cross ARM32 Drone (Release)",
      "description": "Cross-compile ARM32 Drone profile on Linux",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/arm32-drone-release",
      "toolchainFile": "${sourceDir}/cmake/toolchains/arm32-elf.cmake",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "BHARAT_DEVICE_PROFILE": "DRONE",
        "BHARAT_TARGET_BOARD": "avh-corstone310",
        "BHARAT_BOOT_GUI": "OFF"
      }
    },
    {
      "name": "riscv32-robot-debug",
      "displayName": "Cross RISC-V32 Robot (Debug)",
      "description": "Cross-compile RISC-V32 Robot profile on Linux",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/riscv32-robot-debug",
      "toolchainFile": "${sourceDir}/cmake/toolchains/riscv32-elf.cmake",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "BHARAT_DEVICE_PROFILE": "ROBOT",
        "BHARAT_TARGET_BOARD": "virt",
        "BHARAT_BOOT_GUI": "OFF"
      }
    },
    {
      "name": "riscv32-robot-release",
      "displayName": "Cross RISC-V32 Robot (Release)",
      "description": "Cross-compile RISC-V32 Robot profile on Linux",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/riscv32-robot-release",
      "toolchainFile": "${sourceDir}/cmake/toolchains/riscv32-elf.cmake",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "BHARAT_DEVICE_PROFILE": "ROBOT",
        "BHARAT_TARGET_BOARD": "virt",
        "BHARAT_BOOT_GUI": "OFF"
      }
    },
    {
      "name": "arm64-medical-debug",
      "displayName": "Cross ARM64 Medical (Debug)",
      "description": "Cross-compile ARM64 Medical profile on Linux",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/arm64-medical-debug",
      "toolchainFile": "${sourceDir}/cmake/toolchains/aarch64-elf-clang.cmake",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "BHARAT_DEVICE_PROFILE": "MEDICAL",
        "BHARAT_TARGET_BOARD": "virt",
        "BHARAT_BOOT_GUI": "ON"
      }
    },
    {
      "name": "arm64-medical-release",
      "displayName": "Cross ARM64 Medical (Release)",
      "description": "Cross-compile ARM64 Medical profile on Linux",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/arm64-medical-release",
      "toolchainFile": "${sourceDir}/cmake/toolchains/aarch64-elf-clang.cmake",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "BHARAT_DEVICE_PROFILE": "MEDICAL",
        "BHARAT_TARGET_BOARD": "virt",
        "BHARAT_BOOT_GUI": "ON"
      }
    },
    {
      "name": "riscv64-medical-debug",
      "displayName": "Cross RISC-V64 Medical (Debug)",
      "description": "Cross-compile RISC-V64 Medical profile on Linux",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/riscv64-medical-debug",
      "toolchainFile": "${sourceDir}/cmake/toolchains/riscv64-elf-clang.cmake",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "BHARAT_DEVICE_PROFILE": "MEDICAL",
        "BHARAT_TARGET_BOARD": "virt",
        "BHARAT_BOOT_GUI": "OFF"
      }
    },
    {
      "name": "riscv64-medical-release",
      "displayName": "Cross RISC-V64 Medical (Release)",
      "description": "Cross-compile RISC-V64 Medical profile on Linux",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/riscv64-medical-release",
      "toolchainFile": "${sourceDir}/cmake/toolchains/riscv64-elf-clang.cmake",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "BHARAT_DEVICE_PROFILE": "MEDICAL",
        "BHARAT_TARGET_BOARD": "virt",
        "BHARAT_BOOT_GUI": "OFF"
      }
    }
]

new_build_presets = [
    {"name": p["name"], "configurePreset": p["name"]}
    for p in new_configure_presets
]

new_test_presets = [
    {
        "name": p["name"],
        "configurePreset": p["name"],
        "output": {"outputOnFailure": True},
        "execution": {"noTestsAction": "error", "stopOnFailure": True}
    }
    for p in new_configure_presets
]

data["configurePresets"].extend(new_configure_presets)
data["buildPresets"].extend(new_build_presets)
data["testPresets"].extend(new_test_presets)

with open("CMakePresets.json", "w") as f:
    json.dump(data, f, indent=2)
