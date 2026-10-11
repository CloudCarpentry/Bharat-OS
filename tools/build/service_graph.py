import yaml
import hashlib
import json
from pathlib import Path

def resolve_service_graph(target, repo_root: Path):
    catalog_path = repo_root / "interface/contracts/services_catalog.yaml"
    if not catalog_path.exists():
        return {}

    with open(catalog_path, "r") as f:
        catalog = yaml.safe_load(f)

    services = catalog.get("services", {})

    target_services = {}
    if hasattr(target, "source_metadata") and target.source_metadata:
        target_path = Path(target.source_metadata.get("source_path", ""))
        if target_path.exists():
            with open(target_path, "r") as f:
                 raw_target = yaml.safe_load(f)
                 target_services = raw_target.get("services", {})

    runtime_model = target.userspace.runtime_model.upper()

    included = {}

    for svc_name, svc_info in services.items():
        state = "disabled"

        supported = svc_info.get("supported_runtime_models", [])
        if runtime_model not in supported:
            if target_services and svc_name in target_services and target_services[svc_name].get("state") in ("required", "optional"):
                raise ValueError(f"Service '{svc_name}' explicitly enabled but not supported on runtime '{runtime_model}'")
            continue

        if target_services and svc_name in target_services:
            state = target_services[svc_name].get("state", state)
        elif not target_services:
            if svc_name in ["namesvc", "process_manager"]:
                state = "required"
            elif svc_name in ["vm_manager", "devmgr", "faultmgr", "servicemgr", "boot_displayd", "telemetrymgr", "storagemgr", "accelmgr", "app_payload"]:
                state = "disabled"

        if state != "disabled":
            included[svc_name] = {
                "state": state,
                "info": svc_info
            }

    changed = True
    while changed:
        changed = False
        for svc_name, svc_data in list(included.items()):
            if svc_data["state"] == "required":
                deps = svc_data["info"].get("dependencies", [])
                for dep in deps:
                    if dep not in included:
                         raise ValueError(f"Required service '{svc_name}' has missing dependency '{dep}'.")
                    if included[dep]["state"] == "disabled":
                         raise ValueError(f"Required service '{svc_name}' depends on disabled service '{dep}'.")

                    if included[dep]["state"] != "required":
                         included[dep]["state"] = "required"
                         changed = True

    def check_cycle(node, visited, rec_stack):
        visited.add(node)
        rec_stack.add(node)

        deps = included[node]["info"].get("dependencies", [])
        for neighbor in deps:
             if neighbor in included:
                 if neighbor not in visited:
                     if check_cycle(neighbor, visited, rec_stack):
                         return True
                 elif neighbor in rec_stack:
                     return True
        rec_stack.remove(node)
        return False

    visited = set()
    rec_stack = set()
    for svc_name in included:
        if svc_name not in visited:
            if check_cycle(svc_name, visited, rec_stack):
                raise ValueError(f"Dependency cycle detected involving service '{svc_name}'.")

    module_ids = {}
    for svc_name, svc_data in included.items():
        mod_id = svc_data["info"].get("module_id")
        if mod_id:
            if mod_id in module_ids:
                raise ValueError(f"Duplicate module_id '{mod_id}' found in services '{svc_name}' and '{module_ids[mod_id]}'")
            module_ids[mod_id] = svc_name

    graph_str = json.dumps(included, sort_keys=True)
    graph_hash = hashlib.sha256(graph_str.encode("utf-8")).hexdigest()

    included["_metadata"] = {"hash": graph_hash}

    return included
