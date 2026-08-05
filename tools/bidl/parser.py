import re

def parse_bidl(path):
    with open(path, 'r') as f:
        lines = f.readlines()

    service = {"name": "", "id": 0, "rpcs": [], "messages": {}, "enums": {}}

    current_msg = None
    current_enum = None

    for line in lines:
        line = line.strip()

        if not line or line.startswith("//"):
            continue

        # service
        m = re.match(r"service\s+([\w\.]+)\s*=\s*(\d+)\s*\{", line)
        if m:
            service["name"] = m.group(1)
            service["id"] = int(m.group(2))
            continue

        m = re.match(r"service\s+([\w\.]+)\s*\{", line)
        if m:
            service["name"] = m.group(1)
            # Default ID if none specified
            service["id"] = 0
            continue

        # rpc
        m = re.match(r"rpc\s+(\w+)\s*\(\s*([\w\.]+)\s*\)\s*->\s*([\w\.]+)", line)
        if m:
            service["rpcs"].append({
                "name": m.group(1),
                "req": m.group(2),
                "resp": m.group(3)
            })
            continue

        # message start
        m = re.match(r"struct\s+(\w+)\s*\{", line)
        if not m:
            m = re.match(r"message\s+(\w+)\s*\{", line)

        if m:
            current_msg = m.group(1)
            service["messages"][current_msg] = []
            continue

        # enum start
        m = re.match(r"enum\s+(\w+)\s*\{", line)
        if m:
            current_enum = m.group(1)
            service["enums"][current_enum] = []
            continue

        # block end
        if line == "}":
            current_msg = None
            current_enum = None
            continue

        # message fields
        if current_msg:
            # Try to match 'type name;'
            m = re.match(r"([\w<>\.]+)\s+(\w+);", line)
            if m:
                service["messages"][current_msg].append(
                    {"type": m.group(1), "name": m.group(2)}
                )

        # enum fields
        if current_enum:
            # Try to match 'NAME = VALUE;'
            m = re.match(r"(\w+)\s*=\s*(\d+);", line)
            if m:
                service["enums"][current_enum].append(
                    {"name": m.group(1), "value": int(m.group(2))}
                )

    return service
