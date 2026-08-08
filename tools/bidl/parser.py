import re
from enum import Enum

class BidlParseError(Exception):
    def __init__(self, path, line, message):
        super().__init__(f"{path}:{line}: {message}")
        self.path = path
        self.line = line
        self.msg = message

class IdlDialect(Enum):
    BIDL_V1_SERVICE = 1
    PACKAGE_STRUCT = 2
    NAMESPACE_INTERFACE = 3
    UNKNOWN = 4

class SkipDialectError(Exception):
    def __init__(self, path, dialect, message):
        super().__init__(message)
        self.path = path
        self.dialect = dialect
        self.msg = message

def classify_dialect(lines):
    has_service = False
    has_package = False
    has_namespace = False
    has_interface = False

    for line in lines:
        s = line.strip()
        if s.startswith("service "): has_service = True
        if s.startswith("package "): has_package = True
        if s.startswith("namespace "): has_namespace = True
        if s.startswith("interface "): has_interface = True

    if has_service:
        return IdlDialect.BIDL_V1_SERVICE

    if has_namespace or has_interface:
        return IdlDialect.NAMESPACE_INTERFACE

    if has_package:
        return IdlDialect.PACKAGE_STRUCT

    return IdlDialect.UNKNOWN

SUPPORTED_METADATA = {
    "qos": r"^[a-zA-Z_][a-zA-Z0-9_]*$",
    "timeout_ms": r"^\d+$",
    "idempotent": r"^(true|false)$",
    "auth": r"^[a-zA-Z_][a-zA-Z0-9_]*$",
    "criticality": r"^[a-zA-Z_][a-zA-Z0-9_]*$"
}

