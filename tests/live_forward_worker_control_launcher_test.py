"""Unit tests for Desktop Worker Control launcher and W6/W3 coherent runtime defaults."""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RING_DIR = ROOT / "tools" / "bizhawk-native-ring"
if str(RING_DIR) not in sys.path:
    sys.path.insert(0, str(RING_DIR))

from live_forward_rom_link_runtime import build_parser, WORKER_MEMORY
from live_forward_worker_control_launcher import main as launcher_main
from live_forward_worker_control_model import (
    DEFAULT_CHAIN_DEPTH,
    DEFAULT_WORKER_COUNT,
    load_next_run_config,
)


class LiveForwardWorkerControlLauncherTest(unittest.TestCase):
    def test_default_parser_values_align_with_w6_coherent_architecture(self) -> None:
        parser = build_parser()
        args = parser.parse_args([])

        # Outdated prerequisites removed / optional
        self.assertIsNone(args.decoder)
        self.assertIsNone(args.range_tool)
        self.assertFalse(args.master_startup)
        self.assertTrue(args.in_process_postrun)

        # W6/W3 Coherent runtime defaults
        self.assertTrue(args.until_closed)
        self.assertTrue(args.control_window)
        self.assertFalse(args.natural_input)
        self.assertEqual(args.cadence, 300)
        self.assertEqual(args.max_total_frames, 0)
        self.assertEqual(args.memory_bytes, 512 * 1024)
        self.assertEqual(WORKER_MEMORY, 512 * 1024)
        self.assertEqual(DEFAULT_WORKER_COUNT, 128)
        self.assertEqual(DEFAULT_CHAIN_DEPTH, 512)

        # Default paths
        self.assertEqual(args.script.name, "live_forward_scaling.lua")
        self.assertTrue(str(args.install).endswith("BizHawk-m12-w2-1-frame-coherent-20260920") or
                        "BizHawk" in str(args.install))
        self.assertTrue(str(args.rom).endswith("Beyond Oasis (USA).md"))

    def test_custom_overrides_parsed_correctly(self) -> None:
        parser = build_parser()
        args = parser.parse_args([
            "--workers", "64",
            "--depth", "256",
            "--cadence", "150",
            "--max-total-frames", "1800",
            "--memory-bytes", "262144",
            "--natural-input",
            "--no-control-window",
            "--no-until-closed",
            "--decoder", str(RING_DIR / "fake_decoder.exe"),
        ])

        self.assertEqual(args.workers, 64)
        self.assertEqual(args.depth, 256)
        self.assertEqual(args.cadence, 150)
        self.assertEqual(args.max_total_frames, 1800)
        self.assertEqual(args.memory_bytes, 262144)
        self.assertTrue(args.natural_input)
        self.assertFalse(args.control_window)
        self.assertFalse(args.until_closed)
        self.assertEqual(args.decoder, RING_DIR / "fake_decoder.exe")

    def test_frame_limited_capture_is_a_terminal_postrun_outcome(self) -> None:
        from live_forward_rom_link_runtime import should_launch_postrun_analysis
        self.assertTrue(should_launch_postrun_analysis("STOPPED_FRAME_LIMIT"))

    def test_missing_config_defaults_to_128_workers_and_512_depth(self) -> None:
        nonexistent = ROOT / "build" / "nonexistent-config-file.json"
        config, state = load_next_run_config(nonexistent)
        self.assertEqual(state, "ACCEPTED_DEFAULTS")
        self.assertEqual(config["worker_count"], 128)
        self.assertEqual(config["chain_depth"], 512)

    def test_parse_live_control_handles_32_and_30_metric_payloads(self) -> None:
        from live_forward_worker_control_model import parse_live_control
        # 32-metric payload from real W6/W3 runtime
        metrics32 = ",".join(str(i) for i in range(32))
        plan = "128,512,524288,10916,256,424,54272,524256,67067904,512,512,4096,134217728,896,67127296,201345920"
        workers = "0,2,256,512,10,10,10,10;1,0,0,512,10,10,10,10"
        payload32 = f"18919,128,512,1,{metrics32}|128,512,{plan}|0|{workers}"
        res32 = parse_live_control(payload32)
        self.assertEqual(res32["worker_count"], 128)
        self.assertEqual(res32["depth"], 512)
        self.assertEqual(len(res32["metrics"]), 32)
        self.assertEqual(len(res32["workers"]), 2)
        self.assertEqual(res32["workers"][0]["state"], "CAPTURING")
        self.assertEqual(res32["workers"][0]["progress"], 256)

        # 30-metric payload (backward compatibility)
        metrics30 = ",".join(str(i) for i in range(30))
        payload30 = f"100,16,20,1,{metrics30}|16,20,REJECTED|0|0,0,0,20,1,1,1,1"
        res30 = parse_live_control(payload30)
        self.assertEqual(res30["worker_count"], 16)
        self.assertEqual(len(res30["metrics"]), 30)

    def test_launcher_callable(self) -> None:
        self.assertTrue(callable(launcher_main))


if __name__ == "__main__":
    unittest.main()
