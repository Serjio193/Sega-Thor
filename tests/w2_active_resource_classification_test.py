from __future__ import annotations

import importlib.util
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
TOOLS = ROOT / "tools" / "bizhawk-native-ring"
for name in ("live_forward_scaling_audit", "w2_frame_coherence", "w2_vdp_decoder", "w2_resource_classification"):
    path = TOOLS / f"{name}.py"
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    assert spec and spec.loader
    import sys
    sys.modules[name] = module
    spec.loader.exec_module(module)
CLASS = sys.modules["w2_resource_classification"]
VDP = sys.modules["w2_vdp_decoder"]


def instruction(stream: int, sequence: int, pc: int, opcode: int) -> tuple[int, ...]:
    return (stream, sequence, 0, pc, pc + 2, opcode, 1 | 2, 0, 2, 0, 0, 0)


def event(stream: int, sequence: int, pc: int, address: int, value: int,
          direction: int, width: int, domain: int) -> tuple[int, ...]:
    subtype = direction << 11
    width_code = {8: 1, 16: 2, 32: 3}[width]
    auxiliary = ((value >> 16) & 0xFFFF) | (width_code << 16) | (domain << 18)
    return (stream, sequence, 0, pc, address, value & 0xFFFF, 0x8000 | subtype, 0, width, domain, 0, auxiliary)


class ActiveResourceClassificationTest(unittest.TestCase):
    def test_primitive_classes_and_ordered_association(self) -> None:
        rows = [
            instruction(1, 1, 0x100, 0x2290),
            event(2, 1, 0x100, 0x000100, 0x12345678, 1, 32, 0),
            event(3, 1, 0x100, 0xFF13CC, 0x12345678, 2, 32, 1),
            instruction(4, 2, 0x102, 0x4E71),
            event(5, 2, 0x102, 0xFF2000, 0x55, 1, 8, 1),
            event(6, 2, 0x102, 0xA00017, 0xFF, 2, 8, 2),
            event(7, 2, 0x102, 0xC00004, 0x8164, 2, 16, 3),
        ]
        report = CLASS.classify_rows(reversed(rows), run_id=7)
        counts = report["counts"]["primitive_classes"]
        self.assertEqual(counts["ROM_DATA_READ"], 1)
        self.assertEqual(counts["RAM_WRITE"], 1)
        self.assertEqual(counts["RAM_READ"], 1)
        self.assertEqual(counts["Z80_WINDOW_WRITE"], 1)
        self.assertEqual(counts["VDP_CONTROL_WRITE"], 1)
        self.assertEqual(report["witnesses"]["SHADOW_SAT_RAM_WRITE"]["address"], 0xFF13CC)
        self.assertEqual(len(report["relations"]), 1)
        self.assertEqual(report["relations"][0]["truth"], "DERIVED_EXACT")

    def test_equal_values_without_closed_move_do_not_relate(self) -> None:
        rows = [instruction(1, 1, 0x100, 0x4E71),
                event(2, 1, 0x100, 0x1000, 7, 1, 16, 1),
                instruction(3, 2, 0x102, 0x4E71),
                event(4, 2, 0x102, 0x2000, 7, 2, 16, 1)]
        report = CLASS.classify_rows(rows)
        self.assertEqual(report["relations"], [])

    def test_vdp_register_command_target_and_complete_dma(self) -> None:
        rows = []
        stream = 1
        sequence = 1
        for register, value in ((1, 0x14), (19, 2), (20, 0), (21, 0xE6),
                                (22, 0x89), (23, 0x7F)):
            rows.extend([instruction(stream, sequence, 0x200, 0x33FC),
                         event(stream + 1, sequence, 0x200, 0xC00004,
                              0x8000 | (register << 8) | value, 2, 16, 3)])
            stream += 2
            sequence += 1
        rows.extend([instruction(stream, sequence, 0x220, 0x33FC),
                      event(stream + 1, sequence, 0x220, 0xC00004, 0x4000, 2, 16, 3),
                      instruction(stream + 2, sequence + 1, 0x222, 0x33FC),
                      event(stream + 3, sequence + 1, 0x222, 0xC00004, 0x0080, 2, 16, 3)])
        report = CLASS.classify_rows(rows)
        vdp = report["vdp"]
        self.assertEqual(vdp["register_writes"][0]["register"], 1)
        self.assertEqual(vdp["control_commands"][0]["target"], "VRAM")
        dma = vdp["dma_operations"][0]
        self.assertTrue(dma["programming_complete"])
        self.assertEqual(dma["source_byte_address"], 0xFF13CC)
        self.assertEqual(dma["length_words"], 2)
        self.assertFalse(dma["transfer_observed"])

    def test_vdp_incomplete_command_and_no_audio_semantics(self) -> None:
        rows = [instruction(1, 1, 0x300, 0x33FC),
                event(2, 1, 0x300, 0xC00004, 0x4000, 2, 16, 3),
                instruction(3, 2, 0x302, 0x4E71),
                event(4, 2, 0x302, 0xA11100, 1, 2, 8, 2)]
        report = CLASS.classify_rows(rows)
        self.assertEqual(len(report["vdp"]["incomplete_commands"]), 1)
        self.assertEqual(len(report["z80"]["audio_handoff_candidates"]), 1)
        self.assertEqual(report["z80"]["song_semantics"], "NOT_CLAIMED")
        self.assertEqual(report["z80"]["instrument_semantics"], "NOT_CLAIMED")

    def test_width_and_deterministic_hash_are_stable(self) -> None:
        rows = [instruction(1, 1, 0x400, 0x4E71),
                event(2, 1, 0x400, 0xFF0000, 0x12345678, 2, 32, 1)]
        first = CLASS.classify_rows(rows)
        second = CLASS.classify_rows(reversed(rows))
        self.assertEqual(first["raw_w1_record_bytes"], 96)
        self.assertEqual(first["raw_w1_records_sha256"], second["raw_w1_records_sha256"])
        self.assertEqual(first["counts"], second["counts"])


if __name__ == "__main__":
    unittest.main()
