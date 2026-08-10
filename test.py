import re
line = "  rpc Validate(CapValidateReq) -> CapValidateResp {"
m_rpc = re.match(r"^rpc\s+(\w+)\s*\(\s*([\w\.]+)\s*\)\s*->\s*([\w\.]+)\s*(;)?\s*(\{)?$", line.strip())
print(m_rpc)
