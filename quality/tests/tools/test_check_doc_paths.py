import pytest
import importlib.util
from pathlib import Path

# Load the module under test dynamically
REPO_ROOT = Path(__file__).resolve().parents[3]
SCRIPT_PATH = REPO_ROOT / "tools/docs/check_doc_paths.py"

def load_check_doc_paths():
    spec = importlib.util.spec_from_file_location("check_doc_paths", str(SCRIPT_PATH))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module

check_doc_paths = load_check_doc_paths()

@pytest.mark.parametrize("s,expected", [
    # Slash-based POSIX paths
    ("core/kernel/src/main.c", True),
    ("docs/adr/001-test.md", True),
    ("foo/bar", True),

    # Backslash-based Windows paths
    ("core\\kernel\\src\\main.c", True),
    ("docs\\adr\\001-test.md", True),
    ("foo\\bar", True),

    # Every supported extension
    ("file.md", True),
    ("file.json", True),
    ("file.yaml", True),
    ("file.c", True),
    ("file.h", True),
    ("file.S", True),
    ("file.py", True),
    ("file.sh", True),
    ("file.ld", True),
    ("file.png", True),
    ("file.jpg", True),

    # Strings without a recognized extension
    ("file.txt", False),
    ("file", False),
    ("some_random_word", False),

    # Commands containing spaces
    ("git commit -m 'test'", False),
    ("python3 script.py", True), # No slashes, but ends with '.py', so returns True in original logic
    ("foo/bar with space.md", False),

    # Wildcard, shell, comparison, and punctuation exclusions inside slash paths
    ("foo/bar/*", False),
    ("foo/bar/$VAR", False),
    ("foo/bar/<tag>", False),
    ("foo/bar/>tag", False),
    ("foo/bar|baz", False),
    ("foo/bar!baz", False),
    ("foo/bar==baz", False),
    ("foo/bar=baz", False),
    ("foo/bar(baz)", False),
    ("foo/bar{baz}", False),

    # Include-header-like strings
    ("<string.h>", False),
    ("<sys/types.h>", False),

    # Empty strings
    ("", False),
])
def test_looks_like_path(s, expected):
    assert check_doc_paths.looks_like_path(s) == expected
