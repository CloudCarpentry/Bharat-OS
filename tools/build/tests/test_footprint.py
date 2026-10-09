import pytest
from pathlib import Path
from tools.build.footprint import _parse_memory_to_kb, validate_footprint_contract
from tools.build.models import ResolvedTarget, RunConfig, UserspaceConfig, BuildConfig, KernelConfig, BootConfig, DtbConfig, PackageConfig

def test_parse_memory_to_kb():
    assert _parse_memory_to_kb(None) is None
    assert _parse_memory_to_kb("") is None
    assert _parse_memory_to_kb("   ") is None
    assert _parse_memory_to_kb("1G") == 1048576
    assert _parse_memory_to_kb("2g") == 2097152
    assert _parse_memory_to_kb("512M") == 524288
    assert _parse_memory_to_kb("1024m") == 1048576
    assert _parse_memory_to_kb("1024K") == 1024
    assert _parse_memory_to_kb("2048k") == 2048
    assert _parse_memory_to_kb("1048576") == 1024

    # Unrecognized formats
    assert _parse_memory_to_kb("invalid") is None
    assert _parse_memory_to_kb("10X") is None

    # Edge cases
    assert _parse_memory_to_kb("-10G") == -10485760
    assert _parse_memory_to_kb("-512M") == -524288

    with pytest.raises(ValueError):
        _parse_memory_to_kb("1.5G")

    assert _parse_memory_to_kb("1.5") is None

def create_mock_target(arch="arm64", memory="512M", footprint_profile="test_profile"):
    return ResolvedTarget(
        name="test_target",
        kind="qemu_target",
        arch=arch,
        board="test_board",
        device_profile="test_device",
        personality_profile="test_personality",
        execution_profile=None,
        userspace=UserspaceConfig(),
        build=BuildConfig(cmake_preset="test"),
        kernel=KernelConfig(canonical_elf="test.elf"),
        boot=BootConfig(protocol="raw", artifact_format="bin", dtb=DtbConfig(mode="qemu_generated")),
        package=PackageConfig(),
        run=RunConfig(backend="qemu", memory=memory),
        footprint_profile=footprint_profile
    )

def test_validate_footprint_contract(tmp_path):
    repo_root = tmp_path
    matrix_dir = repo_root / "configs" / "footprint"
    matrix_dir.mkdir(parents=True)
    matrix_path = matrix_dir / "footprint_matrix.csv"

    with open(matrix_path, "w") as f:
        f.write("profile_id,arch,boot_min_ram_kb\n")
        f.write("test_profile,arm64,262144\n") # 256M
        f.write("small_profile,arm64,65536\n") # 64M
        f.write("missing_boot_profile,arm64,\n")

    # Happy path
    target = create_mock_target(arch="arm64", memory="512M", footprint_profile="test_profile")
    validate_footprint_contract(target, repo_root)

    # Unknown profile
    target_unknown = create_mock_target(footprint_profile="unknown_profile")
    with pytest.raises(ValueError, match="Unknown footprint_profile 'unknown_profile'"):
        validate_footprint_contract(target_unknown, repo_root)

    # Arch mismatch
    target_arch = create_mock_target(arch="x86_64", footprint_profile="test_profile")
    with pytest.raises(ValueError, match="Footprint profile arch mismatch"):
        validate_footprint_contract(target_arch, repo_root)

    # Memory below boot minimum
    target_mem = create_mock_target(memory="128M", footprint_profile="test_profile")
    with pytest.raises(ValueError, match="is below boot minimum"):
        validate_footprint_contract(target_mem, repo_root)

    # No matrix path
    validate_footprint_contract(target, tmp_path / "nonexistent")

    # No footprint profile on target
    target_no_profile = create_mock_target(footprint_profile=None)
    validate_footprint_contract(target_no_profile, repo_root)

    # Missing run config
    target_no_run = create_mock_target(footprint_profile="test_profile")
    target_no_run.run = None
    # No exception should be raised as run_memory_kb will be None
    validate_footprint_contract(target_no_run, repo_root)

    # Missing boot_min_ram_kb in matrix (defaults to 0)
    target_missing_boot = create_mock_target(footprint_profile="missing_boot_profile")
    validate_footprint_contract(target_missing_boot, repo_root)

    # Non-numeric boot_min_ram_kb raises ValueError
    with open(matrix_path, "a") as f:
        f.write("invalid_boot_profile,arm64,invalid_value\n")
    target_invalid_boot = create_mock_target(footprint_profile="invalid_boot_profile")
    with pytest.raises(ValueError):
        validate_footprint_contract(target_invalid_boot, repo_root)
