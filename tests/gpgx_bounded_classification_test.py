import importlib.util
from pathlib import Path


MODULE_PATH = Path(__file__).parents[1] / "src" / "tools" / "gpgx_bounded_classification.py"
SPEC = importlib.util.spec_from_file_location("gpgx_bounded_classification", MODULE_PATH)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(MODULE)


def fixtures():
    rom = bytearray(0x700000)
    raw = {
        0x060BB6: "4E71", 0x060BB8: "4E71", 0x060BBA: "4E71", 0x060BBC: "4E71",
        0x060BBE: "4E71", 0x060BC0: "4E71", 0x060BC2: "60CC",
        0x060BC4: "33FC000000A11100",
    }
    for pc, value in raw.items():
        rom[pc:pc + len(bytes.fromhex(value))] = bytes.fromhex(value)
    instructions = [{
        "address": MODULE.fmt(pc), "raw_bytes": value,
        "decoded_instruction": "fixture", "instruction_length": len(bytes.fromhex(value)),
        "status": "DECODED", "classification": "UNKNOWN",
    } for pc, value in raw.items()]
    runtime = {
        "canonical_rom_sha256": MODULE.hashlib.sha256(rom).hexdigest(),
        "source": "GPGX_MANUAL_REALTIME", "evidence_type": "CODE_EXECUTED_AT_ADDRESS",
        "executed_addresses": [MODULE.fmt(pc) for pc in raw],
    }
    bounded_decoder = {"direct_control_flow": [
        {"source": MODULE.fmt(source), "target": MODULE.fmt(target)}
        for source, target, kind in MODULE.TARGET_EDGES if kind != "FALLTHROUGH"
    ]}
    explorer = {"bounded_control_pass": True, "edges": [
        {"source_pc": MODULE.fmt(source), "target": MODULE.fmt(target), "kind": kind}
        for source, target, kind in MODULE.TARGET_EDGES
    ]}
    return bytes(rom), runtime, {"instructions": instructions}, bounded_decoder, explorer, "RUNTIME_REGION_060BB6_STRUCTURALLY_UNDERSTOOD"


def test_full_decode_and_runtime_support_upgrades_only_unit():
    result = MODULE.classify_unit(*fixtures())
    assert result["decision"] == "BOUNDED_REGION_060BB6_STATIC_SUPPORTED"
    assert result["unit"]["classification_after"] == "CODE_STATIC_SUPPORTED"
    assert result["unit"]["observed_instruction_starts"] == 8
    assert result["unit"]["decoded_instruction_starts"] == 8
    assert all(item["evidence_type"] == "CODE_EXECUTED_AT_ADDRESS" for item in result["unit"]["address_level_execution_evidence"])


def test_decode_failure_prevents_upgrade():
    rom, runtime, decoded, bounded_decoder, explorer, report = fixtures()
    decoded["instructions"][0]["status"] = "DECODE_UNSUPPORTED"
    result = MODULE.classify_unit(rom, runtime, decoded, bounded_decoder, explorer, report)
    assert result["decision"] == "BOUNDED_REGION_060BB6_CLASSIFICATION_NEEDS_FIXUPS"
    assert result["unit"]["classification_after"] == "UNKNOWN"


def test_missing_runtime_preserves_previous_classification():
    rom, runtime, decoded, bounded_decoder, explorer, report = fixtures()
    runtime["executed_addresses"] = []
    result = MODULE.classify_unit(rom, runtime, decoded, bounded_decoder, explorer, report, "UNKNOWN")
    assert result["unit"]["classification_after"] == "UNKNOWN"
    assert result["decision"] == "BOUNDED_REGION_060BB6_CLASSIFICATION_NEEDS_FIXUPS"


def test_boundary_is_not_a_routine_identity():
    result = MODULE.classify_unit(*fixtures())
    assert result["unit"]["boundary_status"] == "LIKELY_INTERNAL_BLOCK"
    assert result["unit"]["routine_identity"] is None
    assert result["unit"]["whole_routine_promotion"] is False


def test_output_is_deterministic():
    args = fixtures()
    first = MODULE.classify_unit(*args)
    second = MODULE.classify_unit(*args)
    assert MODULE.json.dumps(first, sort_keys=True) == MODULE.json.dumps(second, sort_keys=True)


if __name__ == "__main__":
    for test in (test_full_decode_and_runtime_support_upgrades_only_unit,
                 test_decode_failure_prevents_upgrade,
                 test_missing_runtime_preserves_previous_classification,
                 test_boundary_is_not_a_routine_identity,
                 test_output_is_deterministic):
        test()
    print("GPGX bounded classification tests passed")
