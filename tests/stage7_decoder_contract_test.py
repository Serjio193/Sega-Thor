"""Stage 7 range-decoder contract and fixture checks."""

from __future__ import annotations

import pathlib
import sys
import tempfile
import unittest
from unittest import mock

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src" / "tools"))
sys.path.insert(0, str(ROOT / "src" / "tools" / "thor_evidence"))

from stage7_decode import (  # noqa: E402
    STOP_DECODER_CONTRACT, STOP_DECODER_INPUT, STOP_DECODER_OUTPUT,
    Stage7DecodeFailure, decode_candidate, validate_range_decoder,
)


ROM = pathlib.Path(r"C:\Github\gpgx-test-roms\Beyond Oasis (USA).md")
RANGE_TOOL = ROOT / "build" / "m12-auto2-release" / "oasis_re_assemble_range.exe"
PC_TOOL = ROOT / "build" / "m12-auto2-release" / "oasis_re_rom_range_decode.exe"


class Stage7DecoderContractTests(unittest.TestCase):
    def test_wrong_executable_is_rejected_by_capability_marker(self) -> None:
        if not PC_TOOL.is_file():
            self.skipTest("built PC-list decoder is unavailable")
        with self.assertRaisesRegex(Stage7DecodeFailure, STOP_DECODER_CONTRACT):
            validate_range_decoder(PC_TOOL)

    def test_missing_and_malformed_outputs_fail_closed(self) -> None:
        completed = mock.Mock(returncode=0, stdout="", stderr="")
        with mock.patch("stage7_decode.run", return_value=completed), \
                tempfile.TemporaryDirectory() as directory:
            output = pathlib.Path(directory)
            with self.assertRaisesRegex(Stage7DecodeFailure, STOP_DECODER_OUTPUT):
                decode_candidate(pathlib.Path("tool"), pathlib.Path("rom"),
                                 {"intervals": [[0, 2]]}, output)

    def test_malformed_json_metadata_is_output_failure(self) -> None:
        def write_bad_outputs(command, **_kwargs):
            pathlib.Path(command[-2]).write_text("text", encoding="utf-8")
            pathlib.Path(command[-1]).write_text('{"start":0}', encoding="utf-8")
            return mock.Mock(returncode=0, stdout="", stderr="")

        with mock.patch("stage7_decode.run", side_effect=write_bad_outputs), \
                tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(Stage7DecodeFailure, STOP_DECODER_OUTPUT):
                decode_candidate(pathlib.Path("tool"), pathlib.Path("rom"),
                                 {"intervals": [[0, 2]]}, pathlib.Path(directory))

    def test_input_failure_is_not_labeled_unsupported(self) -> None:
        completed = mock.Mock(returncode=1, stdout="", stderr="error: invalid even bounded range")
        with mock.patch("stage7_decode.run", return_value=completed), \
                tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(Stage7DecodeFailure, STOP_DECODER_INPUT):
                decode_candidate(pathlib.Path("tool"), pathlib.Path("rom"),
                                 {"intervals": [[1, 2]]}, pathlib.Path(directory))

    @unittest.skipUnless(ROM.is_file() and RANGE_TOOL.is_file(),
                         "canonical ROM and release range decoder are unavailable")
    def test_known_ranges_round_trip_through_stage7_parser(self) -> None:
        fixtures = {
            "single": (0x382C, 0x382E),
            "branch": (0x3828, 0x382C),
            "immediate_memory": (0x2AA4, 0x2AAC),
            "multi": (0x3820, 0x3830),
        }
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            for name, (start, end) in fixtures.items():
                (root / name).mkdir()
                decoded, asm = decode_candidate(
                    RANGE_TOOL, ROM, {"intervals": [[start, end]]}, root / name)
                self.assertEqual((decoded["start"], decoded["end"]), (start, end))
                self.assertTrue(decoded["instructions"], name)
                self.assertTrue(asm.is_file(), name)

    @unittest.skipUnless(ROM.is_file() and RANGE_TOOL.is_file(),
                         "canonical ROM and release range decoder are unavailable")
    def test_unsupported_and_truncated_ranges_fail_closed(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(Stage7DecodeFailure, "STOP_UNSUPPORTED_M68K_DECODE"):
                decode_candidate(RANGE_TOOL, ROM, {"intervals": [[0x2D58, 0x2D68]]},
                                 pathlib.Path(directory) / "unsupported")
            with self.assertRaisesRegex(Stage7DecodeFailure, STOP_DECODER_INPUT):
                decode_candidate(RANGE_TOOL, ROM, {"intervals": [[0x382C, 0x382D]]},
                                 pathlib.Path(directory) / "truncated")


if __name__ == "__main__":
    unittest.main(verbosity=2)
