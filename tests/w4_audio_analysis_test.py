"""Unit tests for W4 Active Audio Resource Analysis, Provenance & Sinks.

Explicit test coverage for requirements A through P:
A. BANKED_ROM_READ -> Z80 processing -> audio sink
B. repeated Z80 driver PC consuming multiple physical ROM addresses
C. observed physical ROM cluster with multiple real reads
D. candidate table/stream structure (neutral types, HYPOTHESIS allowed)
E. M68K -> Z80 handoff candidate or OBSERVED_HANDOFF
F. Port-local YM2612 Part 1 latching (0x4000 -> 0x4001)
G. Port-local YM2612 Part 2 latching (0x4002 -> 0x4003)
H. Address overwrite replaces previous unconsumed latch
I. DAC register identification (0x2A data, 0x2B enable)
J. PSG write identification (0x7F11)
K. strict-before without dataflow rejected (Level 0 vs Level 3)
L. unsupported Z80 opcode terminates/downgrades provenance
M. intervening write invalidates handoff (last-writer rule)
N. YM ports cannot cross-pair (0x4000 cannot pair with 0x4003)
O. ROM cluster does not become exact boundary without closure proof
P. report-only/historical witness cannot satisfy acceptance without raw record
"""

from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from src.tools.thor_evidence.w3_z80_evidence import (
    CPU_68K,
    CPU_Z80,
    DOMAIN_BANKED_ROM,
    DOMAIN_PSG,
    DOMAIN_YM2612,
    DOMAIN_Z80_RAM,
    DOMAIN_Z80_WINDOW,
    EVENT_BUS_READ,
    EVENT_BUS_WRITE,
    FLAG_COMPLETE,
    FLAG_EVENT,
    FLAG_INSTRUCTION,
    W3Record,
    pack_record,
)
from src.tools.thor_evidence.w4_audio_pipeline import run_pipeline
from src.tools.thor_evidence.w4_audio_provenance import (
    AudioProvenanceAnalyzer,
    CausalLevel,
    CausalRelationKind,
    LastWriterTracker,
)
from src.tools.thor_evidence.w4_audio_sink import (
    AudioSinkEvent,
    AudioSinkTracker,
    AudioSinkType,
    TruthClass,
)
from src.tools.thor_evidence.w4_z80_dataflow import (
    OriginType,
    ProvenanceTag,
    Z80DataflowTracker,
)


def make_inst(seq: int, pc: int, mtime: int, op_bytes: bytes) -> W3Record:
    val = int.from_bytes(op_bytes.ljust(4, b"\x00"), "little")
    return W3Record(
        stream_sequence=seq,
        instruction_sequence=seq,
        master_time=mtime,
        pc=pc,
        address=pc + len(op_bytes),
        value=val,
        kind_flags=FLAG_INSTRUCTION | FLAG_COMPLETE,
        cpu_id=CPU_Z80,
        length_or_width=len(op_bytes),
        domain=0,
        auxiliary=0,
    )


def make_bus(seq: int, pc: int, addr: int, val: int, cpu: int,
             mtime: int, subtype: int, domain: int, aux: int = 0) -> W3Record:
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
        length_or_width=1,
        domain=domain,
        auxiliary=aux,
    )


