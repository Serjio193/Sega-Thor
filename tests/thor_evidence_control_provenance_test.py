import sys
import hashlib
from pathlib import Path
import struct
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools/thor_evidence"))
sys.path.insert(0, str(ROOT / "tools/bizhawk-native-ring"))

from auto67_control_provenance import (  # noqa: E402
    analyze_flow_segment, merge_relation_evidence, static_indirect_candidates,
)
from auto67_predecessor import (  # noqa: E402
    PredecessorCapture, PredecessorRecord, resolve as resolve_predecessor,
)
from control_provenance_audit import audit_segment  # noqa: E402
from control_provenance_campaign import micro_rom  # noqa: E402


FLOW_OK = 1 | 2


def segment_rows(rows, *, capture=1, epoch=1):
    return ({"valid": True, "ready_for_cartographer": True,
             "run_id": 7, "epoch": epoch, "worker_id": 0,
             "capture_id": capture, "generation": capture}, rows)


def instruction(sequence, pc, next_pc, opcode, flags=FLOW_OK):
    return (sequence, sequence, pc, next_pc, opcode, flags, 0)


def put(rom, pc, encoded):
    data = bytes.fromhex(encoded)
    rom[pc:pc + len(data)] = data


def direct_pointer_rom(consumer_opcode=0x4E90, target=0x300, producer=0x10,
                       source=0x400, destination_register=0):
    rom = bytearray(0x800)
    move = 0x2079 | (destination_register << 9)
    put(rom, producer, move.to_bytes(2, "big").hex() + source.to_bytes(4, "big").hex())
    put(rom, source, target.to_bytes(4, "big").hex())
    consumer = producer + 6
    put(rom, consumer, consumer_opcode.to_bytes(2, "big").hex())
    put(rom, target, "4E75")
    return rom, consumer, move


def result_for(rom, rows, capture=1):
    segment, rows = segment_rows(rows, capture=capture)
    raw = bytes(rom)
    return analyze_flow_segment(segment, rows, raw, hashlib.sha256(raw).hexdigest())


