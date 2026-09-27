from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import unittest


ROOT = Path(__file__).resolve().parents[1]
TOOLS = ROOT / "tools" / "bizhawk-native-ring"
for name in ("live_forward_scaling_audit", "w2_frame_coherence", "w2_vdp_decoder",
             "w2_resource_classification"):
    path = TOOLS / f"{name}.py"
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    assert spec and spec.loader
    sys.modules[name] = module
    spec.loader.exec_module(module)

CLASS = sys.modules["w2_resource_classification"]
FRAME = sys.modules["w2_frame_coherence"]


def instruction(stream: int, sequence: int, pc: int, opcode: int = 0x4E71) -> tuple[int, ...]:
    return (stream, sequence, 0, pc, pc + 2, opcode, 1 | 2, 0, 2, 0, 0, 0)


def event(stream: int, sequence: int, pc: int, address: int, value: int,
          direction: int, width: int, domain: int) -> tuple[int, ...]:
    subtype = direction << 11
    auxiliary = ((value >> 16) & 0xFFFF) | ({8: 1, 16: 2, 32: 3}[width] << 16)
    auxiliary |= domain << 18
    return (stream, sequence, 0, pc, address, value & 0xFFFF, 0x8000 | subtype, 0, width, domain, 0, auxiliary)


def boundary(stream: int, frame: int) -> tuple[int, ...]:
    return (stream, 0, 0, frame & 0xFFFFFFFF, frame >> 32, 0, 0x8000 | (3 << 11), 0, 0, 0, 0, 0)


def segment(run: int, epoch: int, entry: int, exit_frame: int,
            start: int, end: int, cycle: int = 1) -> dict[str, object]:
    return {"run_id": run, "epoch": epoch, "entry_frame": entry,
            "exit_frame": exit_frame, "entry_stream_sequence": start,
            "exit_stream_sequence": end, "worker_id": 0, "cycle": cycle}


