import pytest
from unittest.mock import patch, mock_open, MagicMock
import sys
import yaml
import jsonschema

from tools.build.target_resolver import validate_yaml_target

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
