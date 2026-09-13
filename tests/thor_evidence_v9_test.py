"""V9 bounded operational cycle and deterministic manifest tests."""
import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools"))
sys.path.insert(0, str(ROOT / "tests"))

from m12_carver import IntervalDB, ROM_END
from thor_evidence.orchestrator import OperationalCycle
from thor_evidence.events import write_capture
from thor_evidence_v0_test import events, header


def manifest():
    return {"schema": "oasis.full-rom-split.v1", "start": 0, "end": ROM_END,
            "rom_size": ROM_END, "rom_sha256": "fixture", "entries": [
                {"start": 0, "end": 0x100, "kind": "CODE_VERIFIED", "classification": "CODE_VERIFIED",
                 "confidence": "CONFIRMED", "source": "fixture"},
                {"start": 0x100, "end": ROM_END, "kind": "UNKNOWN", "classification": "UNKNOWN",
                 "confidence": "ROM_HASH_VERIFIED", "source": "fixture"}],
            "metrics": {"SOURCE_OWNED_BYTES": 0x100}}


def test_bounded_cycle_and_persistent_manifest():
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        capture = root / "capture.jsonl"
        write_capture(capture, header(), events())
        db = IntervalDB.from_manifest(manifest())
        cycle = OperationalCycle(root / "evidence.sqlite", db)
        try:
            trace = cycle.capture(capture)
            frontier = cycle.seed_frontier("FF13CC", 0x100, 0x140,
                                           information_gain=8, confidence=10, cost=1, risk=1,
                                           evidence_classes=("STATIC",))
            request = cycle.next_request()
            assert request["trace"] == trace and request["bounded"]
            response = cycle.static_response(request, records=[
                {"id": "range", "start": 0x110, "end": 0x120, "type": "OBSERVED_RANGE"}],
                unresolved=["access-width"])
            cycle.ingest_static(response["id"], frontier, progress=False)
            first = cycle.manifest_bytes()
            second = cycle.manifest_bytes()
            assert first == second
            assert hashlib.sha256(first).hexdigest() == hashlib.sha256(second).hexdigest()
            assert cycle.export()["ownership"]["source_owned_delta"] == 0
            assert db.source_owned_bytes() == 0x100
        finally:
            cycle.close()


def test_cli_workflow():
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        capture = root / "capture.jsonl"
        write_capture(capture, header(), events())
        output = root / "manifest.json"
        command = [sys.executable, "-m", "thor_evidence.cli", "--database", str(root / "cli.sqlite"),
                   "--capture", str(capture), "--manifest", str(output), "--start", "0x100",
                   "--end", "0x120", "--query-kind", "FF13CC"]
        completed = subprocess.run(command, cwd=ROOT, env={**__import__("os").environ,
                                     "PYTHONPATH": str(ROOT / "src/tools")}, check=False)
        assert completed.returncode == 0
        assert json.loads(output.read_text())["schema"] == "thor.evidence.v9.operational-cycle"


def main():
    test_bounded_cycle_and_persistent_manifest()
    print("PASS thor evidence v9")


if __name__ == "__main__":
    main()
