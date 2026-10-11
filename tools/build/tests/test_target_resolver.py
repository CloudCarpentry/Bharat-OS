import pytest
from unittest.mock import patch, mock_open, MagicMock
import sys
import yaml
import jsonschema

from pathlib import Path
from tools.build.target_resolver import (
    validate_yaml_target,
    resolve_yaml_target,
    augment_package_config,
    RUNTIME_ROOT_COMPONENTS,
    RUNTIME_MODEL_IDS,
)
from tools.build.models import PackageConfig, PackageTransformConfig, BootConfig, KernelConfig, DtbConfig

def test_validate_yaml_target_missing_schema():
    """Test that validation fails closed when schema is missing."""
    with patch("pathlib.Path.exists", return_value=False):
        with patch("builtins.print") as mock_print:
            with pytest.raises(SystemExit) as exc_info:
                validate_yaml_target({"name": "test_target"})

            assert exc_info.value.code == 1
            mock_print.assert_called_once()
            assert "Error: Target schema not found at" in mock_print.call_args[0][0]

def test_validate_yaml_target_malformed_schema():
    """Test that malformed schema raises appropriate yaml exception."""
    with patch("pathlib.Path.exists", return_value=True):
        m = mock_open(read_data="[malformed yaml")
        with patch("builtins.open", m):
            with pytest.raises(yaml.YAMLError):
                validate_yaml_target({"name": "test_target"})

@patch("jsonschema.validate")
def test_validate_yaml_target_invalid_target(mock_validate):
    """Test that invalid target exits with code 1."""
    mock_validate.side_effect = jsonschema.exceptions.ValidationError("Invalid property")

    with patch("pathlib.Path.exists", return_value=True):
        m = mock_open(read_data="type: object")
        with patch("builtins.open", m):
            with patch("builtins.print") as mock_print:
                with pytest.raises(SystemExit) as exc_info:
                    validate_yaml_target({"name": "test_target"})

                assert exc_info.value.code == 1
                mock_print.assert_called_once_with("Schema Validation Error: Invalid property")

@patch("jsonschema.validate")
def test_validate_yaml_target_valid_target(mock_validate):
    """Test that valid target passes without exiting."""
    with patch("pathlib.Path.exists", return_value=True):
        m = mock_open(read_data="type: object")
        with patch("builtins.open", m):
            validate_yaml_target({"name": "test_target"})
            mock_validate.assert_called_once()

@pytest.mark.parametrize("runtime_model, expected_root_component", RUNTIME_ROOT_COMPONENTS.items())
@patch("tools.build.target_resolver.validate_yaml_target")
@patch("tools.build.target_resolver.load_yaml_target")
@patch("tools.build.target_resolver.resolve_target_yaml_path")
def test_resolve_yaml_target_runtime_models(mock_resolve_path, mock_load, mock_validate, runtime_model, expected_root_component):
    """Test each supported userspace runtime model mapping and ID consistency."""
    mock_resolve_path.return_value = Path("/fake/target.yaml")

    mock_load.return_value = {
        "name": "test_target",
        "userspace": {
            "runtime_model": runtime_model
        }
    }

    resolved = resolve_yaml_target(Path("dummy"))

    assert resolved.userspace.runtime_model == runtime_model
    assert resolved.userspace.root_component == expected_root_component
    assert resolved.build.cmake_defs["BHARAT_USERSPACE_RUNTIME_MODEL"] == runtime_model.upper()
    assert resolved.build.cmake_defs["BHARAT_USERSPACE_RUNTIME_MODEL_ID"] == RUNTIME_MODEL_IDS[runtime_model]

def test_augment_package_config_x86_64_multiboot2():
    """Test multiboot2 ELF fix is appended for x86_64 architectures."""
    package_cfg = PackageConfig(transforms=[])
    boot_cfg = BootConfig(protocol="multiboot2", artifact_format="unknown", dtb=DtbConfig(mode="unknown"))
    kernel_cfg = KernelConfig(canonical_elf="kernel/kernel.elf")

    augmented = augment_package_config(package_cfg, arch="x86_64", boot=boot_cfg, kernel=kernel_cfg)

    assert len(augmented.transforms) == 1
    assert augmented.transforms[0].type == "multiboot_elf_fix"
    assert augmented.transforms[0].input == "kernel/kernel.elf"
    assert augmented.transforms[0].output == "kernel/kernel.elf32"

def test_augment_package_config_raw_bin():
    """Test raw binary conversion is appended for raw_bin artifact format."""
    package_cfg = PackageConfig(transforms=[])
    boot_cfg = BootConfig(protocol="unknown", artifact_format="raw_bin", dtb=DtbConfig(mode="unknown"))
    kernel_cfg = KernelConfig(canonical_elf="kernel/kernel.elf")

    augmented = augment_package_config(package_cfg, arch="arm64", boot=boot_cfg, kernel=kernel_cfg)

    assert len(augmented.transforms) == 1
    assert augmented.transforms[0].type == "elf_to_bin"
    assert augmented.transforms[0].input == "kernel/kernel.elf"
    assert augmented.transforms[0].output == "kernel/kernel.bin"

