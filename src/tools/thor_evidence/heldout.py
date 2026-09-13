"""V8 held-out frontier evaluation preserving unresolved capability status."""
import importlib.util
from pathlib import Path

from .identity import ROM_SHA, ROM_SIZE, canonical, digest, require_hash

FRONTIER_START = 0x03BDA6
FRONTIER_END = 0x03BF86
FRONTIER_NAME = "03BDA6-03BDD8-CONSUMER"


def _load_static_analyzer():
    path = Path(__file__).resolve().parents[1] / "m12_relative_table_analysis.py"
    spec = importlib.util.spec_from_file_location("thor_v8_relative_table", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class HeldOutEvaluation:
    """Join one real static query and one runtime receipt without promotion."""

    def __init__(self, rom_sha256=ROM_SHA, start=FRONTIER_START, end=FRONTIER_END):
        if rom_sha256 != ROM_SHA or start != FRONTIER_START or end != FRONTIER_END:
            raise ValueError("V8 requires the canonical held-out frontier")
        self.rom_sha256 = require_hash(rom_sha256)
        self.frontier_id = digest({"kind": "thor-v8-held-out", "rom": rom_sha256,
                                   "start": start, "end": end})

    def static_enumeration(self, rom):
        if len(rom) != ROM_SIZE:
            raise ValueError("held-out static query requires canonical ROM size")
        result = _load_static_analyzer().parse_relative_table(rom)
        return {"class": "STATIC_ENUMERATION", "frontier_id": self.frontier_id,
                "status": "UNKNOWN" if result["extent"]["status"] != "PROVEN" else "OBSERVED",
                "root": result["root"], "consumer_grammar": result["consumer_grammar"],
                "consumed_pointer_entries": result["consumed_pointer_entries"],
                "extent": result["extent"], "source": result["schema"]}

    def runtime_discovery(self, report):
        if not isinstance(report, dict) or report.get("rom_sha256") != self.rom_sha256:
            raise ValueError("runtime discovery ROM identity mismatch")
        required = {"schema", "scenario_id", "backend", "frames_executed",
                    "target_addresses", "target_reached", "target_frame", "target_sequence"}
        if not required.issubset(report):
            raise ValueError("runtime discovery receipt is incomplete")
        if not isinstance(report["scenario_id"], str) or not report["scenario_id"] or \
                not isinstance(report["backend"], str) or not report["backend"] or \
                type(report["frames_executed"]) is not int or report["frames_executed"] < 0 or \
                type(report["target_reached"]) is not bool or not isinstance(report["target_addresses"], list) or \
                not report["target_addresses"] or any(not isinstance(item, str) or not item
                                                       for item in report["target_addresses"]):
            raise ValueError("runtime discovery receipt shape is invalid")
        for key in ("target_frame", "target_sequence"):
            if report[key] is not None and (type(report[key]) is not int or report[key] < 0):
                raise ValueError("runtime discovery temporal witness is invalid")
        canonical({key: report[key] for key in required})
        return {"class": "RUNTIME_DISCOVERY", "frontier_id": self.frontier_id,
                "status": "OBSERVED" if report["target_reached"] else "UNKNOWN",
                "scenario_id": report["scenario_id"], "backend": report["backend"],
                "frames_executed": report["frames_executed"],
                "target_addresses": list(report["target_addresses"]),
                "target_reached": bool(report["target_reached"]),
                "target_frame": report["target_frame"],
                "target_sequence": report["target_sequence"]}

    def evaluate(self, rom, runtime_report):
        static = self.static_enumeration(rom)
        runtime = self.runtime_discovery(runtime_report)
        return {"schema": "thor.evidence.v8.held-out-evaluation", "rom_sha256": self.rom_sha256,
                "frontier": {"id": self.frontier_id, "name": FRONTIER_NAME,
                             "start": FRONTIER_START, "end": FRONTIER_END},
                "static": static, "runtime": runtime,
                "join": {"status": "UNKNOWN", "causal": False,
                         "reason": "runtime target does not close unterminated 03BDD8 stream"},
                "preserved_unknowns": ["03BDD8 termination", "complete consumer extent"],
                "ownership": {"source_owned_delta": 0, "promotion": False}}
