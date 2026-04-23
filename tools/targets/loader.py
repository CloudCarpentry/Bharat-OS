import json
import os
import sys

def load_target_matrix(matrix_path=None):
    if not matrix_path:
        matrix_path = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(__file__))), 'targets', 'target_matrix.json')
    try:
        with open(matrix_path, 'r') as f:
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
