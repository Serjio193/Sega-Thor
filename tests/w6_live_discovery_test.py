"""Unit and integration tests for M12 W6 Live Discovery and Analysis Engine.

Validates:
- Canonical ROM hash and size invariants
- Bounded chunk scanning and metadata extraction
- General novelty classification (M68K, Z80, VDP, Audio, Memory)
- SOURCE_OWNED preservation (delta == 0)
- All 13 canonical artifact generation schemas
"""

import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from src.tools.thor_evidence.w3_z80_evidence import (
    CPU_68K,
    CPU_Z80,
    DOMAIN_68K_RAM,
    DOMAIN_BANKED_ROM,
    DOMAIN_ROM,
    DOMAIN_VDP,
    DOMAIN_YM2612,
    FLAG_CONTROL_FLOW,
    FLAG_INSTRUCTION,
    RECORD_STRUCT,
    W3Record,
    pack_record,
)
from src.tools.thor_evidence.w6_discovery_analysis import (
    CANONICAL_SOURCE_OWNED,
    analyze_discovery_records,
    generate_all_w6_artifacts,
    merge_intervals,
)
from src.tools.thor_evidence.w6_live_discovery import (
    CANONICAL_DEPTH,
    CANONICAL_ROM_SHA,
    CANONICAL_ROM_SIZE,
    CANONICAL_WORKERS,
    index_discovery_chunks,
    scan_raw_chunk_metadata,
    split_unified_records_into_chunks,
    verify_rom,
)


