"""Transactional temporal observation store; never calls ownership/promoter APIs."""
from pathlib import Path
import sqlite3

from .events import read_capture
from .identity import ROM_SHA, STATUSES, canonical, identity, location_key

TABLES = ("metadata", "environment", "scenario", "trace", "epoch", "event",
          "location", "value_version", "temporal_link", "relation", "witness")


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