class ControlProvenanceTest(unittest.TestCase):
    def test_emulator_micro_rom_contains_real_pointer_call_and_offset_jump(self):
        rom = micro_rom()
        self.assertEqual(int.from_bytes(rom[4:8], "big"), 0x200)
        self.assertEqual(rom[0x200:0x21A], bytes.fromhex(
            "2079000005004E90207C0000030030390000051048C0D1C04ED0"))
        self.assertEqual(int.from_bytes(rom[0x500:0x504], "big"), 0x300)
        self.assertEqual(rom[0x510:0x512], bytes.fromhex("0280"))
        self.assertEqual(rom[0x580:0x586], bytes.fromhex("4EF900000200"))

    def test_memory_source_auto_update_uses_exact_pre_and_post_address(self):
        for setup, load, source_address in (
                ("207C00000500", "2258", 0x500),
                ("207C00000504", "2260", 0x500)):
            with self.subTest(load=load):
                rom = bytearray(0x800)
                put(rom, 0x10, setup)
                put(rom, 0x16, load)
                put(rom, source_address, "00000300")
                put(rom, 0x18, "4E91")
                put(rom, 0x300, "4E75")
                setup_opcode = int.from_bytes(bytes.fromhex(setup[:4]), "big")
                load_opcode = int.from_bytes(bytes.fromhex(load), "big")
                rows = [instruction(1, 0x10, 0x16, setup_opcode),
                        instruction(2, 0x16, 0x18, load_opcode),
                        instruction(3, 0x18, 0x300, 0x4E91)]
                segment, _ = segment_rows(rows)
                analysis = analyze_flow_segment(segment, rows, bytes(rom),
                                                hashlib.sha256(rom).hexdigest())
                event = analysis["consumers"][0]
                self.assertEqual(event["relation"], "OBSERVED_CODE_POINTER_TO")
                self.assertEqual(event["source"]["start"], source_address)
                self.assertEqual(audit_segment(bytes(rom), segment, rows, analysis)[
                    "audited_facts"], 1)

    def test_generic_high_address_register_and_data_index_are_resolved(self):
        rom = bytearray(0x800)
        put(rom, 0x10, "2E7C00000500")  # MOVEA.L #$500,A7
        put(rom, 0x16, "2057")          # MOVEA.L (A7),A0
        put(rom, 0x18, "4E90")          # JSR (A0)
        put(rom, 0x500, "00000300")
        put(rom, 0x300, "4E75")
        rows = [instruction(1, 0x10, 0x16, 0x2E7C),
                instruction(2, 0x16, 0x18, 0x2057),
                instruction(3, 0x18, 0x300, 0x4E90)]
        segment, _ = segment_rows(rows)
        analysis = analyze_flow_segment(segment, rows, bytes(rom),
                                        hashlib.sha256(rom).hexdigest())
        self.assertEqual(analysis["consumers"][0]["relation"],
                         "OBSERVED_CODE_POINTER_TO")
        self.assertEqual(analysis["consumers"][0]["pre_consumer_register_values"]["A0"],
                         0x300)
        self.assertEqual(audit_segment(bytes(rom), segment, rows, analysis)[
            "audited_facts"], 1)

    def test_a_direct_rom_pointer_to_jsr_is_unmodified(self):
        rom, pc, move = direct_pointer_rom()
        result = result_for(rom, [instruction(1, 0x10, pc, move),
                                  instruction(2, pc, 0x300, 0x4E90)])
        event = result["consumers"][0]
        self.assertEqual(event["relation"], "OBSERVED_CODE_POINTER_TO")
        self.assertEqual(event["classification"], "UNMODIFIED_FROM_MEMORY_READ")
        self.assertEqual(event["source"]["bytes_hex"], "00000300")
        self.assertEqual(event["producer_occurrences"][0]["instruction_sequence"], 1)
        self.assertEqual(event["consumer_occurrence"]["instruction_sequence"], 2)

    def test_b_direct_rom_pointer_to_jmp(self):
        rom, pc, move = direct_pointer_rom(consumer_opcode=0x4ED1, target=0x320,
                                           destination_register=1)
        result = result_for(rom, [instruction(1, 0x10, pc, move),
                                  instruction(2, pc, 0x320, 0x4ED1)])
        self.assertEqual(result["consumers"][0]["relation"],
                         "OBSERVED_CODE_POINTER_TO")

    def test_c_exact_long_register_copy_preserves_pointer(self):
        rom, pc, move = direct_pointer_rom(consumer_opcode=0x4E91)
        copy_pc = pc
        put(rom, copy_pc, "2248")  # MOVE.L A0,A1
        consumer_pc = copy_pc + 2
        put(rom, consumer_pc, "4E91")
        rows = [instruction(1, 0x10, copy_pc, move),
                instruction(2, copy_pc, consumer_pc, 0x2248),
                instruction(3, consumer_pc, 0x300, 0x4E91)]
        segment, _ = segment_rows(rows)
        analysis = analyze_flow_segment(segment, rows, bytes(rom),
                                        hashlib.sha256(rom).hexdigest())
        event = analysis["consumers"][0]
        self.assertEqual(event["classification"], "UNMODIFIED_FROM_MEMORY_READ")
        self.assertEqual(event["producer_occurrences"][-1]["pc"], copy_pc)
        self.assertEqual(audit_segment(bytes(rom), segment, rows, analysis)["audited_facts"], 1)
        relation = merge_relation_evidence([analysis])["relations"][0]
        self.assertEqual(len(relation["evidence"][0]["producer_occurrences"]), 2)

    def test_d_non_memory_overwrite_cannot_claim_rom_pointer(self):
        rom = bytearray(0x800)
        put(rom, 0x10, "207C00000300")  # MOVEA.L #$300,A0
        put(rom, 0x18, "4E90")
        put(rom, 0x300, "4E75")
        result = result_for(rom, [instruction(1, 0x10, 0x18, 0x207C),
                                  instruction(2, 0x18, 0x300, 0x4E90)])
        self.assertEqual(result["consumers"][0]["status"], "SOURCE_NON_MEMORY")

    def test_e_arithmetic_never_keeps_unmodified_pointer_classification(self):
        rom, pc, move = direct_pointer_rom()
        put(rom, pc, "5448")  # ADDQ #2,A0
        put(rom, pc + 2, "4E90")
        put(rom, 0x302, "4E75")
        rows = [instruction(1, 0x10, pc, move),
                instruction(2, pc, pc + 2, 0x5448),
                instruction(3, pc + 2, 0x302, 0x4E90)]
        event = result_for(rom, rows)["consumers"][0]
        self.assertNotEqual(event["classification"], "UNMODIFIED_FROM_MEMORY_READ")
        self.assertNotEqual(event.get("relation"), "OBSERVED_CODE_POINTER_TO")

    def test_f_word_offset_sign_extend_base_and_jump_is_offset(self):
        rom = bytearray(0x800)
        put(rom, 0x10, "207C00000300")  # base A0 = $300
        put(rom, 0x18, "303900000400")  # MOVE.W $400,D0
        put(rom, 0x1E, "48C0")          # EXT.L D0
        put(rom, 0x20, "D1C0")          # ADDA.L D0,A0
        put(rom, 0x22, "4ED0")          # JMP (A0)
        put(rom, 0x400, "0280")
        put(rom, 0x580, "4E75")
        rows = [instruction(1, 0x10, 0x18, 0x207C),
                instruction(2, 0x18, 0x1E, 0x3039),
                instruction(3, 0x1E, 0x20, 0x48C0),
                instruction(4, 0x20, 0x22, 0xD1C0),
                instruction(5, 0x22, 0x580, 0x4ED0)]
        event = result_for(rom, rows)["consumers"][0]
        self.assertEqual(event["relation"], "OBSERVED_CODE_OFFSET_TO")
        self.assertEqual(event["source"]["bytes_hex"], "0280")
        self.assertEqual([step["kind"] for step in event["register_transforms"]],
                         ["SIGN_EXTEND", "ADD_REGISTER_BASE"])

    def _jump_table_case(self, capture, index_displacement):
        rom = bytearray(0x900)
        put(rom, 0x10, "207C00000400")  # table base A0
        put(rom, 0x18, "247C00000400")  # code base A2
        moveq = 0x7000 | index_displacement
        put(rom, 0x20, moveq.to_bytes(2, "big").hex())
        put(rom, 0x22, "32300000")      # MOVE.W (A0,D0.W),D1
        put(rom, 0x26, "48C1")          # EXT.L D1
        put(rom, 0x28, "D5C1")          # ADDA.L D1,A2
        put(rom, 0x2A, "4ED2")          # JMP (A2)
        entry = 0x400 + index_displacement
        put(rom, entry, "0200")
        target = 0x400 + int.from_bytes(rom[entry:entry + 2], "big")
        put(rom, target, "4E75")
        rows = [instruction(1, 0x10, 0x18, 0x207C),
                instruction(2, 0x18, 0x20, 0x247C),
                instruction(3, 0x20, 0x22, moveq),
                instruction(4, 0x22, 0x26, 0x3230),
                instruction(5, 0x26, 0x28, 0x48C1),
                instruction(6, 0x28, 0x2A, 0xD5C1),
                instruction(7, 0x2A, target, 0x4ED2)]
        segment, _ = segment_rows(rows, capture=capture)
        analysis = analyze_flow_segment(segment, rows, bytes(rom),
                                        hashlib.sha256(rom).hexdigest())
        return analysis["consumers"][0], entry, bytes(rom), segment, rows

    def test_g_only_selected_jump_table_entry_is_factual(self):
        event, entry, rom, segment, rows = self._jump_table_case(7, 6)
        self.assertEqual(event["jump_table_relation"], "OBSERVED_JUMP_TABLE_ENTRY_TO")
        self.assertEqual(event["jump_table_entry"]["index"], 3)
        self.assertEqual((event["jump_table_entry"]["start"],
                          event["jump_table_entry"]["end"]), (entry, entry + 2))
        self.assertEqual(audit_segment(rom, segment, rows,
                         {"consumers": [event]})["jump_table_entries"], 1)

    def test_h_two_occurrences_produce_two_entries_only(self):
        event3, entry3, _, _, _ = self._jump_table_case(8, 6)
        event7, entry7, _, _, _ = self._jump_table_case(9, 14)
        merged = merge_relation_evidence([
            {"consumers": [event3]}, {"consumers": [event7]}])
        entries = {entry3, entry7}
        self.assertEqual(len(entries), 2)
        self.assertEqual({item["source"]["start"] for item in merged["relations"]}, entries)

    def test_i_ram_value_is_not_reconstructed_from_current_state(self):
        rom = bytearray(0x800)
        put(rom, 0x10, "207C00FF0000")  # A0 = RAM address
        put(rom, 0x18, "2250")          # MOVE.L (A0),A1
        put(rom, 0x1A, "4E91")
        rows = [instruction(1, 0x10, 0x18, 0x207C),
                instruction(2, 0x18, 0x1A, 0x2250),
                instruction(3, 0x1A, 0x300, 0x4E91)]
        result = result_for(rom, rows)
        self.assertEqual(result["consumers"][0]["status"],
                         "RAM_SOURCE_VALUE_UNRESOLVED")

    def test_j_non_rom_next_pc_never_creates_rom_relation(self):
        rom = bytearray(0x800)
        put(rom, 0x10, "207C00FF0000")
        put(rom, 0x18, "4E90")
        result = result_for(rom, [instruction(1, 0x10, 0x18, 0x207C),
                                  instruction(2, 0x18, 0xFF0000, 0x4E90)])
        self.assertEqual(result["consumers"][0]["status"], "TARGET_NOT_CANONICAL_ROM")

    def test_k_instruction_sequence_gap_fails_closed(self):
        rom, pc, move = direct_pointer_rom()
        result = result_for(rom, [instruction(1, 0x10, pc, move),
                                  instruction(3, pc, 0x300, 0x4E90)])
        self.assertEqual(result["status"], "STOP_PREDECESSOR_GAP")

    def test_l_faulted_or_incomplete_predecessor_fails_closed(self):
        rom, pc, move = direct_pointer_rom()
        result = result_for(rom, [instruction(1, 0x10, pc, move, 1),
                                  instruction(2, pc, 0x300, 0x4E90)])
        self.assertEqual(result["status"], "STOP_PREDECESSOR_TRUNCATED")

    def test_m_epoch_mismatch_in_existing_predecessor_fails_closed(self):
        records = (PredecessorRecord(0, 1, 0, 0x10, 0x49F9, {"A4": 0x300}),
                   PredecessorRecord(1, 2, 1, 0x20, 0x3955, {"A4": 0x300}))
        capture = PredecessorCapture("test", 1, 0x20, ("A4",), 1, 2,
            True, False, False, 2, 1, 0, records, 2, 4096, False, 0x20, "EXACT")
        result = resolve_predecessor(capture, bytes(0x100), ["A4"])
        self.assertEqual(result["steps"], [])

    def test_n_flow_next_pc_mismatch_stops_instead_of_guessing(self):
        rom, pc, move = direct_pointer_rom()
        result = result_for(rom, [instruction(1, 0x10, pc, move),
                                  instruction(2, pc, 0x302, 0x4E90)])
        self.assertEqual(result["status"], "STOP_CONTROL_PROVENANCE_FLOW_MISMATCH")

    def test_o_relation_dedup_preserves_multiple_occurrence_references(self):
        event_a, _, _, _, _ = self._jump_table_case(10, 6)
        event_b = dict(event_a)
        event_b["consumer_occurrence"] = dict(event_a["consumer_occurrence"], capture_id=11)
        merged = merge_relation_evidence([{"consumers": [event_a, event_b]}])
        self.assertEqual(merged["unique_relation_count"], 1)
        self.assertEqual(merged["evidence_occurrence_count"], 2)

    def test_p_unknown_transform_fails_closed(self):
        rom, pc, move = direct_pointer_rom()
        put(rom, pc, "D1C1")  # ADDA.L D1,A0: D1 has no bounded value
        put(rom, pc + 2, "4E90")
        rows = [instruction(1, 0x10, pc, move),
                instruction(2, pc, pc + 2, 0xD1C1),
                instruction(3, pc + 2, 0x300, 0x4E90)]
        event = result_for(rom, rows)["consumers"][0]
        self.assertEqual(event["status"], "SOURCE_TRANSFORMED_UNSUPPORTED")
        self.assertNotIn("relation", event)

    def test_static_census_labels_candidates_without_runtime_claim(self):
        rom = bytearray(0x100)
        put(rom, 0x20, "4E90")
        candidates = static_indirect_candidates(bytes(rom))
        self.assertEqual(candidates[0]["status"], "STATIC_CANDIDATE")
        self.assertNotIn("relation", candidates[0])

    def test_independent_auditor_reconciles_pointer_offset_and_selected_entry(self):
        rom, pc, move = direct_pointer_rom()
        rows = [instruction(1, 0x10, pc, move), instruction(2, pc, 0x300, 0x4E90)]
        segment, _ = segment_rows(rows)
        analysis = analyze_flow_segment(segment, rows, bytes(rom),
                                        hashlib.sha256(rom).hexdigest())
        self.assertEqual(audit_segment(bytes(rom), segment, rows, analysis)["audited_facts"], 1)

        rom = bytearray(0x800)
        put(rom, 0x10, "207C00000300")
        put(rom, 0x18, "303900000400")
        put(rom, 0x1E, "48C0")
        put(rom, 0x20, "D1C0")
        put(rom, 0x22, "4ED0")
        put(rom, 0x400, "0280")
        put(rom, 0x580, "4E75")
        rows = [instruction(1, 0x10, 0x18, 0x207C),
                instruction(2, 0x18, 0x1E, 0x3039),
                instruction(3, 0x1E, 0x20, 0x48C0),
                instruction(4, 0x20, 0x22, 0xD1C0),
                instruction(5, 0x22, 0x580, 0x4ED0)]
        segment, _ = segment_rows(rows)
        analysis = analyze_flow_segment(segment, rows, bytes(rom),
                                        hashlib.sha256(rom).hexdigest())
        self.assertEqual(audit_segment(bytes(rom), segment, rows, analysis)["offset_relations"], 1)

        event, _, _, _, _ = self._jump_table_case(20, 6)
        self.assertEqual(event["relation"], "OBSERVED_CODE_OFFSET_TO")

    def test_indirect_consumer_addressing_modes_use_exact_runtime_targets(self):
        cases = (
            (0x207C, "207C00000300", 0x16, "4EA80002", 0x302, "MEMORY_REGISTER"),
            (0x207C, "207C00000300", 0x18, "4EF00002", 0x312, "INDEXED_REGISTER"),
            (None, None, 0x10, "4EFA0010", 0x22, "PC_MEMORY"),
            (0x7E10, "7E10", 0x12, "4EFB7002", 0x26, "PC_INDEXED"),
        )
        for case_index, (setup_opcode, setup_bytes, consumer_pc, consumer_bytes,
                         target, mode) in enumerate(cases, 1):
            with self.subTest(mode=mode):
                rom = bytearray(0x100)
                rows = []
                seq = 1
                if setup_bytes:
                    setup_pc = 0x10
                    put(rom, setup_pc, setup_bytes)
                    rows.append(instruction(seq, setup_pc, consumer_pc, setup_opcode))
                    seq += 1
                    if setup_opcode == 0x207C and mode == "INDEXED_REGISTER":
                        put(rom, consumer_pc - 2, "7010")
                        rows[-1] = instruction(seq - 1, setup_pc, consumer_pc - 2, 0x207C)
                        rows.append(instruction(seq, consumer_pc - 2, consumer_pc, 0x7010))
                        seq += 1
                    elif setup_opcode & 0xF100 == 0x7000:
                        setup_pc = 0x10
                        consumer_pc = 0x12
                put(rom, consumer_pc, consumer_bytes)
                put(rom, target, "4E75")
                opcode = int.from_bytes(bytes.fromhex(consumer_bytes[:4]), "big")
                rows.append(instruction(seq, consumer_pc, target, opcode))
                segment, _ = segment_rows(rows, capture=case_index)
                analysis = analyze_flow_segment(segment, rows, bytes(rom),
                                                hashlib.sha256(rom).hexdigest())
                event = analysis["consumers"][0]
                self.assertEqual(event["addressing_mode"], mode)
                self.assertEqual(event["actual_next_pc"], target)
                if mode in ("PC_MEMORY", "PC_INDEXED"):
                    self.assertEqual(event["relation"], "OBSERVED_CODE_OFFSET_TO")
                    self.assertEqual(audit_segment(bytes(rom), segment, rows, analysis)[
                        "offset_relations"], 1)


if __name__ == "__main__":
    unittest.main()
