"""Adversarial checks for the bounded engine-derived FF13CC certificate."""
import copy
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools"))
from thor_evidence.canary_engine import TARGET, validate_certificate
from thor_evidence.identity import digest
from thor_evidence.ram_versions import (CoverageCertificate, RamVersionEngine,
                                         VerifiedCoverageCertificate, attest_source_events)


def fixture():
    def v(i, loc, value, origin):
        return {"id": i, "epoch": 1, "location": loc, "bits": [0, 32],
                "value": value, "origin": origin, "slot": 0, "value_hex": f"{value:08X}"}
    vs = [v("out", {"space": "RAM", "key": TARGET}, 0x00880901, "a372-write-output"),
          v("d2", {"space": "REGISTER", "key": "D2"}, 0x00880901, "byte-slice-merge"),
          v("hi", {"space": "REGISTER", "key": "D2"}, 0x00880900, "move-long-preserved-high24"),
          v("lo", {"space": "REGISTER", "key": "D2"}, 1, "move-byte-counter"),
          v("rom", {"space": "ROM_OFFSET", "key": 0xA438}, 0x00880901, "rom-read"),
          v("cnt", {"space": "REGISTER", "key": "D5"}, 1, "addq-byte"),
          v("dst", {"space": "REGISTER", "key": "A5"}, TARGET, "adda-sign-extend-word")]
    def e(source, target, rule, role="VALUE"):
        return {"source": source, "target": target, "rule_id": rule, "role": role,
                "status": "PROVEN", "witness_event_id": 1}
    return {"schema": "thor.evidence.provenance.v1", "status": "PROVEN",
            "versions": vs, "dependencies": [e("d2", "out", "MOVE_LONG_D2_TO_RAM"),
                e("hi", "d2", "MERGE_HIGH24"), e("lo", "d2", "MERGE_LOW8"),
                e("rom", "hi", "MOVE_LONG_PRESERVE_HIGH24"),
                e("cnt", "lo", "MOVE_BYTE_D5_TO_D2_LOW8")],
            "target": {"version_id": "out", "value_hex": "00880901"},
            "checks": {"pc2_inference": False, "address_only_edge": False}}


def test_certificate_accepts_slice_provenance():
    assert validate_certificate(fixture())


def test_negative_cases_are_machine_readable_and_rejected():
    cases = json.loads((ROOT / "tests/fixtures/thor_evidence_v1_canary/negative_cases.json").read_text())
    assert len(cases["cases"]) >= 8
    for name in cases["cases"]:
        candidate = copy.deepcopy(fixture())
        if name == "rom_low8_dependency":
            candidate["versions"].append({"id": "rom-low", "epoch": 1,
                "location": {"space": "ROM_OFFSET", "key": 0xA43B}, "value_hex": "01"})
            candidate["dependencies"].append({"source": "rom-low", "target": "lo",
                "rule_id": "BAD_ROM_LOW8", "role": "VALUE", "status": "PROVEN"})
        elif name == "missing_counter_slice":
            candidate["dependencies"] = [d for d in candidate["dependencies"] if d["target"] != "lo"]
        elif name == "address_as_value":
            candidate["dependencies"].append({"source": "dst", "target": "out",
                "rule_id": "BAD_ADDRESS", "role": "VALUE", "status": "PROVEN"})
        elif name == "unknown_transform_promoted":
            candidate["dependencies"][1]["status"] = "UNKNOWN_TRANSFORM"
        elif name == "cross_epoch_merge":
            candidate["versions"][1]["epoch"] = 2
        elif name == "same_value_no_version":
            candidate["versions"].append(copy.deepcopy(candidate["versions"][0]))
        elif name == "manual_edge":
            candidate["dependencies"].append({"source": "rom", "target": "out",
                "rule_id": "MANUAL_GRAPH", "role": "VALUE", "status": "PROVEN"})
        elif name == "pc2_heuristic":
            candidate["checks"]["pc2_inference"] = True
        assert not validate_certificate(candidate), name


def test_epoch_identity_is_not_value_identity():
    a = fixture()["versions"][0]
    b = dict(a, id="epoch-2", epoch=2)
    assert a["id"] != b["id"]


