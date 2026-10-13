import sys

def patch_file():
    with open('core/arch/arm/arm32/hal_pt_arm32.c', 'r') as f:
        content = f.read()

    search = """            // Section AP bits -> Small Page AP bits
            if (sect_flags & ARM32_PT_L1_SECT_AP0) {
                if ((sect_flags & ARM32_PT_L1_SECT_AP0) && !(sect_flags & (1<<15))) {
                    base_l2_flags |= ARM32_PT_L2_AP0; // Priv R/W
                } else if ((sect_flags & ARM32_PT_L1_SECT_AP0) && (sect_flags & (1<<15))) {
                    base_l2_flags |= ARM32_PT_L2_AP2 | ARM32_PT_L2_AP0; // Priv RO
                }
            } else {
                 base_l2_flags |= ARM32_PT_L2_AP0; // Default Priv R/W
            }"""

    replace = """            // Section AP bits -> Small Page AP bits
            // Section AP[0] is bit 10, AP[1] is bit 11, AP[2] is bit 15
            // L2 AP[0] is bit 4, AP[1] is bit 5, AP[2] is bit 9

            // Check original bits 10 and 11
            if (sect_flags & (1 << 10)) base_l2_flags |= ARM32_PT_L2_AP0;
            if (sect_flags & (1 << 11)) base_l2_flags |= ARM32_PT_L2_AP1;
            if (sect_flags & (1 << 15)) base_l2_flags |= ARM32_PT_L2_AP2;

            // Fallback for safe bootstrapping if entirely missing
            if (!(base_l2_flags & (ARM32_PT_L2_AP0 | ARM32_PT_L2_AP1 | ARM32_PT_L2_AP2))) {
                base_l2_flags |= ARM32_PT_L2_AP0; // Default Priv R/W
            }"""

    if search in content:
        content = content.replace(search, replace)
        with open('core/arch/arm/arm32/hal_pt_arm32.c', 'w') as f:
            f.write(content)
        print("Success")
    else:
        print("Search string not found")

patch_file()
