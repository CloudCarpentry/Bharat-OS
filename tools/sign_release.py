#!/usr/bin/env python3
import json
import os
import sys
import hashlib
import argparse
from pathlib import Path

# Optional: if cryptography is installed, we can sign with Ed25519
try:
    from cryptography.hazmat.primitives import serialization, hashes
    from cryptography.hazmat.primitives.asymmetric import ed25519
    HAS_CRYPTO = True
except ImportError:
    HAS_CRYPTO = False

def compute_sha256(filepath):
    sha256 = hashlib.sha256()
    with open(filepath, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            sha256.update(chunk)
    return sha256.hexdigest()

def sign_file_ed25519(private_key_path, filepath, sig_path):
    if not HAS_CRYPTO:
        print("Warning: cryptography package not found. Skipping Ed25519 signatures.")
        return False

    try:
        with open(private_key_path, "rb") as key_file:
            private_key = serialization.load_pem_private_key(
                key_file.read(),
                password=None
            )

        with open(filepath, "rb") as f:
            data = f.read()

        signature = private_key.sign(data)
        with open(sig_path, "wb") as f:
            f.write(signature)
        return True
    except Exception as e:
        print(f"Error signing {filepath}: {e}")
        return False

def generate_keypair(out_dir):
    if not HAS_CRYPTO:
        print("Error: cryptography package not found. Cannot generate keys.")
        sys.exit(1)

    private_key = ed25519.Ed25519PrivateKey.generate()
    public_key = private_key.public_key()

    priv_pem = private_key.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.PKCS8,
        encryption_algorithm=serialization.NoEncryption()
    )

    pub_pem = public_key.public_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PublicFormat.SubjectPublicKeyInfo
    )

    os.makedirs(out_dir, exist_ok=True)
    priv_path = os.path.join(out_dir, "release_key.pem")
    pub_path = os.path.join(out_dir, "release_key.pub")

    with open(priv_path, "wb") as f:
        f.write(priv_pem)

    with open(pub_path, "wb") as f:
        f.write(pub_pem)

    print(f"Generated Ed25519 keypair at {out_dir}")
    return priv_path

def create_release_layout(build_dir, dist_dir, manifest_path, private_key_path=None):
    with open(manifest_path, "r") as f:
        manifest = json.load(f)

    version = manifest["product"]["version"]
    release_dir = Path(dist_dir) / f"Bharat-OS-{version}"

    # Create structure
    release_dir.mkdir(parents=True, exist_ok=True)
    (release_dir / "kernel").mkdir(exist_ok=True)
    (release_dir / "services").mkdir(exist_ok=True)
    (release_dir / "sdk").mkdir(exist_ok=True)

    # 1. Copy and sign manifest
    manifest_dest = release_dir / "manifest.json"
    with open(manifest_dest, "w") as f:
        json.dump(manifest, f, indent=2)

    # 2. Process Kernel
    kernel_elf = Path(build_dir) / "kernel" / "kernel.elf"
    if kernel_elf.exists():
        kernel_dest = release_dir / "kernel" / "kernel.elf"
        kernel_dest.write_bytes(kernel_elf.read_bytes())

        # Digest
        digest = compute_sha256(kernel_dest)
        (release_dir / "kernel" / "kernel.elf.sha256").write_text(digest + "  kernel.elf\n")

        # Sign
        if private_key_path:
            sign_file_ed25519(private_key_path, kernel_dest, release_dir / "kernel" / "kernel.elf.sig")

    # 3. Process Services
    services_build = Path(build_dir) / "services"
    if services_build.exists():
        for svc_name in manifest.get("services", {}).keys():
            svc_elf = services_build / svc_name / f"{svc_name}.elf"
            if svc_elf.exists():
                svc_dest = release_dir / "services" / f"{svc_name}.elf"
                svc_dest.write_bytes(svc_elf.read_bytes())

                # Digest
                digest = compute_sha256(svc_dest)
                (release_dir / "services" / f"{svc_name}.elf.sha256").write_text(digest + f"  {svc_name}.elf\n")

                # Sign
                if private_key_path:
                    sign_file_ed25519(private_key_path, svc_dest, release_dir / "services" / f"{svc_name}.elf.sig")

    # Sign manifest last
    if private_key_path:
        sign_file_ed25519(private_key_path, manifest_dest, release_dir / "manifest.sig")

    print(f"Release layout created at {release_dir}")

def main():
    parser = argparse.ArgumentParser(description="Bharat-OS Release Tool")
    parser.add_argument("--build-dir", required=True, help="CMake build directory")
    parser.add_argument("--dist-dir", required=True, help="Output distribution directory")
    parser.add_argument("--manifest", required=True, help="Path to os-release.json")
    parser.add_argument("--key", help="Path to Ed25519 private key (PEM format)")
    parser.add_argument("--generate-key", help="Generate a new keypair in this directory instead of building release", type=str)

    args = parser.parse_args()

    if args.generate_key:
        generate_keypair(args.generate_key)
        return

    create_release_layout(args.build_dir, args.dist_dir, args.manifest, args.key)

if __name__ == "__main__":
    main()
