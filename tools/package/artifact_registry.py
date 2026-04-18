from tools.package.transforms.elf_to_bin import apply_elf_to_bin
from tools.package.transforms.multiboot_elf_fix import apply_multiboot_elf_fix


class ArtifactRegistry:
    def __init__(self):
        self.transforms = {
            "elf_to_bin": apply_elf_to_bin,
            "multiboot_elf_fix": apply_multiboot_elf_fix,
        }

    def get_transform(self, transform_name):
        if transform_name not in self.transforms:
            raise ValueError(f"Unknown transform: {transform_name}")
        return self.transforms[transform_name]

    def apply_transform(self, transform_name, input_path, output_path):
        transform_func = self.get_transform(transform_name)
        return transform_func(input_path, output_path)
