import pytest
from unittest.mock import patch

from tools.build.models import (
    ResolvedTarget, UserspaceConfig, BuildConfig, KernelConfig,
    BootConfig, DtbConfig, PackageConfig, PackageTransformConfig,
    RunConfig, FlashConfig
)
from tools.build.validators import (
    validate_resolved_target, validate_boot_contract, validate_run_contract,
    validate_flash_contract, validate_arch_profile_contract
)

def create_mock_target(
    kind="qemu_target", arch="arm64",
    cmake_defs=None, flash_config=None, run_config=None,
    boot_protocol="linux_arm64", boot_artifact_format="raw_bin",
    dtb_required=False, dtb_mode="qemu_generated",
    canonical_elf="test.elf", transforms=None
):
    build_config = BuildConfig(cmake_preset="test", cmake_defs=cmake_defs or {})
    kernel_config = KernelConfig(canonical_elf=canonical_elf)
    boot_config = BootConfig(
        protocol=boot_protocol, artifact_format=boot_artifact_format,
        dtb=DtbConfig(mode=dtb_mode, required=dtb_required)
    )
    package_config = PackageConfig(transforms=transforms or [])

    return ResolvedTarget(
        name="test_target",
        kind=kind,
        arch=arch,
        board="test_board",
        device_profile="test_device",
        personality_profile="test_personality",
        execution_profile=None,
        userspace=UserspaceConfig(),
        build=build_config,
        kernel=kernel_config,
        boot=boot_config,
        package=package_config,
        run=run_config,
        flash=flash_config
    )

def test_validate_arch_profile_contract():
    # Matching arch family
    target = create_mock_target(arch="arm64", cmake_defs={"BHARAT_ARCH_FAMILY": "ARM64"})
    validate_arch_profile_contract(target)

    # Mismatched arch family
    target = create_mock_target(arch="arm64", cmake_defs={"BHARAT_ARCH_FAMILY": "ARM32"})
    with pytest.raises(SystemExit):
        validate_arch_profile_contract(target)

    # Missing defs
    target = create_mock_target(arch="arm64")
    target.build.cmake_defs = None
    validate_arch_profile_contract(target)

    # Missing BHARAT_ARCH_FAMILY
    target = create_mock_target(arch="arm64", cmake_defs={"OTHER_DEF": "VALUE"})
    validate_arch_profile_contract(target)

    # Unknown arch
    target = create_mock_target(arch="unknown_arch", cmake_defs={"BHARAT_ARCH_FAMILY": "ARM64"})
    validate_arch_profile_contract(target)

def test_validate_flash_contract():
    # No flash config
    target = create_mock_target(kind="board_target")
    with pytest.raises(SystemExit):
        validate_flash_contract(target)

    # Backend openocd missing artifact
    flash_config = FlashConfig(backend="openocd", artifact=None)
    target = create_mock_target(kind="board_target", flash_config=flash_config)
    with pytest.raises(SystemExit):
        validate_flash_contract(target)

    # Valid flash config with openocd
    flash_config = FlashConfig(backend="openocd", artifact="test.bin")
    target = create_mock_target(kind="board_target", flash_config=flash_config)
    validate_flash_contract(target)

    # Other backend without artifact
    flash_config = FlashConfig(backend="pyocd", artifact=None)
    target = create_mock_target(kind="board_target", flash_config=flash_config)
    validate_flash_contract(target)

def test_validate_run_contract():
    # No run config
    target = create_mock_target(kind="qemu_target")
    with pytest.raises(SystemExit):
        validate_run_contract(target)

    # Backend qemu missing machine
    run_config = RunConfig(backend="qemu", machine=None, boot_artifact="test.bin")
    target = create_mock_target(kind="qemu_target", run_config=run_config)
    with pytest.raises(SystemExit):
        validate_run_contract(target)

    # Missing boot_artifact
    run_config = RunConfig(backend="qemu", machine="virt", boot_artifact=None)
    target = create_mock_target(kind="qemu_target", run_config=run_config)
    with pytest.raises(SystemExit):
        validate_run_contract(target)

    # Valid run config
    run_config = RunConfig(backend="qemu", machine="virt", boot_artifact="test.bin")
    target = create_mock_target(kind="qemu_target", run_config=run_config)
    validate_run_contract(target)

    # Non-qemu valid config
    run_config = RunConfig(backend="renode", machine=None, boot_artifact="test.bin")
    target = create_mock_target(kind="qemu_target", run_config=run_config)
    validate_run_contract(target)

