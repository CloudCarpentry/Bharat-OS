import os
import sys
import glob
from pycparser import c_parser, c_ast
import common

UAPI_DIR = "include/bharat/uapi"

def remove_comments(text):
    import re
    # Remove single line and multi-line C comments
    text = re.sub(r'//.*?\n', '\n', text)
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.DOTALL)
    return text

def preprocess(filepath):
    with open(filepath, 'r') as f:
        content = f.read()

    content = remove_comments(content)

    lines = []
    for line in content.split('\n'):
        line = line.strip()
        if line.startswith('#'):
            continue
        # simple hack to support some compiler-specific stuff if any
        lines.append(line)

    # Provide simple typedefs so pycparser doesn't choke on stdint
    header = """
    typedef char int8_t;
    typedef short int16_t;
    typedef int int32_t;
    typedef long long int64_t;
    typedef unsigned char uint8_t;
    typedef unsigned short uint16_t;
    typedef unsigned int uint32_t;
    typedef unsigned long long uint64_t;
    typedef unsigned long size_t;
    typedef long ssize_t;
    typedef int bool;

    typedef uint64_t bharat_handle_t;
    typedef uint64_t bharat_object_id_t;
    typedef uint64_t bharat_cap_id_t;
    typedef uint32_t bharat_service_id_t;
    typedef uint32_t bharat_interface_id_t;
    typedef uint32_t bharat_interface_version_t;

    typedef uint32_t bharat_device_class_t;
    typedef uint32_t bharat_service_status_t;
    typedef uint32_t sys_errno_t;
    """

    return header + '\n' + '\n'.join(lines)

class StructVisitor(c_ast.NodeVisitor):
    def __init__(self):
        self.structs = {}

    def visit_Typedef(self, node):
        if isinstance(node.type.type, c_ast.Struct):
            struct_name = node.name
            fields = []
            if node.type.type.decls:
                for decl in node.type.type.decls:
                    field_name = decl.name
                    # Extract type string
                    typ = decl.type
                    type_str = ""

                    if isinstance(typ, c_ast.TypeDecl):
                        if isinstance(typ.type, c_ast.IdentifierType):
                            type_str = " ".join(typ.type.names)
                    elif isinstance(typ, c_ast.ArrayDecl):
                        if isinstance(typ.type, c_ast.TypeDecl):
                            if isinstance(typ.type.type, c_ast.IdentifierType):
                                type_str = " ".join(typ.type.type.names)
                        type_str += f"[{typ.dim.value if hasattr(typ.dim, 'value') else '?'}]"
                    elif isinstance(typ, c_ast.PtrDecl):
                        if isinstance(typ.type, c_ast.TypeDecl):
                            if isinstance(typ.type.type, c_ast.IdentifierType):
                                type_str = " ".join(typ.type.type.names)
                        type_str += "*"

                    fields.append({"name": field_name, "type": type_str})

            self.structs[struct_name] = {"fields": fields}

def generate_struct_manifest():
    parser = c_parser.CParser()
    manifest = {}

    for root, dirs, files in os.walk(UAPI_DIR):
        for file in files:
            if not file.endswith('.h'):
                continue

            filepath = os.path.join(root, file)
            # Some headers might have complex macros we can't parse easily with pycparser without a real CPP.
            # We'll try to parse, and if it fails, we skip or report.
            # Specifically we care about syscall_args.h and stable UAPI structs.
            try:
                text = preprocess(filepath)
                ast = parser.parse(text, filename=filepath)
                visitor = StructVisitor()
                visitor.visit(ast)

                for k, v in visitor.structs.items():
                    manifest[k] = v
            except Exception as e:
                # Silently ignore unparseable files, unless they are carrier structs
                if 'syscall_args' in filepath:
                    common.report_error(f"Failed to parse {filepath}: {e}")

    return manifest

def check_struct_layouts(baseline, current):
    if baseline is None:
        return False

    success = True

    for struct_name, base_info in baseline.items():
        if struct_name not in current:
            common.report_error(f"Struct {struct_name} was removed from ABI.")
            success = False
            continue

        curr_info = current[struct_name]

        base_fields = base_info.get("fields", [])
        curr_fields = curr_info.get("fields", [])

        if len(curr_fields) < len(base_fields):
            common.report_error(f"Struct {struct_name} has fewer fields. ABI structs can only be appended to.")
            success = False
            continue

        for i, b_field in enumerate(base_fields):
            c_field = curr_fields[i]
            if b_field["name"] != c_field["name"]:
                common.report_error(f"Struct {struct_name} field {i} name changed from {b_field['name']} to {c_field['name']}. Reordering/renaming is forbidden.")
                success = False
            if b_field["type"] != c_field["type"]:
                common.report_error(f"Struct {struct_name} field {b_field['name']} type changed from {b_field['type']} to {c_field['type']}.")
                success = False

    return success

if __name__ == "__main__":
    curr = generate_struct_manifest()
    import json
    print(json.dumps(curr, indent=2))
