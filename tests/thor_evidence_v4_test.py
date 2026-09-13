"""V4 bounded ROM/resource/video-domain integration tests."""
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools"))
sys.path.insert(0, str(ROOT / "tests"))

from thor_evidence.events import write_capture
from thor_evidence.store import Store
from thor_evidence.v4_domains import V4DomainEngine
from thor_evidence_v0_test import events, header


TRACE = "8" * 64


def test_rom_resource_dma_and_hardware_domains():
    engine = V4DomainEngine(TRACE)
    engine.begin_epoch(1)
    rom = engine.add_root(1, "ROM_DATA", "0x3820", status="OBSERVED")
    code = engine.add_root(1, "ROM_CODE_CONSTANT", "0x3820")
    constant = engine.add_root(1, "IMMEDIATE_CONSTANT", "FF13CC")
    transform = engine.add_3820_transform(1, (rom.id,), output_size=0x1000)
    vram = engine.add_hardware_write(1, "VRAM", 0x4000, 0x20, 0x1122,
                                     "exec-1", "VDP_WRITE", (transform.id,))
    dma, sat = engine.add_dma_transfer(1, (transform.id,), "SAT", 0xD000, 0x20,
                                       "exec-2", "PROVISIONAL", ("dma-witness",))
    engine.link(code.id, "register-version-A5", "ADDRESS", "PROVISIONAL", "LEA")
    engine.link(constant.id, vram.id, "ADDRESS", "OBSERVED", "VDP_DESTINATION")
    exported = engine.export()
    assert transform.routine_pc == 0x3820
    assert dict(transform.interop)["decoder_family"] == "Ancient"
    assert vram.domain == "VRAM" and sat.domain == "SAT"
    assert dma.destination_address == 0xD000
    assert {edge["role"] for edge in exported["dependencies"]} >= {"VALUE", "ADDRESS", "EXECUTION"}
    assert engine.explain(sat.id)["target"]["domain"] == "SAT"
    assert engine.explain(sat.id)["unresolved_frontier"]


def test_domain_and_rom_guards():
    engine = V4DomainEngine(TRACE)
    engine.begin_epoch(1)
    try:
        engine.add_root(1, "ROM_DATA", "0x300000")
    except ValueError:
        pass
    else:
        raise AssertionError("ROM roots outside canonical image must fail")
    try:
        engine.add_hardware_write(1, "CRAM", 0x70, 0x20)
    except ValueError:
        pass
    else:
        raise AssertionError("hardware writes outside domain must fail")
    try:
        engine.add_dma_transfer(1, (), "VRAM", 0, 0)
    except ValueError:
        pass
    else:
        raise AssertionError("zero-length DMA must fail")


def test_sqlite_reopen_idempotence_and_conflict():
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        capture = root / "capture.jsonl"
        write_capture(capture, header(), events())
        store = Store(root / "evidence.sqlite")
        trace = store.import_capture(capture)
        engine = V4DomainEngine(trace)
        engine.begin_epoch(1)
        source = engine.add_root(1, "ROM_DATA", "0x3820")
        engine.add_3820_transform(1, (source.id,), output_size=64)
        payload = engine.export()
        store.import_v4(payload, trace)
        before = store.export_v4(trace)
        store.import_v4(payload, trace)
        assert store.export_v4(trace) == before
        conflict = dict(payload)
        conflict["roots"] = [dict(payload["roots"][0], value=7)]
        try:
            store.import_v4(conflict, trace)
        except ValueError:
            pass
        else:
            raise AssertionError("same V4 identity with different payload must reject")
        store.close()


def main():
    test_rom_resource_dma_and_hardware_domains()
    test_domain_and_rom_guards()
    test_sqlite_reopen_idempotence_and_conflict()
    print("PASS thor evidence v4")


if __name__ == "__main__":
    main()