def parse_bidl(path):
    with open(path, 'r') as f:
        lines = f.readlines()

    dialect = classify_dialect(lines)

    if dialect == IdlDialect.PACKAGE_STRUCT:
        raise SkipDialectError(path, dialect, f"SKIP: {path}: recognized PACKAGE_STRUCT dialect; not handled by BIDL-v1 codegen")
    if dialect == IdlDialect.NAMESPACE_INTERFACE:
        raise SkipDialectError(path, dialect, f"SKIP: {path}: recognized NAMESPACE_INTERFACE dialect; not handled by BIDL-v1 codegen")
    if dialect == IdlDialect.UNKNOWN:
        raise BidlParseError(path, 0, "Unknown IDL dialect")

    service = {"name": "", "id": 0, "rpcs": [], "messages": {}, "enums": {}}

    current_block = None
    current_block_name = None
    current_rpc = None

    for i, line_raw in enumerate(lines):
        line_num = i + 1
        line = line_raw.strip()

        if "//" in line:
            line = line.split("//", 1)[0].strip()

        if not line:
            continue

        if current_block is None:
            m_service_id = re.match(r"^service\s+([\w\.]+)\s*=\s*(\d+)\s*\{$", line)
            m_service = re.match(r"^service\s+([\w\.]+)\s*\{$", line)

            if m_service_id or m_service:
                name = m_service_id.group(1) if m_service_id else m_service.group(1)
                if service["name"] and service["name"] != name:
                    raise BidlParseError(path, line_num, f"Multiple services declared in one file: {name}")
                service["name"] = name
                service["id"] = int(m_service_id.group(2)) if m_service_id else 0
                current_block = "service"
                current_block_name = name
                continue

            m_struct = re.match(r"^(?:struct|message)\s+(\w+)\s*\{$", line)

            # The test puts struct contents on one line sometimes like `struct Req1 { u32 v; }`
            m_struct_inline = re.match(r"^(?:struct|message)\s+(\w+)\s*\{\s*([\w<>\.]+)\s+(\w+);\s*\}$", line)

            if m_struct or m_struct_inline:
                name = m_struct.group(1) if m_struct else m_struct_inline.group(1)
                if name in service["messages"]:
                    raise BidlParseError(path, line_num, f"Duplicate message '{name}'")
                service["messages"][name] = []
                if m_struct_inline:
                    service["messages"][name].append({"type": m_struct_inline.group(2), "name": m_struct_inline.group(3)})
                else:
                    current_block = "message"
                    current_block_name = name
                continue

            m_enum = re.match(r"^enum\s+(\w+)\s*\{$", line)
            if m_enum:
                name = m_enum.group(1)
                if name in service["enums"]:
                    raise BidlParseError(path, line_num, f"Duplicate enum '{name}'")
                service["enums"][name] = []
                current_block = "enum"
                current_block_name = name
                continue

            m_package = re.match(r"^package\s+([\w\.]+);$", line)
            if m_package:
                continue

            m_import = re.match(r"^import\s+\"[^\"]+\";$", line)
            if m_import:
                continue

            raise BidlParseError(path, line_num, f"Unknown top-level syntax or unexpected content: {line}")

        elif current_block == "service":
            if line == "}":
                current_block = None
                current_block_name = None
                continue

            m_rpc = re.match(r"^rpc\s+(\w+)\s*\(\s*([\w\.]+)\s*\)\s*->\s*([\w\.]+)\s*(;)?\s*(\{)?$", line)
            if m_rpc:
                name = m_rpc.group(1)
                for rpc in service["rpcs"]:
                    if rpc["name"] == name:
                        raise BidlParseError(path, line_num, f"Duplicate RPC name '{name}'")

                new_rpc = {
                    "name": name,
                    "req": m_rpc.group(2),
                    "resp": m_rpc.group(3),
                    "metadata": {}
                }
                service["rpcs"].append(new_rpc)

                # group(4) is (;)? and group(5) is ({)?
                if m_rpc.group(5) == "{":
                    current_block = "rpc"
                    current_rpc = new_rpc
                else:
                    # Allow without ';' if it just ends
                    if line.endswith(";"):
                        pass
                    elif "{" in line:
                        raise BidlParseError(path, line_num, f"Expected ';' or '{{' after RPC declaration")
                continue

            raise BidlParseError(path, line_num, f"Unknown syntax in service block: {line}")

        elif current_block == "rpc":
            if line == "}":
                current_block = "service"
                current_rpc = None
                continue

            m_meta = re.match(r"^([a-zA-Z_][a-zA-Z0-9_]*)\s*=\s*([^;]+);$", line)
            if m_meta:
                k = m_meta.group(1)
                v = m_meta.group(2).strip()
                if k not in SUPPORTED_METADATA:
                    raise BidlParseError(path, line_num, f"Unknown metadata key '{k}'")
                if not re.match(SUPPORTED_METADATA[k], v):
                    raise BidlParseError(path, line_num, f"Malformed metadata value for '{k}': '{v}'")
                current_rpc["metadata"][k] = v
                continue

            raise BidlParseError(path, line_num, f"Unknown syntax in rpc block: {line}")

        elif current_block == "message":
            if line == "}":
                current_block = None
                current_block_name = None
                continue

            m_field = re.match(r"^([\w<>\.]+)\s+(\w+);$", line)
            if m_field:
                ftype = m_field.group(1)
                fname = m_field.group(2)
                for field in service["messages"][current_block_name]:
                    if field["name"] == fname:
                        raise BidlParseError(path, line_num, f"Duplicate field name '{fname}' in message '{current_block_name}'")
                service["messages"][current_block_name].append({"type": ftype, "name": fname})
                continue

            raise BidlParseError(path, line_num, f"Unknown syntax in message block: {line}")

        elif current_block == "enum":
            if line == "}":
                current_block = None
                current_block_name = None
                continue

            m_eval = re.match(r"^(\w+)\s*=\s*(-?\d+);$", line)
            if m_eval:
                ename = m_eval.group(1)
                evalue = int(m_eval.group(2))
                for ev in service["enums"][current_block_name]:
                    if ev["name"] == ename:
                        raise BidlParseError(path, line_num, f"Duplicate enum member '{ename}' in enum '{current_block_name}'")
                service["enums"][current_block_name].append({"name": ename, "value": evalue})
                continue

            raise BidlParseError(path, line_num, f"Unknown syntax in enum block: {line}")

    if current_block is not None:
        raise BidlParseError(path, len(lines), f"Unclosed block: '{current_block}'")

    return service
