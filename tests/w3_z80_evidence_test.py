"""Unit tests for W3 Z80 Evidence, Cross-CPU Timeline, and Classification."""

from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from src.tools.thor_evidence.w3_z80_evidence import (
    CPU_68K,
    CPU_NONE,
    CPU_Z80,
    DOMAIN_68K_RAM,
    DOMAIN_BANKED_ROM,
    DOMAIN_PSG,
    DOMAIN_ROM,
    DOMAIN_YM2612,
    DOMAIN_Z80_RAM,
    DOMAIN_Z80_WINDOW,
    EVENT_BANK_REGISTER_CHANGE,
    EVENT_BUS_READ,
    EVENT_BUS_WRITE,
    EVENT_FRAME_BOUNDARY,
    FLAG_COMPLETE,
    FLAG_EVENT,
    FLAG_INSTRUCTION,
    RECORD_STRUCT,
    W3Record,
    Z80BankTracker,
    pack_record,
    unpack_record,
)
from src.tools.thor_evidence.w3_cross_cpu_timeline import (
    CANONICAL_TIMELINE_KEY,
    MASTER_TIME_SEMANTICS,
    MASTER_TIME_UNIT,
    CrossCpuEvent,
    CrossCpuTimeline,
    TieBreak,
)
from src.tools.thor_evidence.w3_z80_classification import (
    W3PrimitiveKind,
    classify_record,
    classify_ym2612_register_pairs,
)


def make_instruction(seq: int, pc: int, next_pc: int, cpu: int,
                     mtime: int, opcode: int, length: int) -> W3Record:
    return W3Record(
        stream_sequence=seq,
        instruction_sequence=seq,
        master_time=mtime,
        pc=pc,
        address=next_pc,
        value=opcode,
        kind_flags=FLAG_INSTRUCTION | FLAG_COMPLETE,
        cpu_id=cpu,
        length_or_width=length,
        domain=0,
        auxiliary=0,
    )


def make_bus_event(seq: int, pc: int, addr: int, val: int, cpu: int,
                   mtime: int, subtype: int, domain: int, width: int,
                   aux: int = 0) -> W3Record:
    kind = FLAG_EVENT | ((subtype & 7) << 11)
    return W3Record(
        stream_sequence=seq,
        instruction_sequence=seq,
        master_time=mtime,
        pc=pc,
        address=addr,
        value=val,
        kind_flags=kind,
        cpu_id=cpu,
        length_or_width=width,
        domain=domain,
        auxiliary=aux,
    )


def make_frame_boundary(seq: int, frame: int, mtime: int) -> W3Record:
    kind = FLAG_EVENT | ((EVENT_FRAME_BOUNDARY & 7) << 11)
    return W3Record(
        stream_sequence=seq,
        instruction_sequence=seq,
        master_time=mtime,
        pc=frame & 0xFFFFFFFF,
        address=frame >> 32,
        value=frame,
        kind_flags=kind,
        cpu_id=CPU_NONE,
        length_or_width=0,
        domain=0,
        auxiliary=0,
    )


class W3RecordCodecTest(unittest.TestCase):
    def test_record_size_is_48_bytes(self):
        self.assertEqual(RECORD_STRUCT.size, 48)

    def test_record_pack_unpack_roundtrip(self):
        rec = W3Record(
            stream_sequence=42,
            instruction_sequence=10,
            master_time=896000,
            pc=0x06134E,
            address=0x061350,
            value=0x4E71,
            kind_flags=FLAG_INSTRUCTION | FLAG_COMPLETE,
            cpu_id=CPU_68K,
            length_or_width=2,
            domain=DOMAIN_ROM,
            auxiliary=0,
        )
        data = pack_record(rec)
        self.assertEqual(len(data), 48)
        restored = unpack_record(data)
        self.assertEqual(rec, restored)

    def test_z80_instruction_bytes_decoding(self):
        # 4-byte Z80 instruction: DD CB 05 46
        val = 0x4605CBDD
        rec = make_instruction(1, 0x0100, 0x0104, CPU_Z80, 500, val, 4)
        self.assertEqual(rec.z80_instruction_bytes, b"\xdd\xcb\x05\x46")

        # 2-byte Z80 instruction: 3E 42
        rec2 = make_instruction(2, 0x0104, 0x0106, CPU_Z80, 600, 0x423E, 2)
        self.assertEqual(rec2.z80_instruction_bytes, b"\x3e\x42")

        # 1-byte Z80 NOP: 00
        rec3 = make_instruction(3, 0x0106, 0x0107, CPU_Z80, 700, 0x00, 1)
        self.assertEqual(rec3.z80_instruction_bytes, b"\x00")


