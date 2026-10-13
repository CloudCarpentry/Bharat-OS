import re
with open("tools/nirmaan/cli.py", "r") as f:
    content = f.read()

# Add resolve and verify commands to commands dict
replacement = """    commands = {
        "build": "Build a target.",
        "run": "Run a target.",
        "test": "Test a target.",
        "debug": "Debug a target.",
        "package": "Package a target.",
        "doctor": "Check development environment.",
        "targets": "List available targets.",
        "demo": "Run a demo target.",
        "resolve": "Resolve composition for a target.",
        "verify": "Verify composition for a target.",
    }"""
content = re.sub(r'    commands = \{.*?\n    \}', replacement, content, flags=re.DOTALL)

with open("tools/nirmaan/cli.py", "w") as f:
    f.write(content)
