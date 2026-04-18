import os
import subprocess

def elf_to_bin(input_path, output_path):
    """
    Transforms an ELF artifact into a raw binary artifact.
    """
    print(f"[Package] Transforming ELF to BIN: {input_path} -> {output_path}")

    # Check if the input exists
    if not os.path.isfile(input_path):
        raise FileNotFoundError(f"Input ELF file not found: {input_path}")

    # Create the output directory if it doesn't exist
    os.makedirs(os.path.dirname(output_path), exist_ok=True)

    # We will use llvm-objcopy or objcopy. llvm-objcopy is standard with llvm.
    objcopy_cmd = os.environ.get('OBJCOPY', 'llvm-objcopy')

    cmd = [objcopy_cmd, '-O', 'binary', input_path, output_path]
    try:
        subprocess.run(cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    except subprocess.CalledProcessError as e:
        print(f"Error running {objcopy_cmd}: {e.stderr.decode()}")
        # Fallback to standard objcopy
        if objcopy_cmd == 'llvm-objcopy':
            print("Falling back to standard objcopy...")
            objcopy_cmd = 'objcopy'
            cmd = [objcopy_cmd, '-O', 'binary', input_path, output_path]
            subprocess.run(cmd, check=True)
        else:
            raise e