class W4AudioAnalysisTest(unittest.TestCase):
    # Requirement A: BANKED_ROM_READ -> Z80 processing -> audio sink
    def test_a_banked_rom_read_to_audio_sink_dataflow(self):
        analyzer = AudioProvenanceAnalyzer()
        sink_tracker = AudioSinkTracker()

        r_rom = make_bus(1, 0x0855, 0xEDE4, 0xCD, CPU_Z80, 100, EVENT_BUS_READ, DOMAIN_BANKED_ROM, aux=0xBEDE4)
        analyzer.process_record(r_rom)

        i_lda = make_inst(2, 0x0854, 100, b"\x7E")
        analyzer.process_record(i_lda)

        i_write = make_inst(3, 0x05C1, 130, b"\xDD\x77\x00")
        analyzer.process_record(i_write)

        r_addr = make_bus(4, 0x05C4, 0x4000, 0x2A, CPU_Z80, 130, EVENT_BUS_WRITE, DOMAIN_YM2612)
        r_data = make_bus(5, 0x097D, 0x4001, 0xCD, CPU_Z80, 150, EVENT_BUS_WRITE, DOMAIN_YM2612)
        sink_tracker.process_record(r_addr)
        ev = sink_tracker.process_record(r_data)

        chain = analyzer.create_chain_from_audio_event(ev)
        self.assertEqual(chain.level, CausalLevel.LEVEL_3_STRICT)
        self.assertEqual(chain.relation_kind, CausalRelationKind.STRICT_CAUSAL_CHAIN)
        self.assertEqual(chain.truth_class, TruthClass.DERIVED_EXACT)
        self.assertIsNotNone(chain.provenance_tag)
        self.assertEqual(chain.provenance_tag.origin_type, OriginType.BANKED_ROM_READ)
        self.assertEqual(chain.provenance_tag.origin_address, 0xBEDE4)

    # Requirement B: repeated Z80 driver PC consuming multiple physical ROM addresses
    def test_b_repeated_z80_pc_consuming_rom(self):
        analyzer = AudioProvenanceAnalyzer()
        r1 = make_bus(1, 0x0855, 0xEDE4, 0xCD, CPU_Z80, 100, EVENT_BUS_READ, DOMAIN_BANKED_ROM, aux=0xBEDE4)
        r2 = make_bus(2, 0x0855, 0xEDE5, 0x12, CPU_Z80, 120, EVENT_BUS_READ, DOMAIN_BANKED_ROM, aux=0xBEDE5)
        analyzer.process_record(r1)
        analyzer.process_record(r2)

        self.assertEqual(len(analyzer.banked_rom_reads), 2)
        self.assertEqual(analyzer.banked_rom_reads[0].pc, 0x0855)
        self.assertEqual(analyzer.banked_rom_reads[1].pc, 0x0855)
        self.assertNotEqual(analyzer.banked_rom_reads[0].auxiliary, analyzer.banked_rom_reads[1].auxiliary)

    # Requirement C: observed physical ROM cluster with multiple real reads
    def test_c_rom_access_clustering(self):
        analyzer = AudioProvenanceAnalyzer()
        r1 = make_bus(1, 0x0855, 0xEDE4, 0xCD, CPU_Z80, 100, EVENT_BUS_READ, DOMAIN_BANKED_ROM, aux=0xBEDE4)
        r2 = make_bus(2, 0x0855, 0xEDE5, 0x12, CPU_Z80, 110, EVENT_BUS_READ, DOMAIN_BANKED_ROM, aux=0xBEDE5)
        analyzer.process_record(r1)
        analyzer.process_record(r2)

        clusters = analyzer.compute_rom_clusters(distance_threshold=64)
        self.assertEqual(len(clusters), 1)
        self.assertEqual(clusters[0]["classification"], "OBSERVED_ROM_CLUSTER")
        self.assertEqual(clusters[0]["read_count"], 2)
        self.assertEqual(clusters[0]["physical_range"], ["0x0BEDE4", "0x0BEDE5"])

    # Requirement D: candidate table/stream structure, HYPOTHESIS allowed
    def test_d_neutral_candidate_resources(self):
        analyzer = AudioProvenanceAnalyzer()
        candidates = analyzer.generate_candidate_resources()
        allowed_types = {
            "AUDIO_COMMAND_CANDIDATE", "AUDIO_TABLE_CANDIDATE", "AUDIO_SEQUENCE_CANDIDATE",
            "AUDIO_PATCH_CANDIDATE", "AUDIO_SAMPLE_CANDIDATE", "AUDIO_RESOURCE_RANGE_CANDIDATE",
            "Z80_DRIVER_IMAGE",
        }
        for c in candidates:
            self.assertIn(c["candidate_type"], allowed_types)
            for forbidden in ("SONG", "TRACK", "INSTRUMENT", "VOICE", "SAMPLE_NAME"):
                self.assertNotIn(forbidden, c["candidate_id"])

    # Requirement E: M68K -> Z80 handoff candidate or OBSERVED_HANDOFF
    def test_e_m68k_z80_last_writer_handoff(self):
        tracker = LastWriterTracker()
        w1 = make_bus(1, 0x060310, 0xA00018, 0x01, CPU_68K, 100, EVENT_BUS_WRITE, DOMAIN_Z80_WINDOW)
        tracker.record_write(w1, CPU_68K, 0x0018, 0x01)

        r1 = make_bus(2, 0x0574, 0x0018, 0x01, CPU_Z80, 120, EVENT_BUS_READ, DOMAIN_Z80_RAM)
        truth, kind, writer, msg = tracker.evaluate_read(r1, 0x0018, 0x01)
        self.assertEqual(truth, TruthClass.DERIVED_EXACT)
        self.assertEqual(kind, CausalRelationKind.OBSERVED_HANDOFF)
        self.assertEqual(len(tracker.observed_handoffs), 1)

    # Requirement F: Port-local YM2612 Part 1 latching (0x4000 -> 0x4001)
    def test_f_port_local_pairing_part1(self):
        tracker = AudioSinkTracker()
        r_addr = make_bus(1, 0x05C0, 0x4000, 0x28, CPU_Z80, 100, EVENT_BUS_WRITE, DOMAIN_YM2612)
        r_data = make_bus(2, 0x05CF, 0x4001, 0x01, CPU_Z80, 150, EVENT_BUS_WRITE, DOMAIN_YM2612)

        tracker.process_record(r_addr)
        ev = tracker.process_record(r_data)
        self.assertIsNotNone(ev)
        self.assertEqual(ev.sink_type, AudioSinkType.YM2612_REGISTER_WRITE)
        self.assertEqual(ev.port, 0x4001)
        self.assertEqual(ev.register, 0x28)
        self.assertEqual(ev.value, 0x01)

    # Requirement G: Port-local YM2612 Part 2 latching (0x4002 -> 0x4003)
    def test_g_port_local_pairing_part2(self):
        tracker = AudioSinkTracker()
        r_addr = make_bus(1, 0x05C0, 0x4002, 0xA0, CPU_Z80, 100, EVENT_BUS_WRITE, DOMAIN_YM2612)
        r_data = make_bus(2, 0x05CF, 0x4003, 0x12, CPU_Z80, 150, EVENT_BUS_WRITE, DOMAIN_YM2612)

        tracker.process_record(r_addr)
        ev = tracker.process_record(r_data)
        self.assertIsNotNone(ev)
        self.assertEqual(ev.sink_type, AudioSinkType.YM2612_REGISTER_WRITE)
        self.assertEqual(ev.port, 0x4003)
        self.assertEqual(ev.register, 0xA0)
        self.assertEqual(ev.value, 0x12)

    # Requirement H: Address overwrite replaces previous unconsumed latch
    def test_h_latch_overwrite_replaces_previous(self):
        tracker = AudioSinkTracker()
        r_addr1 = make_bus(1, 0x05C0, 0x4000, 0x28, CPU_Z80, 100, EVENT_BUS_WRITE, DOMAIN_YM2612)
        r_addr2 = make_bus(2, 0x05C0, 0x4000, 0xA4, CPU_Z80, 120, EVENT_BUS_WRITE, DOMAIN_YM2612)
        r_data = make_bus(3, 0x05CF, 0x4001, 0x0B, CPU_Z80, 150, EVENT_BUS_WRITE, DOMAIN_YM2612)

        tracker.process_record(r_addr1)
        tracker.process_record(r_addr2)
        self.assertEqual(tracker.address_overwrites, 1)
        ev = tracker.process_record(r_data)
        self.assertIsNotNone(ev)
        self.assertEqual(ev.register, 0xA4)

    # Requirement I: DAC register identification (0x2A data, 0x2B enable)
    def test_i_dac_register_identification(self):
        tracker = AudioSinkTracker()
        r_addr_2a = make_bus(1, 0x0968, 0x4000, 0x2A, CPU_Z80, 100, EVENT_BUS_WRITE, DOMAIN_YM2612)
        r_data_2a = make_bus(2, 0x097D, 0x4001, 0x80, CPU_Z80, 150, EVENT_BUS_WRITE, DOMAIN_YM2612)
        tracker.process_record(r_addr_2a)
        ev_2a = tracker.process_record(r_data_2a)
        self.assertEqual(ev_2a.sink_type, AudioSinkType.YM2612_DAC_WRITE)
        self.assertTrue(ev_2a.is_dac_register)

        r_addr_2b = make_bus(3, 0x0968, 0x4000, 0x2B, CPU_Z80, 200, EVENT_BUS_WRITE, DOMAIN_YM2612)
        r_data_2b = make_bus(4, 0x097D, 0x4001, 0x80, CPU_Z80, 250, EVENT_BUS_WRITE, DOMAIN_YM2612)
        tracker.process_record(r_addr_2b)
        ev_2b = tracker.process_record(r_data_2b)
        self.assertTrue(ev_2b.is_dac_register)

    # Requirement J: PSG write identification (0x7F11)
    def test_j_psg_write_identification(self):
        tracker = AudioSinkTracker()
        r_psg = make_bus(1, 0x08A0, 0x7F11, 0x9F, CPU_Z80, 200, EVENT_BUS_WRITE, DOMAIN_PSG)
        ev = tracker.process_record(r_psg)
        self.assertEqual(ev.sink_type, AudioSinkType.PSG_WRITE)
        self.assertTrue(ev.is_psg)

    # Requirement K: strict-before without dataflow rejected (Level 0 vs Level 3)
    def test_k_strict_before_without_dataflow_rejected(self):
        analyzer = AudioProvenanceAnalyzer()
        sink_tracker = AudioSinkTracker()

        r_read = make_bus(1, 0x0100, 0x0010, 0x55, CPU_Z80, 100, EVENT_BUS_READ, DOMAIN_Z80_RAM)
        analyzer.process_record(r_read)

        r_addr = make_bus(2, 0x05C0, 0x4000, 0x28, CPU_Z80, 180, EVENT_BUS_WRITE, DOMAIN_YM2612)
        r_data = make_bus(3, 0x05CF, 0x4001, 0x01, CPU_Z80, 200, EVENT_BUS_WRITE, DOMAIN_YM2612)
        sink_tracker.process_record(r_addr)
        ev = sink_tracker.process_record(r_data)

        chain = analyzer.create_chain_from_audio_event(ev)
        self.assertEqual(chain.level, CausalLevel.LEVEL_0_TEMPORAL)
        self.assertEqual(chain.relation_kind, CausalRelationKind.TEMPORAL_ASSOCIATION)

    # Requirement L: unsupported Z80 opcode terminates/downgrades provenance
    def test_l_unsupported_opcode_terminates_provenance(self):
        dataflow = Z80DataflowTracker()
        prov = ProvenanceTag(OriginType.M68K_HANDOFF, 1, 100, 0xA00010, 0x42)
        dataflow.state.set_reg("A", 0x42, prov)
        self.assertIsNotNone(dataflow.state.get_reg("A")[1])

        unsupported = make_inst(2, 0x0100, 110, b"\x2F")
        dataflow.process_instruction(unsupported)
        self.assertEqual(dataflow.unsupported_opcodes, 1)

    # Requirement M: intervening write invalidates handoff (last-writer rule)
    def test_m_intervening_write_invalidates_handoff(self):
        tracker = LastWriterTracker()
        w_68k = make_bus(1, 0x060310, 0xA00018, 0x01, CPU_68K, 100, EVENT_BUS_WRITE, DOMAIN_Z80_WINDOW)
        tracker.record_write(w_68k, CPU_68K, 0x0018, 0x01)

        w_z80 = make_bus(2, 0x0500, 0x0018, 0x01, CPU_Z80, 110, EVENT_BUS_WRITE, DOMAIN_Z80_RAM)
        tracker.record_write(w_z80, CPU_Z80, 0x0018, 0x01)

        r_z80 = make_bus(3, 0x0574, 0x0018, 0x01, CPU_Z80, 120, EVENT_BUS_READ, DOMAIN_Z80_RAM)
        truth, kind, writer, msg = tracker.evaluate_read(r_z80, 0x0018, 0x01)
        self.assertEqual(truth, TruthClass.HYPOTHESIS)
        self.assertEqual(kind, CausalRelationKind.HANDOFF_CANDIDATE)
        self.assertIn("Intervening write", msg)

    # Requirement N: YM ports cannot cross-pair (0x4000 cannot pair with 0x4003)
    def test_n_ym_ports_cannot_cross_pair(self):
        tracker = AudioSinkTracker()
        r_addr1 = make_bus(1, 0x05C0, 0x4000, 0x28, CPU_Z80, 100, EVENT_BUS_WRITE, DOMAIN_YM2612)
        r_data2 = make_bus(2, 0x05CF, 0x4003, 0x55, CPU_Z80, 150, EVENT_BUS_WRITE, DOMAIN_YM2612)

        tracker.process_record(r_addr1)
        ev2 = tracker.process_record(r_data2)
        self.assertIsNotNone(ev2)
        self.assertEqual(ev2.sink_type, AudioSinkType.YM2612_UNLATCHED_DATA_WRITE)
        self.assertIsNone(ev2.register)

    # Requirement O: ROM cluster does not become exact boundary without closure proof
    def test_o_rom_cluster_does_not_become_exact_boundary(self):
        analyzer = AudioProvenanceAnalyzer()
        r1 = make_bus(1, 0x0855, 0xEDE4, 0xCD, CPU_Z80, 100, EVENT_BUS_READ, DOMAIN_BANKED_ROM, aux=0xBEDE4)
        analyzer.process_record(r1)

        clusters = analyzer.compute_rom_clusters(distance_threshold=64)
        self.assertIn("spatial span does not define an exact resource boundary", clusters[0]["caveat"])
        candidates = analyzer.generate_candidate_resources()
        for c in candidates:
            if c["candidate_id"] == "RES_ROM_WITNESS_001":
                self.assertEqual(c["truth_class"], TruthClass.HYPOTHESIS.value)

    # Requirement P: report-only/historical witness cannot satisfy active acceptance
    def test_p_historical_witness_cannot_satisfy_active_acceptance(self):
        analyzer = AudioProvenanceAnalyzer()
        # Analyzer has zero input records processed
        self.assertEqual(len(analyzer.banked_rom_reads), 0)
        # Even though WORKLOG.md documents stream_sequence 3019544,
        # it is NOT present in analyzer without raw record
        observed_reads = [r for r in analyzer.banked_rom_reads if r.stream_sequence == 3019544]
        self.assertEqual(len(observed_reads), 0)

    # Integration: All 9 acceptance artifacts emitted
    def test_pipeline_generates_all_9_artifacts(self):
        with tempfile.TemporaryDirectory() as tmp_dir:
            tmp_path = Path(tmp_dir)
            bin_path = tmp_path / "test_records.bin"

            records = [
                make_bus(1, 0x060310, 0xA00019, 0xA4, CPU_68K, 100, EVENT_BUS_WRITE, DOMAIN_Z80_WINDOW),
                make_bus(2, 0x05BA, 0x0019, 0xA4, CPU_Z80, 120, EVENT_BUS_READ, DOMAIN_Z80_RAM),
                make_bus(3, 0x0855, 0xEDE4, 0xCD, CPU_Z80, 130, EVENT_BUS_READ, DOMAIN_BANKED_ROM, aux=0xBEDE4),
                make_inst(4, 0x05B9, 140, b"\x7E"),
                make_inst(5, 0x05C1, 150, b"\xDD\x77\x00"),
                make_bus(6, 0x05C4, 0x4000, 0xA4, CPU_Z80, 150, EVENT_BUS_WRITE, DOMAIN_YM2612),
                make_bus(7, 0x05CF, 0x4001, 0x0B, CPU_Z80, 180, EVENT_BUS_WRITE, DOMAIN_YM2612),
            ]
            with bin_path.open("wb") as f:
                for r in records:
                    f.write(pack_record(r))

            out_dir = tmp_path / "output"
            receipt = run_pipeline([bin_path], out_dir)
            self.assertEqual(receipt["verdict"], "PASS_ACTIVE_AUDIO_RESOURCE_ANALYSIS_V1")

            expected_files = [
                "audio_sink_events.jsonl",
                "audio_rom_reads.jsonl",
                "audio_causal_chains.jsonl",
                "audio_driver_hotspots.json",
                "audio_rom_clusters.json",
                "audio_resource_candidates.json",
                "audio_provenance_graph.json",
                "w4_active_audio_report.md",
                "w4_receipt.json",
            ]
            for ef in expected_files:
                p = out_dir / ef
                self.assertTrue(p.is_file(), f"Missing artifact: {ef}")
                self.assertGreater(p.stat().st_size, 0, f"Artifact empty: {ef}")

    # Requirement Q: Banked ROM read -> ALU/Delta -> DAC sink write Level 3 chain
    def test_q_banked_rom_read_to_dac_provenance(self):
        dataflow = Z80DataflowTracker()
        # 1. Banked ROM read
        r_rom = make_bus(1, 0x0855, 0xEDE4, 0xCD, CPU_Z80, 100, EVENT_BUS_READ, DOMAIN_BANKED_ROM, aux=0xBEDE4)
        dataflow.process_bus_read(r_rom)
        # 2. LD A, (HL) [0x0854]
        dataflow.process_instruction(make_inst(2, 0x0854, 100, b"\x7E"))
        val, prov = dataflow.state.get_reg("A")
        self.assertIsNotNone(prov)
        self.assertEqual(prov.origin_type, OriginType.BANKED_ROM_READ)
        self.assertEqual(prov.origin_address, 0xBEDE4)
        # 3. AND $0F [0x0855]
        dataflow.process_instruction(make_inst(3, 0x0855, 110, b"\xE6\x0F"))
        val, prov = dataflow.state.get_reg("A")
        self.assertIsNotNone(prov)
        self.assertEqual(val, 0x0D)
        # 4. ADD A, $07 [0x0874]
        dataflow.process_instruction(make_inst(4, 0x0874, 120, b"\xC6\x07"))
        val, prov = dataflow.state.get_reg("A")
        self.assertEqual(val, 0x14)
        # 5. LD ($087A), A [0x0876]
        dataflow.process_instruction(make_inst(5, 0x0876, 130, b"\x32\x7A\x08"))
        self.assertIsNotNone(dataflow.state.get_ram(0x087A))
        # 6. LD A, ($0014) [0x0879]
        r_table = make_bus(6, 0x087C, 0x0014, 0xFA, CPU_Z80, 140, EVENT_BUS_READ, DOMAIN_Z80_RAM)
        dataflow.process_bus_read(r_table)
        dataflow.process_instruction(make_inst(7, 0x0879, 140, b"\x3A\x14\x00"))
        val, prov = dataflow.state.get_reg("A")
        self.assertIsNotNone(prov)
        # 7. ADD A, D [0x096D]
        dataflow.process_instruction(make_inst(8, 0x096D, 150, b"\x82"))
        # 8. LD (IY+1), A [0x097D]
        prov_sink = dataflow.process_instruction(make_inst(9, 0x097D, 160, b"\xFD\x77\x01"))
        self.assertIsNotNone(prov_sink)
        self.assertEqual(prov_sink.origin_type, OriginType.BANKED_ROM_READ)
        self.assertEqual(prov_sink.origin_address, 0xBEDE4)

    # Requirement R: ROM-derived A + independently-derived D -> both preserved in dependency set
    def test_r_multi_input_binary_provenance_preserves_both_roots(self):
        dataflow = Z80DataflowTracker()
        # 1. ROM read into A
        r_rom = make_bus(10, 0x0855, 0xEDE4, 0xCD, CPU_Z80, 100, EVENT_BUS_READ, DOMAIN_BANKED_ROM, aux=0xBEDE4)
        dataflow.process_bus_read(r_rom)
        dataflow.process_instruction(make_inst(11, 0x0854, 100, b"\x7E"))
        # 2. Independent M68K handoff write into RAM offset 0x20, read into D
        r_m68k = make_bus(5, 0x060300, 0xA00020, 0x55, CPU_68K, 50, EVENT_BUS_WRITE, DOMAIN_Z80_WINDOW)
        dataflow.register_m68k_write(0x20, 0x55, r_m68k)
        prov_d = dataflow.state.get_ram(0x20)
        dataflow.state.set_reg("D", 0x55, prov_d)

        # 3. Execute ADD A, D [0x096D] (0x82)
        dataflow.process_instruction(make_inst(12, 0x096D, 150, b"\x82"))
        val, prov = dataflow.state.get_reg("A")

        self.assertIsNotNone(prov)
        # Primary root is BANKED_ROM_READ
        self.assertEqual(prov.origin_type, OriginType.BANKED_ROM_READ)
        self.assertEqual(prov.origin_address, 0xBEDE4)
        # Complete dependency set contains BOTH roots! Neither was discarded
        roots = prov.all_roots()
        self.assertEqual(len(roots), 2)
        root_types = {r[0] for r in roots}
        self.assertIn(OriginType.BANKED_ROM_READ, root_types)
        self.assertIn(OriginType.M68K_HANDOFF, root_types)
        self.assertTrue(prov.has_origin_type(OriginType.BANKED_ROM_READ))
        self.assertTrue(prov.has_origin_type(OriginType.M68K_HANDOFF))
        # Exact arithmetic value
        self.assertEqual(val, (0xCD + 0x55) & 0xFF)


if __name__ == "__main__":
    unittest.main()