def test_augment_package_config_duplicate_prevention():
    """Test duplicate transforms are not appended."""
    existing_transforms = [
        PackageTransformConfig(type="multiboot_elf_fix", input="in", output="out"),
        PackageTransformConfig(type="elf_to_bin", input="in", output="out")
    ]
    package_cfg = PackageConfig(transforms=existing_transforms)
    boot_cfg = BootConfig(protocol="multiboot2", artifact_format="raw_bin", dtb=DtbConfig(mode="unknown"))
    kernel_cfg = KernelConfig(canonical_elf="kernel/kernel.elf")

    augmented = augment_package_config(package_cfg, arch="x86_64", boot=boot_cfg, kernel=kernel_cfg)

    assert len(augmented.transforms) == 2
    assert augmented.transforms[0].type == "multiboot_elf_fix"
    assert augmented.transforms[1].type == "elf_to_bin"

@patch("tools.build.target_resolver.validate_yaml_target")
@patch("tools.build.target_resolver.load_yaml_target")
@patch("tools.build.target_resolver.resolve_target_yaml_path")
def test_resolve_yaml_target_missing_optional_config(mock_resolve_path, mock_load, mock_validate):
    """Test resolution with missing optional configuration (e.g., run, flash, debug)."""
    mock_resolve_path.return_value = Path("/fake/target.yaml")

    # Provide minimal target configuration
    mock_load.return_value = {
        "name": "minimal_target"
    }

    resolved = resolve_yaml_target(Path("dummy"))

    assert resolved.run is None
    assert resolved.flash is None
    assert resolved.debug is None
    assert resolved.execution_profile is None
    assert resolved.footprint_profile is None
    # Verify defaults for userspace
    assert resolved.userspace.runtime_model == "full"

@patch("tools.build.target_resolver.validate_yaml_target")
@patch("tools.build.target_resolver.load_yaml_target")
@patch("tools.build.target_resolver.resolve_target_yaml_path")
def test_resolve_yaml_target_invalid_runtime_model(mock_resolve_path, mock_load, mock_validate):
    """Test resolution with an invalid/unsupported runtime model raises KeyError."""
    mock_resolve_path.return_value = Path("/fake/target.yaml")

    mock_load.return_value = {
        "name": "invalid_target",
        "userspace": {
            "runtime_model": "unsupported_model"
        }
    }

    with pytest.raises(KeyError) as exc:
        resolve_yaml_target(Path("dummy"))

    assert "unsupported_model" in str(exc.value)

@patch("tools.build.target_resolver.resolve_target_yaml_path")
def test_load_yaml_target_missing_file(mock_resolve_path):
    """Test loading a missing target YAML file raises FileNotFoundError."""
    mock_resolve_path.return_value = Path("/nonexistent/target.yaml")

    with patch("pathlib.Path.exists", return_value=False):
        with pytest.raises(FileNotFoundError) as exc:
            from tools.build.target_resolver import load_yaml_target
            load_yaml_target(Path("dummy"))

        assert "Target YAML not found:" in str(exc.value)

@patch("tools.build.target_resolver.resolve_target_yaml_alias")
def test_resolve_target_yaml_path_alias_warning(mock_resolve_alias):
    """Test that resolving an aliased path prints a migration warning."""
    mock_resolve_alias.return_value = (Path("/resolved/target.yaml"), True)

    with patch("builtins.print") as mock_print:
        from tools.build.target_resolver import resolve_target_yaml_path
        resolve_target_yaml_path(Path("/aliased/target.yaml"))

        mock_print.assert_called_once()
        assert "[migration-warning] Using aliased target-yaml path" in mock_print.call_args[0][0]

@patch("tools.build.target_resolver.validate_yaml_target")
@patch("tools.build.target_resolver.load_yaml_target")
@patch("tools.build.target_resolver.resolve_target_yaml_path")
def test_resolve_yaml_target_deterministic(mock_resolve_path, mock_load, mock_validate):
    """Test that resolving identical target configurations yields identical ResolvedTarget instances."""
    mock_resolve_path.return_value = Path("/fake/target.yaml")

    config = {
        "name": "deterministic_target",
        "arch": "x86_64",
        "userspace": {"runtime_model": "light"},
        "boot": {"protocol": "multiboot2", "artifact_format": "elf"}
    }
    mock_load.return_value = config

    resolved_1 = resolve_yaml_target(Path("dummy"))
    resolved_2 = resolve_yaml_target(Path("dummy"))

    assert resolved_1 == resolved_2