class FrameCoherentEvidenceTests(unittest.TestCase):
    def test_a_single_frame_assigns_exact_identity(self) -> None:
        rows = [instruction(1, 1, 0x100), event(2, 1, 0x100, 0xFF0000, 7, 2, 16, 1)]
        report = CLASS.classify_rows(rows, run_id=9,
            segments=[segment(9, 3, 40, 40, 1, 3)])
        self.assertEqual(report["frame_coherence"]["segments"][0]["frame_status"], "SINGLE_FRAME")
        self.assertEqual(report["instruction_accesses"][0]["events"][0]["frame"], 40)
        self.assertEqual(report["instruction_accesses"][0]["events"][0]["epoch"], 3)

    def test_b_boundary_partitions_frames(self) -> None:
        rows = [instruction(1, 1, 0x100), event(2, 1, 0x100, 0xFF0000, 7, 1, 16, 1),
                boundary(3, 6), instruction(4, 2, 0x102),
                event(5, 2, 0x102, 0xFF0000, 8, 2, 16, 1)]
        report = CLASS.classify_rows(rows, run_id=9,
            segments=[segment(9, 3, 5, 6, 1, 6)])
        self.assertEqual(report["frame_coherence"]["segments"][0]["frame_status"],
                         "MULTI_FRAME_PARTITIONED")
        frames = [item["frame"] for item in report["instruction_accesses"]]
        self.assertEqual(frames, [5, 6])

    def test_c_missing_boundary_is_unresolved(self) -> None:
        rows = [instruction(1, 1, 0x100), event(2, 1, 0x100, 0xFF0000, 7, 1, 16, 1)]
        report = CLASS.classify_rows(rows, run_id=9,
            segments=[segment(9, 3, 5, 6, 1, 3)])
        self.assertEqual(report["frame_coherence"]["segments"][0]["frame_status"],
                         "MULTI_FRAME_UNRESOLVED")
        self.assertEqual(report["witnesses"]["RAM_WRITE"], None)
        self.assertEqual(report["instruction_accesses"][0]["events"][0]["frame"], "UNRESOLVED")

    def test_d_epoch_reset_changes_identity(self) -> None:
        rows = [instruction(1, 1, 0x100), event(2, 1, 0x100, 0xFF0000, 7, 1, 16, 1),
                instruction(3, 2, 0x102), event(4, 2, 0x102, 0xFF0002, 8, 1, 16, 1)]
        report = CLASS.classify_rows(rows, run_id=9, segments=[
            segment(9, 1, 10, 10, 1, 3), segment(9, 2, 10, 10, 3, 5, 2)])
        identities = {(item["epoch"], item["frame"])
                      for item in report["instruction_accesses"]}
        self.assertEqual(identities, {(1, 10), (2, 10)})

    def test_e_run_id_is_part_of_join_key(self) -> None:
        rows = [instruction(1, 1, 0x100), event(2, 1, 0x100, 0xFF0000, 7, 1, 16, 1)]
        report = CLASS.classify_rows(rows, run_id=8,
            segments=[segment(7, 1, 10, 10, 1, 3)])
        self.assertEqual(report["instruction_accesses"][0]["events"][0]["run_id"], 7)
        self.assertNotEqual(report["run_id"], report["instruction_accesses"][0]["events"][0]["run_id"])

    def test_f_epoch_and_frame_are_not_joined_independently(self) -> None:
        first = FRAME.resolve_segment_frames([segment(1, 1, 7, 7, 1, 2)],
                                             [event(1, 1, 0x100, 0xFF0000, 1, 1, 16, 1)])
        second = FRAME.resolve_segment_frames([segment(1, 2, 7, 7, 1, 2)],
                                              [event(1, 1, 0x100, 0xFF0000, 1, 1, 16, 1)])
        self.assertNotEqual(first["by_stream"][1], second["by_stream"][1])

    def test_g_dma_inherits_exact_frame(self) -> None:
        rows = []
        stream = sequence = 1
        for register, value in ((19, 2), (20, 0), (21, 0xE6), (22, 0x89), (23, 0x7F)):
            rows.extend([instruction(stream, sequence, 0x200),
                         event(stream + 1, sequence, 0x200, 0xC00004,
                              0x8000 | (register << 8) | value, 2, 16, 3)])
            stream += 2
            sequence += 1
        rows.extend([instruction(stream, sequence, 0x220),
                      event(stream + 1, sequence, 0x220, 0xC00004, 0x4000, 2, 16, 3),
                      instruction(stream + 2, sequence + 1, 0x222),
                      event(stream + 3, sequence + 1, 0x222, 0xC00004, 0x0080, 2, 16, 3)])
        report = CLASS.classify_rows(rows, run_id=4,
            segments=[segment(4, 2, 77, 77, 1, stream + 4)])
        self.assertEqual(report["vdp"]["dma_operations"][0]["frame"], 77)

    def test_h_old_w2_semantics_remain_equal(self) -> None:
        rows = [instruction(1, 1, 0x100, 0x2290),
                event(2, 1, 0x100, 0xFF0000, 7, 1, 32, 1),
                event(3, 1, 0x100, 0xFF0002, 7, 2, 32, 1)]
        old = CLASS.classify_rows(rows)
        new = CLASS.classify_rows(rows, run_id=2,
            segments=[segment(2, 1, 20, 20, 1, 4)])
        self.assertEqual(old["counts"], new["counts"])
        self.assertEqual(old["relations"][0]["source_address"], new["relations"][0]["source_address"])
        self.assertEqual(old["vdp"], new["vdp"])

    def test_i_canonical_output_is_deterministic(self) -> None:
        rows = [instruction(1, 1, 0x100), event(2, 1, 0x100, 0xFF0000, 7, 1, 16, 1)]
        args = [segment(3, 1, 12, 12, 1, 3)]
        self.assertEqual(CLASS.canonical_json(CLASS.classify_rows(rows, 3, args)),
                         CLASS.canonical_json(CLASS.classify_rows(reversed(rows), 3, args)))


if __name__ == "__main__":
    unittest.main()
