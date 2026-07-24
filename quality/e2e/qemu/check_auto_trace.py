import sys
import re

def check_trace(log_file):
    with open(log_file, 'r') as f:
        log_content = f.read()

    required_events = [
        "0x5000", # BH_TRACE_AUTO_TASK_START
        "0x5005", # BH_TRACE_AUTO_CAN_RX
        "0x5003", # BH_TRACE_AUTO_WATCHDOG_HEARTBEAT
        "0x5001", # BH_TRACE_AUTO_TASK_END
    ]

    markers = [
        "AUTO_SMOKE: started.",
        "AUTO_SMOKE: completed"
    ]

    for marker in markers:
        if marker not in log_content:
            print(f"MISSING MARKER: {marker}")
            return False

    for event in required_events:
        if not re.search(f"TRACE:.*event={event}", log_content):
            print(f"MISSING TRACE EVENT: {event}")
            return False

    print("All automotive smoke markers and trace events found.")
    return True

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: check_auto_trace.py <log_file>")
        sys.exit(1)

    if check_trace(sys.argv[1]):
        sys.exit(0)
    else:
        sys.exit(1)
