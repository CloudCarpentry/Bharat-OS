from tools.package.transforms.elf_to_bin import elf_to_bin

class ArtifactRegistry:
    def __init__(self):
        self.transforms = {
            'elf_to_bin': elf_to_bin,
        }

    def get_transform(self, transform_name):
        """
        Returns the transform function based on the name.
        """
        if transform_name not in self.transforms:
            raise ValueError(f"Unknown transform: {transform_name}")
        return self.transforms[transform_name]

    def apply_transform(self, transform_name, input_path, output_path):
        """
        Applies a registered transform.
        """
        transform_func = self.get_transform(transform_name)
        transform_func(input_path, output_path)