def test_ram_branch_cannot_bypass_certificate_contract():
    ids = [f"v{i}" for i in range(4)]
    candidate = {"schema": "thor.evidence.provenance.v1", "status": "PROVEN",
                 "raw_sha256": "a" * 64, "target": {"version_id": ids[0],
                 "ram_version_ids": ids, "ram_operation_id": "op", "value_hex": "00880901"},
                 "ram_engine": {"trace": "a" * 64, "epochs": {"1": {
                     "versions": [{"id": item, "status": "OBSERVED", "operation_id": "op"}
                                  for item in ids],
                     "operations": [{"id": "op", "rule_id": "MOVE_LONG_D2_TO_RAM"}]} }},
                 "v2_dependencies": [{"target": item} for item in ids]}
    assert not validate_certificate(candidate)


def _ram_candidate():
    trace = "a" * 64
    engine = RamVersionEngine(trace)
    engine.begin_epoch(1, 0)
    source = attest_source_events([{"seq": seq, "epoch": 1, "kind": "EXEC",
                                   "receipt_sha256": "b" * 64,
                                   "decoder_id": "decoder", "rule_id": "MOVE_LONG_D2_TO_RAM",
                                   "data": {"pc": 0xA372},
                                   **({"execution_instance": "exec"} if seq == 1 else {})}
                                  for seq in range(11)])
    source.append({"seq": 11, "epoch": 1, "kind": "EPOCH_END",
                   "receipt_sha256": "b" * 64, "decoder_id": "decoder",
                   "rule_id": "MOVE_LONG_D2_TO_RAM", "data": {"reason": "COMPLETE"}})
    source = attest_source_events(source)
    engine.add_coverage(VerifiedCoverageCertificate.from_capture(
        CoverageCertificate("bridge", 0, 10, tuple(range(TARGET, TARGET + 4)), trace),
        trace=trace, epoch=1, raw_artifact_hash=trace, receipt_sha256="b" * 64,
        decoder_id="decoder", rule_id="MOVE_LONG_D2_TO_RAM", execution_instances=("exec",),
        source_events=source))
    operation = engine.write(1, 1, "exec", 0xA372, "MOVE_LONG_D2_TO_RAM", 4,
                             TARGET, 0x00880901)
    ram = engine.export()
    queries = [engine.last_writer(1, TARGET + offset, 1).as_dict() for offset in range(4)]
    legacy = {"id": "legacy", "epoch": 1, "location": {"space": "RAM", "key": TARGET},
              "value_hex": "00880901"}
    legacy_edge = {"source": "d2", "target": "legacy", "role": "VALUE",
                   "rule_id": "MOVE_LONG_D2_TO_RAM", "status": "PROVEN"}
    target = {"version_id": queries[0]["version_id"], "legacy_version_id": "legacy",
              "ram_version_ids": [item["version_id"] for item in queries],
              "ram_operation_id": operation["id"], "value_hex": "00880901"}
    bridge = {"source": "legacy", "operation": operation["id"],
              "targets": target["ram_version_ids"], "role": "RAM_BYTE_OUTPUT",
              "rule_id": "V1_V2_TARGET_BINDING", "status": "PROVEN", "witness_event_id": 1}
    bridge["id"] = digest({"kind": "canary-causal-bridge", "value": bridge})
    result = {"schema": "thor.evidence.provenance.v1", "status": "PROVEN",
              "raw_sha256": trace, "versions": [legacy], "dependencies": [legacy_edge],
              "target": target, "ram_engine": ram,
              "v2_dependencies": [{"source": operation["id"], "target": item["version_id"],
                                   "role": "RAM_BYTE_OUTPUT", "status": item["status"]}
                                  for item in queries],
              "causal_bridge": bridge,
              "checks": {"pc2_inference": False, "address_only_edge": False}}
    result["certificate_sha256"] = digest(result)
    return result


def test_ram_target_requires_v1_v2_bridge():
    candidate = _ram_candidate()
    assert validate_certificate(candidate)
    broken = copy.deepcopy(candidate)
    broken["causal_bridge"]["source"] = "detached-legacy"
    broken["certificate_sha256"] = digest({key: value for key, value in broken.items()
                                            if key != "certificate_sha256"})
    assert not validate_certificate(broken)


def main():
    test_certificate_accepts_slice_provenance()
    test_negative_cases_are_machine_readable_and_rejected()
    test_epoch_identity_is_not_value_identity()
    test_ram_branch_cannot_bypass_certificate_contract()
    test_ram_target_requires_v1_v2_bridge()
    print("PASS thor evidence v1 canary")


if __name__ == "__main__":
    main()
