import hashlib
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOLS = ROOT / "src" / "tools" / "thor_evidence"
sys.path.insert(0, str(TOOLS))

from canonical_full_rom_map import CanonicalRomMap, Range
from runtime_rom_map_integration import CONTRACT_SHA256, PropertyRange, combine, read_range_export


def main():
    digest = "a" * 64
    contract_path = TOOLS / "runtime_rom_properties_contract.json"
    assert hashlib.sha256(contract_path.read_bytes()).hexdigest() == CONTRACT_SHA256
    canonical = CanonicalRomMap([
        Range(0, 4, "ASM", "STATIC_VERIFIED", 1),
        Range(4, 10, "UNKNOWN", "UNRESOLVED", 0),
    ], rom_size=10, rom_sha256=digest)
    identity = {
        "rom_sha256": digest, "rom_size": "10", "classifier_schema": "thor.rom-properties.v1",
        "contract_sha256": CONTRACT_SHA256, "core_build_id": "c" * 64, "run_id": "run-a",
        "validation_state": "VALIDATED",
    }
    runtime = [PropertyRange(0, 2, 1), PropertyRange(2, 10, 0)]
    view = combine(canonical, digest, 10, runtime, identity=identity)
    assert [(r.start, r.end_exclusive, r.runtime_properties) for r in view] == [
        (0, 2, 1), (2, 4, 0), (4, 10, 0)]
    assert sum((r.end_exclusive - r.start) * r.source_owned for r in view) == 4
    assert [r.canonical_class for r in view] == ["ASM", "ASM", "UNKNOWN"]
    assert combine(canonical, digest, 10, [PropertyRange(0, 10, 0)], identity=identity)[0].source_owned == 1

    metadata = {
        "schema": "thor.rom-property-ranges.v1", "rom_sha256": digest, "rom_size": "10",
        "classifier_schema": "thor.rom-properties.v1", "contract_sha256": CONTRACT_SHA256,
        "core_build_id": "c" * 64, "run_id": "run-a", "generation": "1",
        "capabilities": "7", "validation_state": "VALIDATED",
    }
    with tempfile.TemporaryDirectory() as directory:
        export = Path(directory) / "ranges.tsv"
        with export.open("w", encoding="ascii") as stream:
            for key, value in metadata.items():
                stream.write(f"# {key}={value}\n")
            stream.write("start\tend_exclusive\tproperty_mask\n0\t2\t1\n2\ta\t0\n")
        parsed_meta, parsed_ranges = read_range_export(export)
        assert parsed_meta == metadata
        assert combine(canonical, digest, 10, parsed_ranges, identity=parsed_meta) == view

    for bad_digest, bad_size, bad_ranges in [
        ("b" * 64, 10, runtime),
        (digest, 11, runtime),
        (digest, 10, [PropertyRange(1, 10, 0)]),
        (digest, 10, [PropertyRange(0, 10, 0x200)]),
    ]:
        try:
            combine(canonical, bad_digest, bad_size, bad_ranges, identity=identity)
        except ValueError:
            pass
        else:
            raise AssertionError("invalid runtime overlay was accepted")
    graphics = combine(canonical, digest, 10,
        [PropertyRange(0, 10, 0x100)], identity=identity)
    assert all(item.runtime_properties == 0x100 for item in graphics)
    bad_identity = dict(identity, validation_state="FAILED")
    try:
        combine(canonical, digest, 10, runtime, identity=bad_identity)
    except ValueError:
        pass
    else:
        raise AssertionError("invalid property identity was accepted")


if __name__ == "__main__":
    main()