class Z80BankTrackerTest(unittest.TestCase):
    def test_bank_register_shift_and_physical_resolution(self):
        tracker = Z80BankTracker(initial_base=0)
        # Genesis bank register shifts right by 1, and inserts new bit at bit 23
        # Writing bit 1:
        base1 = tracker.write_bit(1, master_time=100)
        self.assertEqual(base1, 0x800000)
        self.assertEqual(tracker.resolve_physical(0x8000), 0x800000)
        self.assertEqual(tracker.resolve_physical(0xFFFF), 0x807FFF)

        # Writing bit 0:
        base2 = tracker.write_bit(0, master_time=200)
        self.assertEqual(base2, 0x400000)
        self.assertEqual(tracker.resolve_physical(0x8100), 0x400100)

    def test_address_out_of_banked_window_raises(self):
        tracker = Z80BankTracker()
        with self.assertRaises(ValueError):
            tracker.resolve_physical(0x7FFF)


class CrossCpuTimelineTest(unittest.TestCase):
    def test_canonical_identity_constants(self):
        self.assertEqual(MASTER_TIME_SEMANTICS, "FRAME_RELATIVE")
        self.assertEqual(MASTER_TIME_UNIT, "Genesis master clock cycles")
        self.assertEqual(CANONICAL_TIMELINE_KEY, ("run_id", "epoch", "frame", "master_time"))

    def test_a_same_frame_strict_different_master_time_orders_correctly(self):
        # 68K at t=1200, Z80 at t=800, 68K at t=1500
        r1 = make_instruction(1, 0x100, 0x102, CPU_68K, 1200, 0x4E71, 2)
        r2 = make_instruction(2, 0x040, 0x041, CPU_Z80, 800, 0x00, 1)
        r3 = make_instruction(3, 0x102, 0x104, CPU_68K, 1500, 0x4E71, 2)

        timeline = CrossCpuTimeline([r1, r2, r3], run_id=1, epoch=0)
        ordered = list(timeline)
        self.assertEqual(len(ordered), 3)
        self.assertEqual(ordered[0].record.cpu_id, CPU_Z80)
        self.assertEqual(ordered[0].record.master_time, 800)
        self.assertEqual(ordered[1].record.cpu_id, CPU_68K)
        self.assertEqual(ordered[1].record.master_time, 1200)
        self.assertEqual(ordered[2].record.cpu_id, CPU_68K)
        self.assertEqual(ordered[2].record.master_time, 1500)
        self.assertTrue(timeline.is_strictly_monotonic())

    def test_b_frame_transition_orders_frame_first(self):
        # Frame N (5) has high master_time (896010)
        # Frame N+1 (6) has low master_time (30)
        fb5 = make_frame_boundary(1, 5, 0)
        r_f5 = make_instruction(2, 0x03B12C, 0x03B124, CPU_68K, 896010, 0x6600, 2)
        fb6 = make_frame_boundary(3, 6, 0)
        r_f6 = make_instruction(4, 0x000786, 0x00078B, CPU_Z80, 30, 0x2803C3, 3)

        timeline = CrossCpuTimeline([fb5, r_f5, fb6, r_f6], run_id=1, epoch=0)
        non_boundary = [e for e in timeline if not e.record.is_frame_boundary]
        self.assertEqual(len(non_boundary), 2)
        self.assertEqual(non_boundary[0].frame, 5)
        self.assertEqual(non_boundary[0].record.master_time, 896010)
        self.assertEqual(non_boundary[1].frame, 6)
        self.assertEqual(non_boundary[1].record.master_time, 30)
        self.assertTrue(non_boundary[0].strictly_precedes(non_boundary[1]))
        self.assertTrue(timeline.is_strictly_monotonic())

    def test_c_same_frame_same_master_time_unordered_and_no_causality(self):
        # Same frame (0), same master_time (1000)
        w1 = make_bus_event(1, 0x60000, 0xA00010, 0x42, CPU_68K, 1000,
                            EVENT_BUS_WRITE, DOMAIN_Z80_WINDOW, 8)
        r2 = make_bus_event(2, 0x0200, 0x0010, 0x42, CPU_Z80, 1000,
                            EVENT_BUS_READ, DOMAIN_Z80_RAM, 8)

        timeline = CrossCpuTimeline([w1, r2], run_id=1, epoch=0)
        events = list(timeline)
        self.assertEqual(len(events), 2)
        self.assertEqual(events[0].tie_break, TieBreak.SAME_MASTER_TIME_UNORDERED)
        self.assertEqual(events[1].tie_break, TieBreak.SAME_MASTER_TIME_UNORDERED)
        self.assertFalse(events[0].strictly_precedes(events[1]))
        self.assertFalse(events[1].strictly_precedes(events[0]))
        # Causality cannot be inferred from same-timestamp events
        self.assertEqual(len(timeline.find_causal_chains()), 0)

    def test_d_different_run_id_never_joined(self):
        r1 = make_instruction(1, 0x100, 0x102, CPU_68K, 100, 0x4E71, 2)
        r2 = make_instruction(2, 0x200, 0x202, CPU_68K, 200, 0x4E71, 2)
        t1 = CrossCpuTimeline([r1], run_id=1, epoch=0)
        t2 = CrossCpuTimeline([r2], run_id=2, epoch=0)

        with self.assertRaises(ValueError):
            t1.join(t2)

        e1 = t1.timeline[0]
        e2 = t2.timeline[0]
        self.assertFalse(e1.can_relate(e2))
        with self.assertRaises(ValueError):
            e1.strictly_precedes(e2)
        with self.assertRaises(ValueError):
            CrossCpuTimeline.from_events([e1, e2])

    def test_e_different_epoch_never_joined(self):
        r1 = make_instruction(1, 0x100, 0x102, CPU_68K, 100, 0x4E71, 2)
        r2 = make_instruction(2, 0x200, 0x202, CPU_68K, 200, 0x4E71, 2)
        t1 = CrossCpuTimeline([r1], run_id=1, epoch=1)
        t2 = CrossCpuTimeline([r2], run_id=1, epoch=2)

        with self.assertRaises(ValueError):
            t1.join(t2)

        e1 = t1.timeline[0]
        e2 = t2.timeline[0]
        self.assertFalse(e1.can_relate(e2))
        with self.assertRaises(ValueError):
            e1.strictly_precedes(e2)
        with self.assertRaises(ValueError):
            CrossCpuTimeline.from_events([e1, e2])

    def test_f_68k_write_z80_read_ym2612_strict_temporal_precedence(self):
        # 68K write at t=1000 -> Z80 read at t=1200 -> YM2612 write at t=1400
        w1 = make_bus_event(1, 0x60000, 0xA00010, 0x28, CPU_68K, 1000,
                            EVENT_BUS_WRITE, DOMAIN_Z80_WINDOW, 8)
        r2 = make_bus_event(2, 0x0200, 0x0010, 0x28, CPU_Z80, 1200,
                            EVENT_BUS_READ, DOMAIN_Z80_RAM, 8)
        w3 = make_bus_event(3, 0x0202, 0x4000, 0x28, CPU_Z80, 1400,
                            EVENT_BUS_WRITE, DOMAIN_YM2612, 8)

        timeline = CrossCpuTimeline([w1, r2, w3], run_id=1, epoch=0)
        chains = timeline.find_causal_chains()
        self.assertEqual(len(chains), 1)
        self.assertEqual(chains[0].write_time, 1000)
        self.assertEqual(chains[0].read_time, 1200)
        self.assertEqual(chains[0].audio_time, 1400)

        # Reversed time: write at t=1500, read at t=1200 -> rejected
        w_late = make_bus_event(1, 0x60000, 0xA00010, 0x28, CPU_68K, 1500,
                                EVENT_BUS_WRITE, DOMAIN_Z80_WINDOW, 8)
        timeline_late = CrossCpuTimeline([w_late, r2, w3], run_id=1, epoch=0)
        self.assertEqual(len(timeline_late.find_causal_chains()), 0)

    def test_g_equal_values_without_temporal_causal_proof_rejected(self):
        # Equal values (0x42) but unrelated domains: 68K writes to 68K RAM (not Z80 window)
        w_unrelated = make_bus_event(1, 0x60000, 0xFF0010, 0x42, CPU_68K, 1000,
                                     EVENT_BUS_WRITE, DOMAIN_68K_RAM, 8)
        r_z80 = make_bus_event(2, 0x0200, 0x0010, 0x42, CPU_Z80, 1200,
                               EVENT_BUS_READ, DOMAIN_Z80_RAM, 8)
        timeline1 = CrossCpuTimeline([w_unrelated, r_z80], run_id=1, epoch=0)
        self.assertEqual(len(timeline1.find_causal_chains()), 0)

        # Address mismatch within Z80 window: 68K writes to 0x0010, Z80 reads from 0x0020
        w_offset1 = make_bus_event(1, 0x60000, 0xA00010, 0x42, CPU_68K, 1000,
                                   EVENT_BUS_WRITE, DOMAIN_Z80_WINDOW, 8)
        r_offset2 = make_bus_event(2, 0x0200, 0x0020, 0x42, CPU_Z80, 1200,
                                   EVENT_BUS_READ, DOMAIN_Z80_RAM, 8)
        timeline2 = CrossCpuTimeline([w_offset1, r_offset2], run_id=1, epoch=0)
        self.assertEqual(len(timeline2.find_causal_chains()), 0)


