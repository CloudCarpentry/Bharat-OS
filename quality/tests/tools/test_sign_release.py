import json
import os
from pathlib import Path
import pytest
import hashlib

from tools.sign_release import (
    compute_sha256,
    sign_file_ed25519,
    generate_keypair,
    create_release_layout,
    HAS_CRYPTO
)

def test_compute_sha256(tmp_path: Path):
    test_file = tmp_path / "test.txt"
    content = b"hello world"
    test_file.write_bytes(content)

    expected_hash = hashlib.sha256(content).hexdigest()
    assert compute_sha256(test_file) == expected_hash

@pytest.mark.skipif(not HAS_CRYPTO, reason="cryptography package not installed")
def test_generate_keypair(tmp_path: Path):
    out_dir = tmp_path / "keys"
    priv_path = generate_keypair(str(out_dir))

    assert priv_path == str(out_dir / "release_key.pem")
    assert (out_dir / "release_key.pem").exists()
    assert (out_dir / "release_key.pub").exists()

@pytest.mark.skipif(not HAS_CRYPTO, reason="cryptography package not installed")
def test_sign_file_ed25519(tmp_path: Path):
    out_dir = tmp_path / "keys"
    priv_path = generate_keypair(str(out_dir))

    test_file = tmp_path / "data.bin"
    test_file.write_bytes(b"some data to sign")
    sig_path = tmp_path / "data.sig"

    success = sign_file_ed25519(priv_path, test_file, sig_path)
    assert success is True
    assert sig_path.exists()
    assert sig_path.stat().st_size > 0

def test_sign_file_ed25519_missing_key(tmp_path: Path):
    if not HAS_CRYPTO:
        pytest.skip("cryptography package not installed")

    test_file = tmp_path / "data.bin"
    test_file.write_bytes(b"some data to sign")
    sig_path = tmp_path / "data.sig"

    # Passing a non-existent key path should fail gracefully
    success = sign_file_ed25519(str(tmp_path / "non_existent_key.pem"), test_file, sig_path)
    assert success is False
    assert not sig_path.exists()

def setup_release_env(tmp_path: Path):
    build_dir = tmp_path / "build"
    dist_dir = tmp_path / "dist"

    # Create build structure
    kernel_dir = build_dir / "kernel"
    kernel_dir.mkdir(parents=True)
    kernel_file = kernel_dir / "kernel.elf"
    kernel_file.write_bytes(b"fake kernel data")

    services_dir = build_dir / "services" / "test_svc"
    services_dir.mkdir(parents=True)
    svc_file = services_dir / "test_svc.elf"
    svc_file.write_bytes(b"fake service data")

    # Create manifest
    manifest_path = tmp_path / "os-release.json"
    manifest_data = {
        "product": {"version": "1.0.0"},
        "services": {"test_svc": {}}
    }
    with open(manifest_path, "w") as f:
        json.dump(manifest_data, f)

    return build_dir, dist_dir, manifest_path

def test_create_release_layout_without_key(tmp_path: Path):
    build_dir, dist_dir, manifest_path = setup_release_env(tmp_path)

    create_release_layout(str(build_dir), str(dist_dir), str(manifest_path))

    release_dir = dist_dir / "Bharat-OS-1.0.0"

    # Verify manifest
    assert (release_dir / "manifest.json").exists()
    assert not (release_dir / "manifest.sig").exists()

    # Verify kernel
    assert (release_dir / "kernel" / "kernel.elf").exists()
    assert (release_dir / "kernel" / "kernel.elf.sha256").exists()
    assert not (release_dir / "kernel" / "kernel.elf.sig").exists()

    # Verify services
    assert (release_dir / "services" / "test_svc.elf").exists()
    assert (release_dir / "services" / "test_svc.elf.sha256").exists()
    assert not (release_dir / "services" / "test_svc.elf.sig").exists()

    # Verify hashes
    kernel_hash = (release_dir / "kernel" / "kernel.elf.sha256").read_text()
    assert compute_sha256(release_dir / "kernel" / "kernel.elf") in kernel_hash

    svc_hash = (release_dir / "services" / "test_svc.elf.sha256").read_text()
    assert compute_sha256(release_dir / "services" / "test_svc.elf") in svc_hash

@pytest.mark.skipif(not HAS_CRYPTO, reason="cryptography package not installed")
def test_create_release_layout_with_key(tmp_path: Path):
    build_dir, dist_dir, manifest_path = setup_release_env(tmp_path)

    out_dir = tmp_path / "keys"
    priv_path = generate_keypair(str(out_dir))

    create_release_layout(str(build_dir), str(dist_dir), str(manifest_path), priv_path)

    release_dir = dist_dir / "Bharat-OS-1.0.0"

    # Verify manifest
    assert (release_dir / "manifest.json").exists()
    assert (release_dir / "manifest.sig").exists()

    # Verify kernel
    assert (release_dir / "kernel" / "kernel.elf").exists()
    assert (release_dir / "kernel" / "kernel.elf.sha256").exists()
    assert (release_dir / "kernel" / "kernel.elf.sig").exists()

    # Verify services
    assert (release_dir / "services" / "test_svc.elf").exists()
    assert (release_dir / "services" / "test_svc.elf.sha256").exists()
    assert (release_dir / "services" / "test_svc.elf.sig").exists()
