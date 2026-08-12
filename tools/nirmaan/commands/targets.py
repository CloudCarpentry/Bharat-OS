def run(args):
    print("TARGET             ARCH      MEMORY      UI")
    print("desktop-x86_64     x86_64    MMU_FULL    GUI")
    print("desktop-arm64      arm64     MMU_FULL    GUI")
    print("desktop-riscv64    riscv64   MMU_FULL    GUI")
    print("drone-arm64        arm64     MMU_FULL    HEADLESS")
    print("robot-riscv64      riscv64   MMU_FULL    GUI")
    print("controller-arm32   arm32     MPU         HEADLESS")
    return 0
