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
          "ram_write_operation", "ram_write_output", "ram_coverage")


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
                for operation in payload.get("operations", []):
                    if operation["byte_range"] != [operation["effective_address"],
                                                    operation["effective_address"] + operation["width"] - 1]:
                        raise ValueError("RAM operation range identity mismatch")
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
                        version = next((item for item in payload.get("versions", [])
                                        if item["id"] == version_id), None)
                        if version is None or version.get("epoch") != epoch or version.get("trace") != ram_trace:
                            raise ValueError("RAM output references an inconsistent version")
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
                       coverage["epoch"] != epoch or coverage["completeness"] != "PROVEN":
                        raise ValueError("unverified or inconsistent RAM coverage")
                    row = {"certificate_id": coverage["certificate_id"], "trace_id": trace_id,
                           "epoch_no": epoch, "start_seq": coverage["start_seq"],
                           "end_seq": coverage["end_seq"], "addresses": canonical(coverage["addresses"]),
                           "evidence_hash": coverage["raw_artifact_hash"], "status": coverage["completeness"],
                           "complete": int(coverage["complete"]), "payload": canonical(coverage)}
                    self._put("ram_coverage", row, "certificate_id")
