"""Transactional temporal observation store; never calls ownership/promoter APIs."""
from pathlib import Path
import sqlite3
import json
from contextlib import nullcontext

from .events import read_capture
from .identity import ROM_SHA, STATUSES, canonical, digest, identity, location_key

TABLES = ("metadata", "environment", "scenario", "trace", "epoch", "event",
          "location", "value_version", "temporal_link", "relation", "witness",
          "operation_instance", "provenance_dependency", "ram_byte_version",
          "ram_write_operation", "ram_write_output", "ram_coverage",
          "v3_execution_instance", "v3_register_version", "v3_register_operation",
          "v3_control_fact", "v3_execution_relation", "v3_dependency",
          "v4_root", "v4_resource_transform", "v4_hardware_version",
          "v4_dma_transfer", "v4_dependency")


class Store:
    def __init__(self, path):
        self.connection = sqlite3.connect(path)
        self.connection.row_factory = sqlite3.Row
        self.connection.execute("PRAGMA foreign_keys=ON")
        self.connection.executescript(Path(__file__).with_name("schema.sql").read_text())
        with self.connection:
            self._put("metadata", {"key": "schema", "value": "thor.evidence.store.v0"}, "key")
            self._put("metadata", {"key": "rom", "value": ROM_SHA}, "key")

    def close(self):
        self.connection.close()

    def import_v3(self, payload, trace_id):
        """Persist one immutable V3 graph inside the existing sidecar transaction."""
        if payload.get("schema") != "thor.evidence.v3.graph" or payload.get("trace") != trace_id:
            raise ValueError("V3 graph identity mismatch")
        execution = payload.get("execution", {})
        register = payload.get("register", {})
        if execution.get("trace") != trace_id or register.get("trace") != trace_id:
            raise ValueError("V3 component identity mismatch")
        instances = execution.get("instances", [])
        epochs = register.get("epochs", {})
        known = {item["id"] for item in instances}
        with self.connection:
            for item in instances:
                expected = {"trace": item["trace"], "epoch": item["epoch"], "seq": item["seq"],
                            "pc": item["pc"], "rule_id": item["rule_id"]}
                if item["id"] != digest({"kind": "thor-v3-execution-instance", "value": expected}):
                    raise ValueError("V3 execution identity digest mismatch")
                epoch = int(item["epoch"])
                self.connection.execute("SELECT 1 FROM epoch WHERE trace_id=? AND number=?",
                                        (trace_id, epoch)).fetchone() or (_ for _ in ()).throw(
                                            ValueError("V3 execution epoch is not imported"))
                self._put("v3_execution_instance", {
                    "id": item["id"], "trace_id": trace_id, "epoch_no": epoch,
                    "exec_seq": item["seq"], "pc": item["pc"], "rule_id": item["rule_id"],
                    "payload": canonical(item)})
            for epoch_text, block in epochs.items():
                epoch = int(epoch_text)
                versions = block.get("versions", [])
                operations = block.get("operations", [])
                version_ids = {v["id"] for v in versions}
                operation_ids = {o["id"] for o in operations}
                for op in operations:
                    if op["epoch"] != epoch or op["trace"] != trace_id:
                        raise ValueError("V3 operation identity mismatch")
                    if op["execution_instance"] not in known:
                        raise ValueError("V3 operation execution is missing")
                    op_value = {key: op[key] for key in (
                        "trace", "epoch", "temporal_seq", "execution_instance", "pc", "rule_id",
                        "destination", "bit_offset", "bit_width", "inputs", "dependency_roles",
                        "status", "witness")}
                    if op["id"] != digest({"kind": "thor-v3-register-operation", "value": op_value}):
                        raise ValueError("V3 operation identity digest mismatch")
                    self._put("v3_register_operation", {
                        "id": op["id"], "trace_id": trace_id, "epoch_no": epoch,
                        "temporal_seq": op["temporal_seq"], "execution_instance": op["execution_instance"],
                        "pc": op["pc"], "rule_id": op["rule_id"], "destination": op["destination"],
                        "bit_offset": op["bit_offset"], "bit_width": op["bit_width"],
                        "status": op["status"], "payload": canonical(op)})
                for version in versions:
                    if version["epoch"] != epoch or version["trace"] != trace_id:
                        raise ValueError("V3 version identity mismatch")
                    if version.get("operation_id") not in operation_ids and version.get("operation_id") is not None:
                        raise ValueError("V3 version operation is missing")
                    if version.get("execution_instance") not in known and version.get("execution_instance") is not None:
                        raise ValueError("V3 version execution is missing")
                    version_value = {key: version[key] for key in (
                        "trace", "epoch", "register", "bit_offset", "bit_width", "value", "version_no",
                        "temporal_seq", "execution_instance", "operation_id", "status",
                        "previous_version_id", "dependencies", "origin")}
                    if version["id"] != digest({"kind": "thor-v3-register-version", "value": version_value}):
                        raise ValueError("V3 version identity digest mismatch")
                    self._put("v3_register_version", {
                        "id": version["id"], "trace_id": trace_id, "epoch_no": epoch,
                        "register_name": version["register"], "bit_offset": version["bit_offset"],
                        "bit_width": version["bit_width"], "value": version["value"],
                        "version_no": version["version_no"], "temporal_seq": version["temporal_seq"],
                        "execution_instance": version.get("execution_instance"),
                        "operation_id": version.get("operation_id"), "status": version["status"],
                        "previous_version_id": version.get("previous_version_id"),
                        "payload": canonical(version)})
            for relation in execution.get("relations", []):
                self._put("v3_execution_relation", {
                    "id": relation["id"], "trace_id": trace_id, "kind": relation["kind"],
                    "source_id": relation["source"], "target_id": relation["target"],
                    "status": relation["status"], "payload": canonical(relation)})
            for fact in execution.get("controls", []):
                self._put("v3_control_fact", {
                    "id": fact["id"], "trace_id": trace_id, "execution_id": fact["execution_id"],
                    "condition_rule": fact["condition_rule"], "branch_execution_id": fact["branch_execution_id"],
                    "taken": None if fact["taken"] is None else int(fact["taken"]),
                    "status": fact["status"], "payload": canonical(fact)})
            for edge in payload.get("dependencies", []):
                if edge["role"] not in {"VALUE", "ADDRESS", "CONTROL", "EXECUTION"}:
                    raise ValueError("invalid V3 dependency role")
                self._put("v3_dependency", {
                    "id": edge["id"], "trace_id": trace_id, "source_id": edge["source"],
                    "target_id": edge["target"], "role": edge["role"],
                    "rule_id": edge["rule_id"], "status": edge["status"],
                    "payload": canonical(edge)})

    def export_v3(self, trace_id):
        """Return deterministic persisted rows for reopen/idempotence checks."""
        rows = {}
        for table in ("v3_execution_instance", "v3_register_version", "v3_register_operation",
                      "v3_control_fact", "v3_execution_relation", "v3_dependency"):
            rows[table] = [dict(row) for row in self.connection.execute(
                f"SELECT * FROM {table} WHERE trace_id=? ORDER BY id", (trace_id,))]
        return rows

    def import_v4(self, payload, trace_id):
        """Persist a V4 cross-domain graph in the same transactional sidecar."""
        if payload.get("schema") != "thor.evidence.v4.graph" or payload.get("trace") != trace_id:
            raise ValueError("V4 graph identity mismatch")
        rom = payload.get("rom", {})
        if rom.get("sha256") != ROM_SHA or rom.get("size") != 3145728:
            raise ValueError("V4 ROM identity mismatch")
        epochs = set(payload.get("epochs", []))
        with self.connection:
            for epoch in epochs:
                if self.connection.execute("SELECT 1 FROM epoch WHERE trace_id=? AND number=?",
                                           (trace_id, epoch)).fetchone() is None:
                    raise ValueError("V4 epoch is not imported")
            for root in payload.get("roots", []):
                self._put("v4_root", {"id": root["id"], "trace_id": trace_id,
                    "epoch_no": root["epoch"], "kind": root["kind"], "root_key": root["key"],
                    "value": None if root["value"] is None else str(root["value"]),
                    "status": root["status"], "payload": canonical(root)})
            for transform in payload.get("transforms", []):
                self._put("v4_resource_transform", {"id": transform["id"], "trace_id": trace_id,
                    "epoch_no": transform["epoch"], "routine_pc": transform["routine_pc"],
                    "decoder_id": transform["decoder_id"], "status": transform["status"],
                    "payload": canonical(transform)})
            for version in payload.get("hardware", []):
                self._put("v4_hardware_version", {"id": version["id"], "trace_id": trace_id,
                    "epoch_no": version["epoch"], "domain": version["domain"],
                    "address": version["address"], "width": version["width"],
                    "value": version["value"], "execution_instance": version["execution_instance"],
                    "operation_id": version["operation_id"], "status": version["status"],
                    "payload": canonical(version)})
            for transfer in payload.get("dma", []):
                self._put("v4_dma_transfer", {"id": transfer["id"], "trace_id": trace_id,
                    "epoch_no": transfer["epoch"], "destination_domain": transfer["destination_domain"],
                    "destination_address": transfer["destination_address"], "length": transfer["length"],
                    "execution_instance": transfer["execution_instance"], "status": transfer["status"],
                    "payload": canonical(transfer)})
            for edge in payload.get("dependencies", []):
                if edge["role"] not in {"VALUE", "ADDRESS", "CONTROL", "EXECUTION"}:
                    raise ValueError("invalid V4 dependency role")
                self._put("v4_dependency", {"id": edge["id"], "trace_id": trace_id,
                    "source_id": edge["source"], "target_id": edge["target"],
                    "role": edge["role"], "rule_id": edge["rule_id"],
                    "status": edge["status"], "payload": canonical(edge)})

    def export_v4(self, trace_id):
        rows = {}
        for table in ("v4_root", "v4_resource_transform", "v4_hardware_version",
                      "v4_dma_transfer", "v4_dependency"):
            rows[table] = [dict(row) for row in self.connection.execute(
                f"SELECT * FROM {table} WHERE trace_id=? ORDER BY id", (trace_id,))]
        return rows

    def _put(self, table, row, key="id"):
        # All table/column identifiers are internal constants, never CLI payload.
        existing = self.connection.execute(f"SELECT * FROM {table} WHERE {key}=?", (row[key],)).fetchone()
        if existing is not None:
            if dict(existing) != row:
                raise ValueError("identity collision or incompatible database: " + table)
            return
        columns = ",".join(row)
        placeholders = ",".join("?" for _ in row)
        self.connection.execute(f"INSERT INTO {table} ({columns}) VALUES ({placeholders})", tuple(row.values()))

    def import_capture(self, path):
        header, events, trace_id, raw_hash = read_capture(path)
        env = header["environment"]
        env_id = identity("environment", env)
        scenario_id = identity("scenario", [env_id, header["scenario"]])
        with self.connection:
            self._put("environment", {"id": env_id, "payload": canonical(env)})
            self._put("scenario", {"id": scenario_id, "environment_id": env_id,
                                   "payload": canonical(header["scenario"])})
            self._put("trace", {"id": trace_id, "scenario_id": scenario_id, "schema": header["schema"]})
            for event in events:
                number = event["epoch"]
                if event["kind"] == "EPOCH_BEGIN":
                    self.connection.execute("INSERT OR IGNORE INTO epoch VALUES (?,?,?)",
                        (trace_id, number, header["scenario"]["state_sha256"]))
                event_id = identity("event", [trace_id, number, event["seq"]])
                self._put("event", {"id": event_id, "trace_id": trace_id, "epoch_no": number,
                                   "seq": event["seq"], "kind": event["kind"], "payload": canonical(event)})
                for slot, sample in enumerate(event["samples"]):
                    loc = sample["location"]
                    loc_id = location_key(env["rom_sha256"], env["map_sha256"], loc)
                    self._put("location", {"id": loc_id, "payload": canonical({
                        "rom": env["rom_sha256"], "map": env["map_sha256"], "location": loc})})
                    version_id = identity("value_version", [event_id, slot, loc_id,
                        sample["bit_offset"], sample["bit_width"]])
                    self._put("value_version", {"id": version_id, "event_id": event_id,
                        "location_id": loc_id, "slot": slot, "bit_offset": sample["bit_offset"],
                        "bit_width": sample["bit_width"], "value_hex": sample["value_hex"],
                        "status": sample["status"]})
            self.connection.execute("INSERT OR IGNORE INTO receipt VALUES (?,?,?)",
                                    (raw_hash, str(Path(path).resolve()), trace_id))
            receipt_path = Path(path).with_name(Path(path).name.replace(".sealed.jsonl", ".receipt.json"))
            if ".sealed.jsonl" in Path(path).name and receipt_path.is_file():
                receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
                if receipt.get("receipt_sha256") == env["receipt_sha256"] and \
                   receipt.get("raw_sha256") and receipt.get("status") == "COMPLETED":
                    self.connection.execute("INSERT OR IGNORE INTO receipt VALUES (?,?,?)",
                                            (receipt["raw_sha256"], str(receipt_path.resolve()), trace_id))
        return trace_id

    def add_temporal_link(self, source, target, kind="CANDIDATE_DEPENDENCY"):
        if kind != "CANDIDATE_DEPENDENCY":
            raise ValueError("no derived provenance or restore links in V0")
        rows = []
        for version in (source, target):
            row = self.connection.execute("SELECT e.trace_id,e.epoch_no,e.seq FROM value_version v "
                "JOIN event e ON e.id=v.event_id WHERE v.id=?", (version,)).fetchone()
            if row is None:
                raise ValueError("missing temporal instance")
            rows.append(row)
        if tuple(rows[0])[:2] != tuple(rows[1])[:2] or rows[0]["seq"] >= rows[1]["seq"]:
            raise ValueError("cross-epoch/trace or non-forward temporal link")
        link_id = identity("temporal_link", [source, target, kind])
        with self.connection:
            self._put("temporal_link", {"id": link_id, "source": source, "target": target,
                                       "kind": kind, "status": "UNKNOWN"})
        return link_id

    def add_relation(self, source, target, kind, context, event_id, status="UNKNOWN"):
        if status not in STATUSES or kind not in {"OBSERVED_ACCESS", "CANDIDATE_DEPENDENCY"}:
            raise ValueError("V0 cannot certify causal/static completeness")
        # The evidence must have actually sampled these exact locations; mere
        # address equality in an unrelated trace is not an observation witness.
        seen = {r[0] for r in self.connection.execute(
            "SELECT location_id FROM value_version WHERE event_id=?", (event_id,))}
        if not {source, target} <= seen:
            raise ValueError("witness does not observe relation endpoints")
        relation_id = identity("relation", [source, target, kind, context])
        with self.connection:
            self._put("relation", {"id": relation_id, "source": source, "target": target,
                                   "kind": kind, "context": canonical(context)})
            prior = self.connection.execute("SELECT status FROM witness WHERE relation_id=? AND event_id=?",
                                            (relation_id, event_id)).fetchone()
            if prior is not None and prior[0] != status:
                raise ValueError("cannot silently change witness status")
            self.connection.execute("INSERT OR IGNORE INTO witness VALUES (?,?,?)", (relation_id,event_id,status))
        return relation_id

    def export(self):
        # Physical import receipts are audit metadata, not logical observations.
        result = {"schema": "thor.evidence.export.v0"}
        for table in TABLES:
            rows = [dict(row) for row in self.connection.execute(f"SELECT * FROM {table}")]
            result[table] = sorted(rows, key=canonical)
        return canonical(result) + "\n"

    def import_provenance(self, result, trace_id):
        """Persist an engine-derived certificate atomically and idempotently.

        This table is deliberately separate from V0 candidate links.  A row can
        only be re-imported when its complete canonical payload is identical.
        """
        operations = result.get("operations", [])
        dependencies = result.get("dependencies", [])
        with self.connection:
            for item in operations:
                row = {"id": item["id"], "trace_id": trace_id, "epoch_no": item["epoch"],
                       "exec_seq": item["exec_seq"], "pc": item["pc"],
                       "rule_id": item["rule_id"], "payload": canonical(item)}
                self._put("operation_instance", row)
            for item in dependencies:
                row = {"id": item["id"], "trace_id": trace_id, "source_id": item["source"],
                       "target_id": item["target"], "role": item["role"],
                       "status": item["status"], "rule_id": item["rule_id"],
                       "witness_event_id": None if item.get("witness_event_id") is None else str(item["witness_event_id"]),
                       "payload": canonical(item)}
                self._put("provenance_dependency", row)
            if result.get("ram_engine"):
                ram_trace = result["ram_engine"].get("trace")
                if ram_trace == trace_id:
                    self.import_ram_engine(result["ram_engine"], trace_id, _transaction=False)
                else:
                    receipt = self.connection.execute(
                        "SELECT trace_id FROM receipt WHERE raw_hash=? ORDER BY source LIMIT 1",
                        (ram_trace,)).fetchone()
                    if receipt is None or receipt[0] != trace_id:
                        raise ValueError("RAM trace is not bound to imported capture")
                    self.import_ram_engine(result["ram_engine"], trace_id, _transaction=False)

    def import_ram_engine(self, result, trace_id, _transaction=True):
        """Persist one V2 RAM engine export atomically and idempotently."""
        if result.get("schema") != "thor.evidence.ram-v2":
            raise ValueError("RAM engine schema mismatch")
        ram_trace = result.get("trace")
        if ram_trace != trace_id:
            receipt = self.connection.execute(
                "SELECT trace_id FROM receipt WHERE raw_hash=? ORDER BY source LIMIT 1",
                (ram_trace,)).fetchone()
            if receipt is None or receipt[0] != trace_id:
                raise ValueError("RAM engine trace is not bound to capture")
        if not isinstance(ram_trace, str) or len(ram_trace) != 64:
            raise ValueError("invalid RAM trace identity")
        with (self.connection if _transaction else nullcontext()):
            for epoch_text, payload in result.get("epochs", {}).items():
                epoch = int(epoch_text)
                if epoch < 1 or not self.connection.execute(
                        "SELECT 1 FROM epoch WHERE trace_id=? AND number=?", (trace_id, epoch)).fetchone():
                    raise ValueError("RAM epoch is not present in capture")
                for item in payload.get("versions", []) + payload.get("operations", []):
                    if item.get("trace") != ram_trace or item.get("epoch") != epoch:
                        raise ValueError("embedded RAM identity disagrees with envelope")
                operations = payload.get("operations", [])
                versions = payload.get("versions", [])
                operation_ids = {item["id"] for item in operations}
                versions_by_id = {item["id"]: item for item in versions}
                if len(operation_ids) != len(operations) or len(versions_by_id) != len(versions):
                    raise ValueError("duplicate RAM operation/version identity")
                for operation in payload.get("operations", []):
                    width = operation.get("width")
                    address = operation.get("effective_address")
                    value = operation.get("value")
                    if width not in {1, 2, 4} or type(address) is not int or \
                            type(value) is not int or not 0 <= address <= 0xFFFFFF or \
                            address + width - 1 > 0xFFFFFF or not 0 <= value < (1 << (8 * width)):
                        raise ValueError("RAM operation shape is invalid")
                    if operation["byte_range"] != [operation["effective_address"],
                                                    operation["effective_address"] + operation["width"] - 1]:
                        raise ValueError("RAM operation range identity mismatch")
                    if len(operation.get("resulting_versions", [])) != width or \
                            len(operation.get("previous_versions", [])) != width:
                        raise ValueError("RAM operation output cardinality mismatch")
                    if type(operation.get("temporal_seq")) is not int or operation["temporal_seq"] < 0 or \
                            not operation.get("execution_instance") or not operation.get("rule_id"):
                        raise ValueError("RAM operation execution identity is invalid")
                    operation_payload = {key: operation[key] for key in (
                        "trace", "epoch", "temporal_seq", "execution_instance", "pc",
                        "rule_id", "width", "effective_address", "value", "raw_witnesses",
                        "decoded_instruction")}
                    if digest({"kind": "ram-write-operation-v2", "value": operation_payload}) != operation["id"]:
                        raise ValueError("RAM operation identity digest mismatch")
                    row = {"id": operation["id"], "trace_id": trace_id, "epoch_no": epoch,
                           "temporal_seq": operation["temporal_seq"],
                           "execution_instance": operation["execution_instance"], "pc": operation["pc"],
                           "rule_id": operation["rule_id"], "width": operation["width"],
                           "effective_address": operation["effective_address"],
                           "byte_start": operation["byte_range"][0], "byte_end": operation["byte_range"][1],
                           "payload": canonical(operation)}
                    self._put("ram_write_operation", row)
                for version in payload.get("versions", []):
                    if version.get("operation_id") is not None and not any(
                            version["operation_id"] == operation["id"]
                            for operation in payload.get("operations", [])):
                        raise ValueError("RAM version references a missing operation")
                    version_keys = ("trace", "epoch", "address", "version", "value", "origin")
                    if version.get("operation_id") is not None:
                        version_keys += ("operation_id", "temporal_seq")
                    version_payload = {key: version[key] for key in version_keys}
                    if digest({"kind": "ram-byte-version-v2", "value": version_payload}) != version["id"]:
                        raise ValueError("RAM version identity digest mismatch")
                    if type(version.get("address")) is not int or not 0 <= version["address"] <= 0xFFFFFF or \
                            type(version.get("value")) is not int or not 0 <= version["value"] <= 0xFF:
                        raise ValueError("RAM byte version shape is invalid")
                    operation_id = version.get("operation_id")
                    if operation_id is None:
                        if version.get("origin") not in {"PRE_CAPTURE_ORIGIN", "EXTERNAL_STATE",
                                                          "RESET_INITIALIZATION"} or \
                                version.get("previous_version_id") is not None:
                            raise ValueError("RAM root version semantics are invalid")
                    else:
                        operation = next((item for item in payload.get("operations", [])
                                          if item["id"] == operation_id), None)
                        if version.get("origin") != "WRITE_OPERATION" or operation is None or \
                                version.get("temporal_seq") != operation.get("temporal_seq"):
                            raise ValueError("RAM output producer semantics are invalid")
                    prior_id = version.get("previous_version_id")
                    if prior_id is not None:
                        prior = versions_by_id.get(prior_id)
                        if prior is None or prior.get("trace") != ram_trace or prior.get("epoch") != epoch or \
                                prior.get("address") != version["address"] or \
                                prior.get("temporal_seq", -1) >= version.get("temporal_seq", -1):
                            raise ValueError("RAM predecessor is missing, foreign or from the future")
                    if version.get("operation_id") is None and version.get("origin") == "WRITE_OPERATION":
                        raise ValueError("RAM write output is missing its producer")
                    row = {"id": version["id"], "trace_id": trace_id, "epoch_no": epoch,
                           "address": version["address"], "version_no": version["version"],
                           "temporal_seq": version["temporal_seq"], "value": version["value"],
                           "status": version["status"], "origin": version["origin"],
                           "operation_id": version.get("operation_id"),
                           "previous_version_id": version.get("previous_version_id"),
                           "payload": canonical(version)}
                    self._put("ram_byte_version", row)
                for operation in payload.get("operations", []):
                    for offset, version_id in enumerate(operation["resulting_versions"]):
                        version = versions_by_id.get(version_id)
                        expected_value = (operation["value"] >>
                                          (8 * (operation["width"] - offset - 1))) & 0xFF
                        if version is None or version.get("epoch") != epoch or version.get("trace") != ram_trace or \
                                version.get("operation_id") != operation["id"] or \
                                version.get("origin") != "WRITE_OPERATION" or \
                                version.get("address") != operation["effective_address"] + offset or \
                                version.get("temporal_seq") != operation["temporal_seq"] or \
                                version.get("value") != expected_value or \
                                version.get("previous_version_id") != operation["previous_versions"][offset]:
                            raise ValueError("RAM output references an inconsistent producer/address/value")
                        values = (operation["id"], version_id, operation["effective_address"] + offset,
                                  offset, operation["previous_versions"][offset])
                        prior = self.connection.execute(
                            "SELECT operation_id,version_id,address,byte_offset,previous_version_id "
                            "FROM ram_write_output WHERE operation_id=? AND address=?",
                            (operation["id"], operation["effective_address"] + offset)).fetchone()
                        if prior is not None and tuple(prior) != values:
                            raise ValueError("RAM output identity collision")
                        if prior is None:
                            self.connection.execute("INSERT INTO ram_write_output VALUES (?,?,?,?,?)", values)
                for coverage in payload.get("coverage", []):
                    required = {"certificate_id", "trace", "epoch", "start_seq", "end_seq",
                                "addresses", "raw_artifact_hash", "receipt_sha256", "decoder_id",
                                "rule_id", "execution_instances", "basis_hash", "completeness"}
                    if not required <= set(coverage) or coverage["trace"] != ram_trace or \
                       coverage["epoch"] != epoch or coverage["completeness"] != "PROVEN" or \
                       type(coverage["start_seq"]) is not int or type(coverage["end_seq"]) is not int or \
                       coverage["start_seq"] > coverage["end_seq"] or not coverage["addresses"] or \
                       any(type(address) is not int or not 0 <= address <= 0xFFFFFF
                           for address in coverage["addresses"]):
                        raise ValueError("unverified or inconsistent RAM coverage")
                    row = {"certificate_id": coverage["certificate_id"], "trace_id": trace_id,
                           "epoch_no": epoch, "start_seq": coverage["start_seq"],
                           "end_seq": coverage["end_seq"], "addresses": canonical(coverage["addresses"]),
                           "evidence_hash": coverage["raw_artifact_hash"], "status": coverage["completeness"],
                           "complete": int(coverage["complete"]), "payload": canonical(coverage)}
                    self._put("ram_coverage", row, "certificate_id")
