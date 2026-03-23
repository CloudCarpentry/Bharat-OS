import json

with open("build_config.json", "r") as f:
    data = json.load(f)

new_builds = {
    "arm64_automobile_debug": {
      "preset": "arm64-automobile-debug",
      "arch": "arm64",
      "profile": "AUTOMOBILE",
      "personality": "NONE",
      "board": "virt",
      "gui": True,
      "run": False
    },
    "arm64_automobile_release": {
      "preset": "arm64-automobile-release",
      "arch": "arm64",
      "profile": "AUTOMOBILE",
      "personality": "NONE",
      "board": "virt",
      "gui": True,
      "run": False
    },
    "riscv64_ev_automobile_debug": {
      "preset": "riscv64-ev-automobile-debug",
      "arch": "riscv64",
      "profile": "EV_AUTOMOBILE",
      "personality": "NONE",
      "board": "virt",
      "gui": True,
      "run": False
    },
    "riscv64_ev_automobile_release": {
      "preset": "riscv64-ev-automobile-release",
      "arch": "riscv64",
      "profile": "EV_AUTOMOBILE",
      "personality": "NONE",
      "board": "virt",
      "gui": True,
      "run": False
    },
    "arm32_drone_debug": {
      "preset": "arm32-drone-debug",
      "arch": "arm32",
      "profile": "DRONE",
      "personality": "NONE",
      "board": "avh-corstone310",
      "gui": False,
      "run": False
    },
    "arm32_drone_release": {
      "preset": "arm32-drone-release",
      "arch": "arm32",
      "profile": "DRONE",
      "personality": "NONE",
      "board": "avh-corstone310",
      "gui": False,
      "run": False
    },
    "riscv32_robot_debug": {
      "preset": "riscv32-robot-debug",
      "arch": "riscv32",
      "profile": "ROBOT",
      "personality": "NONE",
      "board": "virt",
      "gui": False,
      "run": False
    },
    "riscv32_robot_release": {
      "preset": "riscv32-robot-release",
      "arch": "riscv32",
      "profile": "ROBOT",
      "personality": "NONE",
      "board": "virt",
      "gui": False,
      "run": False
    },
    "arm64_medical_debug": {
      "preset": "arm64-medical-debug",
      "arch": "arm64",
      "profile": "MEDICAL",
      "personality": "NONE",
      "board": "virt",
      "gui": True,
      "run": False
    },
    "arm64_medical_release": {
      "preset": "arm64-medical-release",
      "arch": "arm64",
      "profile": "MEDICAL",
      "personality": "NONE",
      "board": "virt",
      "gui": True,
      "run": False
    },
    "riscv64_medical_debug": {
      "preset": "riscv64-medical-debug",
      "arch": "riscv64",
      "profile": "MEDICAL",
      "personality": "NONE",
      "board": "virt",
      "gui": False,
      "run": False
    },
    "riscv64_medical_release": {
      "preset": "riscv64-medical-release",
      "arch": "riscv64",
      "profile": "MEDICAL",
      "personality": "NONE",
      "board": "virt",
      "gui": False,
      "run": False
    }
}

data["builds"].update(new_builds)

with open("build_config.json", "w") as f:
    json.dump(data, f, indent=2)
