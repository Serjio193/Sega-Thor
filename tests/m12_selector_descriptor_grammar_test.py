import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "m12_selector_descriptor_grammar",
    ROOT / "src/tools/m12_selector_descriptor_grammar.py",
)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_child_grammar_constants_are_finite_and_sentinel_closed():
    for selector, table in MODULE.CHILD_TABLES.items():
        expected_size = 0x0C + len(table["records"]) * 8 + 2
        assert table["end"] - table["start"] == expected_size
        assert selector in range(7)


def test_selector_xrefs_have_two_writers_and_eight_reads():
    writers = [role for _, role, _ in MODULE.SELECTOR_XREFS if role.startswith("writer_")]
    reads = [role for _, role, _ in MODULE.SELECTOR_XREFS if not role.startswith("writer_")]
    assert writers == ["writer_initializer", "writer_increment"]
    assert len(reads) == 8
    assert all(expected.endswith("FFAFAE") for _, _, expected in MODULE.SELECTOR_XREFS)


def test_parse_canonical_grammar_is_payload_free_and_fail_closed():
    rom_path = ROOT / "build/reference/Beyond Oasis (USA).bin"
    if not rom_path.exists():
        return
    catalog = MODULE.parse_grammar(rom_path.read_bytes())
    assert catalog["descriptor_table"]["record_count"] == 8
    assert len(catalog["child_tables"]) == 7
    assert catalog["selector_7_child"]["classification"] == (
        "NON_ROM_LONGWORD_ADDRESS_UNRESOLVED"
    )
    assert catalog["source_owned"]["delta_bytes"] == 0
    assert catalog["shared_relative_table_candidate"]["extent_status"].startswith(
        "PARTIAL_CANDIDATE"
    )
    assert "payload" not in catalog


if __name__ == "__main__":
    test_child_grammar_constants_are_finite_and_sentinel_closed()
    test_selector_xrefs_have_two_writers_and_eight_reads()
    test_parse_canonical_grammar_is_payload_free_and_fail_closed()
    print("M12 selector/descriptor grammar tests passed")
