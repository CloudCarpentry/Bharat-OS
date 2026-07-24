import json
import os
import sys

from tools.build.path_aliases import (
    first_existing_path,
    repo_path_candidates,
    resolve_target_matrix_alias,
)


def _candidate_matrix_paths():
    return repo_path_candidates(
        "delivery/targets/target_matrix.json",
        "targets/target_matrix.json",
    )


def load_target_matrix(matrix_path=None):
    if not matrix_path:
        matrix_path = first_existing_path(_candidate_matrix_paths())
    else:
        requested_path = matrix_path if isinstance(matrix_path, str) else str(matrix_path)
        resolved, used_alias = resolve_target_matrix_alias(os.path.abspath(requested_path))
        matrix_path = str(resolved)
        if used_alias:
            print(f"[migration-warning] Using aliased target-matrix path: {os.path.abspath(requested_path)} -> {resolved}")

    if not matrix_path:
        print("Error loading target matrix: no target_matrix.json found in delivery/targets or targets.")
        sys.exit(1)

    try:
        with open(str(matrix_path), "r") as f:
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
