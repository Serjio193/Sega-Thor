"""V5 static contract and non-owning Carver bridge tests."""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools"))

from m12_carver import IntervalDB, ROM_END
from thor_evidence.static_bridge import StaticBridge


def manifest():
    return {"schema": "oasis.full-rom-split.v1", "start": 0, "end": ROM_END,
            "rom_size": ROM_END, "rom_sha256": "fixture", "entries": [
                {"start": 0, "end": 0x100, "kind": "CODE_VERIFIED", "classification": "CODE_VERIFIED",
                 "confidence": "CONFIRMED", "source": "fixture"},
                {"start": 0x100, "end": ROM_END, "kind": "UNKNOWN", "classification": "UNKNOWN",
                 "confidence": "ROM_HASH_VERIFIED", "source": "fixture"}],
            "metrics": {"SOURCE_OWNED_BYTES": 0x100}}


def test_request_response_and_non_owning_merge():
    bridge = StaticBridge("7" * 64)
    request = bridge.request_from_runtime("runtime-dma-1", 0x100, 0x140, "RUNTIME_TO_STATIC")
    response = bridge.response(request, records=[
        {"id": "static-ref", "start": 0x110, "end": 0x120,
         "type": "CODE_TO_ROM_RANGE", "source_pc": 0x3820}],
        certificates=[{"certificate_type": "BOUNDARY", "start": 0x110, "end": 0x120,
                       "status": "PROVISIONAL", "rule_id": "EXACT_STATIC_BOUNDARY"}],
        unresolved=["indirect-alias"])
    db = IntervalDB.from_manifest(manifest())
    before = db.source_owned_bytes()
    merged = bridge.merge_to_carver(db, response["id"])
    assert merged["source_owned_before"] == before == merged["source_owned_after"]
    assert db.source_owned_bytes() == 0x100
    exported = bridge.carver_export(db)
    assert exported["schema"] == "thor.evidence.v5.carver-export"
    assert exported["gap_report"]["gaps"]
    assert response["unresolved"] == ["indirect-alias"]


def test_static_contract_guards_and_determinism():
    bridge = StaticBridge("6" * 64)
    request = bridge.request(0x200, 0x240, "CODE_TO_ROM_RANGE", ("seed",))
    try:
        bridge.response(request, records=[{"start": 0x1FF, "end": 0x210}])
    except ValueError:
        pass
    else:
        raise AssertionError("static records must remain inside request")
    try:
        bridge.response(request, records=[{"start": 0x210, "end": 0x220,
                                          "source_owned": True}])
    except ValueError:
        pass
    else:
        raise AssertionError("ownership claims must be rejected")
    first = bridge.request(0x300, 0x320, "DOMAIN")
    second = bridge.request(0x300, 0x320, "DOMAIN")
    assert first.id == second.id


def main():
    test_request_response_and_non_owning_merge()
    test_static_contract_guards_and_determinism()
    print("PASS thor evidence v5")


if __name__ == "__main__":
    main()