def test_validate_boot_contract():
    # protocol linux_arm64 semantics (pass-through)
    target = create_mock_target(boot_protocol="linux_arm64", boot_artifact_format="raw_bin", canonical_elf="test.bin")
    validate_boot_contract(target)

    # elf_direct with wrong format
    target = create_mock_target(boot_protocol="elf_direct", boot_artifact_format="raw_bin")
    with pytest.raises(SystemExit):
        validate_boot_contract(target)

    # elf_direct with elf format
    target = create_mock_target(boot_protocol="elf_direct", boot_artifact_format="elf")
    validate_boot_contract(target)

    # raw_bin with no transform and wrong canonical_elf ending
    target = create_mock_target(boot_protocol="raw_entry", boot_artifact_format="raw_bin", canonical_elf="test.elf")
    with pytest.raises(SystemExit):
        validate_boot_contract(target)

    # raw_bin with transform
    transforms = [PackageTransformConfig(type="elf_to_bin", input="in", output="out")]
    target = create_mock_target(boot_protocol="raw_entry", boot_artifact_format="raw_bin", canonical_elf="test.elf", transforms=transforms)
    validate_boot_contract(target)

    # raw_bin without transform but ends with .bin
    target = create_mock_target(boot_protocol="raw_entry", boot_artifact_format="raw_bin", canonical_elf="test.bin")
    validate_boot_contract(target)

    # DTB required but invalid mode
    target = create_mock_target(boot_protocol="linux_arm64", boot_artifact_format="raw_bin", canonical_elf="test.bin", dtb_required=True, dtb_mode="invalid_mode")
    with pytest.raises(SystemExit):
        validate_boot_contract(target)

    # DTB required and valid mode
    target = create_mock_target(boot_protocol="linux_arm64", boot_artifact_format="raw_bin", canonical_elf="test.bin", dtb_required=True, dtb_mode="qemu_generated")
    validate_boot_contract(target)

@patch("tools.build.validators.validate_boot_contract")
@patch("tools.build.validators.validate_arch_profile_contract")
@patch("tools.build.validators.validate_footprint_contract")
@patch("tools.build.validators.validate_run_contract")
@patch("tools.build.validators.validate_flash_contract")
def test_validate_resolved_target(
    mock_flash, mock_run, mock_footprint, mock_arch, mock_boot
):
    # Test qemu_target
    target = create_mock_target(kind="qemu_target")
    validate_resolved_target(target)

    mock_boot.assert_called_once_with(target)
    mock_arch.assert_called_once_with(target)
    mock_footprint.assert_not_called()
    mock_run.assert_called_once_with(target)
    mock_flash.assert_not_called()

    # Reset mocks
    mock_boot.reset_mock()
    mock_arch.reset_mock()
    mock_run.reset_mock()

    # Test board_target
    target = create_mock_target(kind="board_target")
    validate_resolved_target(target)

    mock_boot.assert_called_once_with(target)
    mock_arch.assert_called_once_with(target)
    mock_footprint.assert_not_called()
    mock_run.assert_not_called()
    mock_flash.assert_called_once_with(target)

    # Test with repo_root
    mock_boot.reset_mock()
    mock_arch.reset_mock()
    mock_flash.reset_mock()

    from pathlib import Path
    repo_root = Path("/tmp")
    target = create_mock_target(kind="other_target")
    validate_resolved_target(target, repo_root)

    mock_boot.assert_called_once_with(target)
    mock_arch.assert_called_once_with(target)
    mock_footprint.assert_called_once_with(target, repo_root)
    mock_run.assert_not_called()
    mock_flash.assert_not_called()

    # Test footprint exception
    mock_footprint.side_effect = ValueError("Footprint error")
    with pytest.raises(SystemExit):
        validate_resolved_target(target, repo_root)