class W3ClassificationTest(unittest.TestCase):
    def test_classifies_z80_ram_read_and_write(self):
        r_read = make_bus_event(1, 0x0100, 0x1800, 0xAB, CPU_Z80, 100,
                                EVENT_BUS_READ, DOMAIN_Z80_RAM, 8)
        c_read = classify_record(r_read)
        self.assertIsNotNone(c_read)
        self.assertEqual(c_read.kind, W3PrimitiveKind.Z80_RAM_READ)
        self.assertEqual(c_read.details["ram_offset"], 0x1800)
        self.assertEqual(c_read.details["value"], 0xAB)

        r_write = make_bus_event(2, 0x0102, 0x1800, 0xCD, CPU_Z80, 200,
                                 EVENT_BUS_WRITE, DOMAIN_Z80_RAM, 8)
        c_write = classify_record(r_write)
        self.assertIsNotNone(c_write)
        self.assertEqual(c_write.kind, W3PrimitiveKind.Z80_RAM_WRITE)

    def test_classifies_banked_rom_read_with_physical(self):
        r = make_bus_event(1, 0x0200, 0x8010, 0x55, CPU_Z80, 300,
                           EVENT_BUS_READ, DOMAIN_BANKED_ROM, 8, aux=0x090010)
        c = classify_record(r)
        self.assertIsNotNone(c)
        self.assertEqual(c.kind, W3PrimitiveKind.Z80_BANKED_ROM_READ)
        self.assertEqual(c.details["logical_address"], 0x8010)
        self.assertEqual(c.details["physical_address"], 0x090010)

    def test_classifies_ym2612_and_register_pair(self):
        addr_rec = make_bus_event(1, 0x0300, 0x4000, 0x28, CPU_Z80, 400,
                                  EVENT_BUS_WRITE, DOMAIN_YM2612, 8)
        data_rec = make_bus_event(2, 0x0302, 0x4001, 0xF0, CPU_Z80, 450,
                                  EVENT_BUS_WRITE, DOMAIN_YM2612, 8)

        c_addr = classify_record(addr_rec)
        c_data = classify_record(data_rec)
        self.assertEqual(c_addr.kind, W3PrimitiveKind.YM2612_ADDRESS_WRITE)
        self.assertEqual(c_data.kind, W3PrimitiveKind.YM2612_DATA_WRITE)

        pairs = classify_ym2612_register_pairs([addr_rec, data_rec])
        self.assertEqual(len(pairs), 1)
        self.assertEqual(pairs[0].kind, W3PrimitiveKind.YM2612_REGISTER_WRITE)
        self.assertEqual(pairs[0].details["register"], 0x28)
        self.assertEqual(pairs[0].details["value"], 0xF0)

    def test_classifies_psg_write(self):
        psg_rec = make_bus_event(1, 0x0400, 0x7F11, 0x9F, CPU_Z80, 500,
                                 EVENT_BUS_WRITE, DOMAIN_PSG, 8)
        c_psg = classify_record(psg_rec)
        self.assertEqual(c_psg.kind, W3PrimitiveKind.PSG_WRITE)
        self.assertEqual(c_psg.details["value"], 0x9F)


if __name__ == "__main__":
    unittest.main()
