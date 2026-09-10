"""Regression checks for the M12 direct graphics promotion boundaries."""
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_direct_graphics_promote", ROOT / "src/tools/re_m12_direct_graphics_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    entries = [{"start": 0, "end": 0x500, "kind": "UNKNOWN",
                "source": "canonical_local_rom", "emitted_artifact_type": "blob"}]
    promoted = MODULE.split_unknown(entries, 0x100, 0x180)
    item = next(entry for entry in promoted if entry["start"] == 0x100)
    assert item["end"] == 0x180
    assert item["kind"] == "LOCAL_ROM_DERIVED_ASSET"
    assert item["classification"] == "DIRECT_GRAPHICS_STREAM"
    try:
        MODULE.split_unknown(promoted, 0x140, 0x1A0)
    except ValueError as error:
        assert "overlaps non-UNKNOWN" in str(error)
    else:
        raise AssertionError("overlapping direct stream was accepted")
    assert sum(item["end"] - item["start"] for item in MODULE.STREAMS) == 40065
    print("M12 direct graphics promotion helper tests passed")


if __name__ == "__main__":
    main()
