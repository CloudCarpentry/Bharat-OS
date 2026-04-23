import json
import os
import sys


def _candidate_matrix_paths():
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(__file__)))
    return [
        os.path.join(repo_root, "delivery", "targets", "target_matrix.json"),
        os.path.join(repo_root, "targets", "target_matrix.json"),
    ]


def load_target_matrix(matrix_path=None):
    if not matrix_path:
        for candidate in _candidate_matrix_paths():
            if os.path.exists(candidate):
                matrix_path = candidate
                if "/targets/target_matrix.json" in candidate and "/delivery/" not in candidate:
                    print(f"[migration-warning] Using legacy target matrix path: {candidate}")
                break

    if not matrix_path:
        print("Error loading target matrix: no target_matrix.json found in delivery/targets or targets.")
        sys.exit(1)

    try:
        with open(matrix_path, "r") as f:
            return json.load(f)
    except Exception as e:
        print(f"Error loading {matrix_path}: {e}")
        sys.exit(1)

def get_target(target_name, matrix=None, silent=False):
    if not matrix:
        matrix = load_target_matrix()
    if target_name not in matrix:
        if not silent:
            print(f"Error: Target '{target_name}' not found in target matrix.")
        # If silent, raise KeyError instead of exiting, so caller can catch it without killing the process
        if silent:
            raise KeyError(target_name)
        sys.exit(1)
    return matrix[target_name]