class TestW6LiveDiscovery(unittest.TestCase):
    """Test suite for W6 live discovery and analysis engine."""

    def test_canonical_constants(self) -> None:
        self.assertEqual(CANONICAL_ROM_SIZE, 3145728)
        self.assertEqual(CANONICAL_ROM_SHA, "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263")
        self.assertEqual(CANONICAL_WORKERS, 128)
        self.assertEqual(CANONICAL_DEPTH, 512)
        self.assertEqual(CANONICAL_SOURCE_OWNED, 1487388)

    def test_verify_canonical_rom(self) -> None:
        rom_path = Path("local-roms/Beyond Oasis (USA).md")
        if rom_path.is_file():
            verify_rom(rom_path)
        with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
            f.write(b"SHORT_DATA")
            tmp_path = Path(f.name)
        try:
            with self.assertRaises(ValueError):
                verify_rom(tmp_path)
        finally:
            tmp_path.unlink(missing_ok=True)

    def test_merge_intervals(self) -> None:
        self.assertEqual(merge_intervals([]), [])
        self.assertEqual(merge_intervals([(10, 20), (30, 40)]), [(10, 20), (30, 40)])
        self.assertEqual(merge_intervals([(10, 25), (20, 35)]), [(10, 35)])
        self.assertEqual(merge_intervals([(10, 30), (15, 20)]), [(10, 30)])

    def test_chunk_metadata_and_indexing(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            td = Path(tmpdir)
            rec1 = W3Record(
                stream_sequence=1,
                instruction_sequence=1,
                master_time=1000,
                pc=0x000200,
                address=0x000200,
                value=0x4E71,
                kind_flags=FLAG_INSTRUCTION,
                cpu_id=CPU_68K,
                length_or_width=2,
                domain=DOMAIN_ROM,
                auxiliary=0,
            )
            rec2 = W3Record(
                stream_sequence=2,
                instruction_sequence=2,
                master_time=2000,
                pc=0x000202,
                address=0x000202,
                value=0x4E71,
                kind_flags=FLAG_INSTRUCTION,
                cpu_id=CPU_68K,
                length_or_width=2,
                domain=DOMAIN_ROM,
                auxiliary=0,
            )
            chunk_file = td / "live-discovery-wave-000001.bin"
            chunk_file.write_bytes(pack_record(rec1) + pack_record(rec2))

            meta = scan_raw_chunk_metadata(chunk_file)
            self.assertEqual(meta["byte_size"], 96)
            self.assertEqual(meta["record_count"], 2)
            self.assertEqual(meta["stream_sequence_start"], 1)
            self.assertEqual(meta["stream_sequence_end"], 2)
            self.assertEqual(meta["master_time_start"], 1000)
            self.assertEqual(meta["master_time_end"], 2000)

            indexed = index_discovery_chunks(td)
            self.assertEqual(len(indexed), 1)
            self.assertEqual(indexed[0]["wave_index"], 1)

    def test_analysis_engine_and_all_artifacts(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            td = Path(tmpdir)
            from src.tools.thor_evidence.w3_z80_evidence import (
                EVENT_BUS_READ,
                EVENT_BUS_WRITE,
                EVENT_SUBTYPE_SHIFT,
                FLAG_EVENT,
            )
            kind_read = FLAG_EVENT | (EVENT_BUS_READ << EVENT_SUBTYPE_SHIFT)
            kind_write = FLAG_EVENT | (EVENT_BUS_WRITE << EVENT_SUBTYPE_SHIFT)

            recs = [
                W3Record(stream_sequence=1, instruction_sequence=1, master_time=100,
                         pc=0x000200, address=0x000200, value=0, kind_flags=FLAG_INSTRUCTION,
                         cpu_id=CPU_68K, length_or_width=2, domain=DOMAIN_ROM, auxiliary=0),
                W3Record(stream_sequence=2, instruction_sequence=2, master_time=200,
                         pc=0x012344, address=0x012344, value=0, kind_flags=FLAG_INSTRUCTION,
                         cpu_id=CPU_68K, length_or_width=2, domain=DOMAIN_ROM, auxiliary=0),
                W3Record(stream_sequence=3, instruction_sequence=1, master_time=300,
                         pc=0x0550, address=0x0550, value=0, kind_flags=FLAG_INSTRUCTION,
                         cpu_id=CPU_Z80, length_or_width=1, domain=DOMAIN_BANKED_ROM, auxiliary=0),
                W3Record(stream_sequence=4, instruction_sequence=0, master_time=400,
                         pc=0x0550, address=0x0BD600, value=0, kind_flags=kind_read,
                         cpu_id=CPU_Z80, length_or_width=1, domain=DOMAIN_BANKED_ROM, auxiliary=0),
                W3Record(stream_sequence=5, instruction_sequence=0, master_time=500,
                         pc=0x012344, address=0xC00004, value=0x8100, kind_flags=kind_write,
                         cpu_id=CPU_68K, length_or_width=2, domain=DOMAIN_VDP, auxiliary=0),
                W3Record(stream_sequence=6, instruction_sequence=0, master_time=600,
                         pc=0x0550, address=0x4000, value=0x2A, kind_flags=kind_write,
                         cpu_id=CPU_Z80, length_or_width=1, domain=DOMAIN_YM2612, auxiliary=0),
                W3Record(stream_sequence=7, instruction_sequence=0, master_time=601,
                         pc=0x0550, address=0x4001, value=0x80, kind_flags=kind_write,
                         cpu_id=CPU_Z80, length_or_width=1, domain=DOMAIN_YM2612, auxiliary=0),
            ]
            chunk_file = td / "live-discovery-wave-000001.bin"
            data = b"".join(pack_record(r) for r in recs)
            chunk_file.write_bytes(data)

            run_receipt = {
                "play_time_seconds": 612.0,
                "total_frames": 35000,
                "configured_workers": 128,
                "configured_depth": 512,
                "clean_end_game": True,
            }
            chunks_meta = [scan_raw_chunk_metadata(chunk_file)]

            out_dir = td / "artifacts"
            analysis = analyze_discovery_records([chunk_file], run_receipt, out_dir)

            self.assertIn(0x012344, analysis["new_m68k_pcs"])
            self.assertIn(0x0550, analysis["new_z80_pcs"])
            self.assertGreater(analysis["known_resource_hits"]["RESOURCE_1"], 0)
            self.assertGreater(analysis["vdp_writes_count"], 0)
            self.assertGreater(analysis["ym_reg_writes_count"], 0)

            receipt = generate_all_w6_artifacts(run_receipt, chunks_meta, analysis, out_dir)
            self.assertEqual(receipt["pass_condition"], "PASS_LONG_LIVE_GAME_DISCOVERY_RUN_V1")

            # Check all 13 canonical artifacts exist
            expected_files = [
                "discovery_session_manifest.json",
                "discovery_chunk_manifest.json",
                "discovery_health_summary.json",
                "novelty_delta_summary.json",
                "m68k_execution_novelty.json",
                "z80_execution_novelty.json",
                "memory_access_novelty.json",
                "io_register_novelty.json",
                "vdp_activity_novelty.json",
                "audio_stream_novelty.json",
                "cross_cpu_handoff_novelty.json",
                "DISCOVERY_FINDINGS_TABLE.md",
                "THOR_M12_LONG_LIVE_DISCOVERY_REPORT_V1.md",
            ]
            for ef in expected_files:
                p = out_dir / ef
                self.assertTrue(p.is_file(), f"Missing artifact: {ef}")
                self.assertGreater(p.stat().st_size, 0, f"Artifact empty: {ef}")

            # Verify SOURCE_OWNED invariant
            nov = json.loads((out_dir / "novelty_delta_summary.json").read_text(encoding="utf-8"))
            self.assertEqual(nov["source_owned_before"], CANONICAL_SOURCE_OWNED)
            self.assertEqual(nov["source_owned_after"], CANONICAL_SOURCE_OWNED)
            self.assertEqual(nov["source_owned_delta"], 0)


if __name__ == "__main__":
    unittest.main()
