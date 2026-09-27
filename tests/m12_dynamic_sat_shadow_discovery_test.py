import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "m12_dynamic_sat_shadow_discovery",
    ROOT / "src/tools/m12_dynamic_sat_shadow_discovery.py",
)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(MODULE)


def test_writer_classification_keeps_known_contracts_fail_closed():
    result = MODULE.classify_writers(bytes(0x300000))
    assert result["template_copy_writers"] == ["0x00A372", "0x00A37A", "0x00A4FE"]
    assert result["post_template_patch_writers"] == []
    assert result["unknown_writers"] == []


def test_runtime_callback_is_not_promoted_to_independent_writer():
    result = MODULE.runtime_writer_observations(
        {"fields": [{"write_pcs": ["0x00A372", "0x00A374", "0x00A37A"]}]}
    )
    assert result["observed_writer_pcs"] == ["0x00A372", "0x00A37A"]
    assert result["callback_aliases"][0]["normalized_static_writer"] == "0x00A372"


if __name__ == "__main__":
    test_writer_classification_keeps_known_contracts_fail_closed()
    test_runtime_callback_is_not_promoted_to_independent_writer()
    print("M12 dynamic SAT shadow discovery tests passed")
