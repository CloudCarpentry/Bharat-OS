def run(args):
    print("TARGET             ARCH      MEMORY      UI          RUNNER")
    print("desktop-x86_64     x86_64    MMU_FULL    GUI         QEMU")
    print("desktop-arm64      arm64     MMU_FULL    GUI         QEMU")
    print("desktop-riscv64    riscv64   MMU_FULL    GUI         QEMU")
    print("drone-arm64        arm64     MMU_FULL    HEADLESS    QEMU")
    print("robot-riscv64      riscv64   MMU_FULL    GUI         QEMU")
    print("controller-arm32   arm32     MPU         HEADLESS    QEMU")
    print("stm32f4-renode     arm32     MPU         HEADLESS    Renode")
    return 0

