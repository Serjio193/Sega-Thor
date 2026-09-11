import importlib.util
import hashlib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_gfx_loader_census", ROOT / "src/tools/re_m12_gfx_loader_census.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_exact_direct_call_scan_and_screen_split():
    call = MODULE.CALL_BYTES
    rom = b"\0" * 4 + call + b"\0" * 2 + call + b"\0"
    screen = {
        "schema": MODULE.SCREEN_SCHEMA,
        "records": [{"start": 0x120, "uses": [{
            "group": 1, "index": 2, "descriptor": 4,
        }]}],
    }
    MODULE.ROM_SHA256 = hashlib.sha256(rom).hexdigest()
    report = MODULE.build_report(rom, screen)
    assert report["direct_call_count"] == 2
    assert report["screen_descriptor_direct_call_count"] == 1
    assert report["unmatched_direct_call_sites"] == ["0x00000A"]
    assert not report["promotion"]["performed"]


def test_screen_duplicate_use_is_retained_but_unique_count_is_exact():
    screen = {
        "schema": MODULE.SCREEN_SCHEMA,
        "records": [{"start": 0x200, "uses": [
            {"group": 0, "index": 1, "descriptor": 0x20},
            {"group": 0, "index": 2, "descriptor": 0x20},
        ]}],
    }
    uses = MODULE.screen_sites(screen)
    assert len(uses) == 2
    assert len({item["descriptor"] for item in uses}) == 1
