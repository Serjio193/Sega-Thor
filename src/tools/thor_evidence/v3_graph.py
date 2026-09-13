"""Canonical V3 graph, role-preserving export and developer explain API."""
from .identity import digest, require_hash


class V3Graph:
    def __init__(self, trace, registers, execution, ram=None):
        self.trace = require_hash(trace)
        self.registers = registers
        self.execution = execution
        self.ram = ram

    def _dependencies(self):
        edges = []
        for state in self.registers.epochs.values():
            for operation in state["operations"]:
                for source, role in zip(operation.inputs, operation.dependency_roles):
                    edges.append({"id": digest({"source": source, "target": operation.id,
                                                 "role": role}), "source": source,
                                  "target": operation.id, "role": role,
                                  "rule_id": operation.rule_id, "status": operation.status,
                                  "witness": list(operation.witness)})
                edges.append({"id": digest({"source": operation.execution_instance,
                                             "target": operation.id, "role": "EXECUTION"}),
                              "source": operation.execution_instance, "target": operation.id,
                              "role": "EXECUTION", "rule_id": operation.rule_id,
                              "status": operation.status, "witness": list(operation.witness)})
        for item in self.registers.ram_edges:
            for source, role in zip(item["source_versions"] + item["address_sources"], item["roles"]):
                edges.append({"id": digest({"source": source, "target": item["ram_operation_id"],
                                             "role": role}), "source": source,
                              "target": item["ram_operation_id"], "role": role,
                              "rule_id": "RAM_WRITE", "status": item["status"], "witness": []})
        return edges

    def export(self):
        payload = {"schema": "thor.evidence.v3.graph", "trace": self.trace,
                   "register": self.registers.export(), "execution": self.execution.export(),
                   "dependencies": self._dependencies()}
        if self.ram is not None:
            payload["ram"] = self.ram.export()
        return payload

    def explain(self, target_id):
        data = self.export()
        versions = {v["id"]: v for epoch in data["register"]["epochs"].values()
                    for v in epoch["versions"]}
        ram_versions = {}
        for block in data.get("ram", {}).get("epochs", {}).values():
            for version in block.get("versions", []):
                ram_versions[version["id"]] = version
        operations = {o["id"]: o for epoch in data["register"]["epochs"].values()
                      for o in epoch["operations"]}
        ram_operations = {o["id"]: o for block in data.get("ram", {}).get("epochs", {}).values()
                          for o in block.get("operations", [])}
        if target_id not in versions and target_id not in ram_versions:
            raise KeyError(target_id)
        target = versions.get(target_id, ram_versions[target_id])
        operation = operations.get(target.get("operation_id")) or ram_operations.get(target.get("operation_id"))
        frontier = []
        if target["status"] in {"UNKNOWN", "PROVISIONAL", "CONFLICT"}:
            frontier.append({"version_id": target_id, "status": target["status"]})
        relevant = [edge for edge in data["dependencies"]
                    if edge["target"] in {target_id, operation["id"] if operation else None}]
        frontier.extend({"source_id": edge["source"], "status": edge["status"]}
                        for edge in relevant if edge["status"] in {"UNKNOWN", "PROVISIONAL", "CONFLICT"})
        return {"target": target, "operation": operation,
                "dependencies": relevant,
                "unresolved_frontier": frontier}
