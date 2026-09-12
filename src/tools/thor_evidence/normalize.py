"""Validate a complete V0.1 raw stream and seal one normalized capture."""
import argparse
import hashlib
import json
from pathlib import Path

from .events import write_capture
from .identity import ROM_SHA, ROM_SIZE, SCHEMA, STATE_SHA, canonical
from .receipt import RAW_SCHEMA, load as load_receipt

DEFAULT_ROM = Path("build/reference/Beyond Oasis (USA).bin")
DEFAULT_STATE = Path(r"C:\Dev\SegaThorTools\BizHawk-2.11.1-win-x64\Genesis\State\Beyond Oasis (U) [!].Genplus-gx.QuickSave1.State")
DEFAULT_BIZHAWK = Path(r"C:\Dev\SegaThorTools\BizHawk-2.11.1-win-x64")


def file_hash(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def parse_json(text):
    def reject_duplicate(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise ValueError("duplicate JSON key")
            result[key] = value
        return result
    return json.loads(text, object_pairs_hook=reject_duplicate)


def records(path):
    """Return raw header/events/footer without repairing a missing envelope."""
    parsed = [parse_json(line) for line in Path(path).read_text(encoding="utf-8").splitlines()
              if line.strip()]
    if len(parsed) < 3 or parsed[0].get("kind") != "RAW_HEADER":
        raise ValueError("raw stream is missing its header")
    if parsed[0].get("schema") != RAW_SCHEMA:
        raise ValueError("unsupported raw schema")
    if parsed[-1].get("kind") != "RAW_END":
        raise ValueError("raw stream is missing its final RAW_END")
    footer = parsed[-1]
    if footer.get("schema") != RAW_SCHEMA or footer.get("complete") is not True:
        raise ValueError("raw stream is not complete")
    events = parsed[1:-1]
    if any(item.get("kind") in {"RAW_HEADER", "RAW_END"} for item in events):
        raise ValueError("raw envelope appears more than once")
    if footer.get("events") != len(events):
        raise ValueError("raw event count mismatch")
    for seq, event in enumerate(events):
        if set(event) != {"seq", "epoch", "frame", "actor", "phase", "kind", "data"}:
            raise ValueError("invalid raw event envelope")
        if type(event["seq"]) is not int or event["seq"] != seq:
            raise ValueError("raw sequence discontinuity")
        if event["actor"] != "M68K" or event["phase"] != "RAW":
            raise ValueError("unsupported raw event actor/phase")
    return parsed[0], events, footer


def _receipt_matches(raw_path, raw_header, receipt):
    if receipt["status"] != "COMPLETED":
        raise ValueError("launch receipt is not complete")
    if Path(receipt["raw_path"]).resolve() != Path(raw_path).resolve():
        raise ValueError("raw path is not bound to launch receipt")
    if receipt["raw_sha256"] != file_hash(raw_path):
        raise ValueError("raw artifact hash differs from launch receipt")
    run_dir = Path(receipt["run_dir"])
    for field, filename in (("config_sha256", "config.ini"),
                            ("launch_sha256", "launch.json")):
        artifact = run_dir / filename
        if not artifact.is_file() or receipt.get(field) != file_hash(artifact):
            raise ValueError(f"{filename} differs from launch receipt")
    if receipt.get("result_sha256"):
        result = run_dir / "result.log"
        if not result.is_file() or receipt["result_sha256"] != file_hash(result):
            raise ValueError("result log differs from launch receipt")
    expected = {
        "kind": "RAW_HEADER", "schema": RAW_SCHEMA, "receipt_sha256": receipt["receipt_sha256"],
        "capture_id": receipt["capture_id"], "rom_sha256": ROM_SHA,
        "state_sha256": STATE_SHA, "mode": receipt["mode"],
        "reverse": receipt["reverse"], "watch_plan_sha256": receipt["watch_plan_sha256"],
    }
    if raw_header != expected:
        raise ValueError("raw header does not match launch identity")


def validate_raw_epochs(raw_events, expected_epochs):
    epoch_numbers = [event["epoch"] for event in raw_events]
    if sorted(set(epoch_numbers)) != list(range(1, expected_epochs + 1)):
        raise ValueError("raw stream has incomplete restore epochs")
    if sum(event["kind"] == "EPOCH_BEGIN" for event in raw_events) != expected_epochs or \
       sum(event["kind"] == "EPOCH_END" for event in raw_events) != expected_epochs:
        raise ValueError("raw stream has malformed epoch boundaries")


def normalize(raw_path, run_dir, output_path, mode=None, collector_path=None,
              harness_path=None, rom_path=DEFAULT_ROM, state_path=DEFAULT_STATE,
              bizhawk_dir=DEFAULT_BIZHAWK, receipt_path=None):
    raw_header, raw_events, _ = records(raw_path)
    if receipt_path is None:
        raise ValueError("V0.1 normalization requires a launch receipt")
    receipt = load_receipt(receipt_path)
    _receipt_matches(raw_path, raw_header, receipt)
    validate_raw_epochs(raw_events, receipt["scenario"]["expected_epochs"])
    if mode is not None and mode != receipt["mode"]:
        raise ValueError("mode differs from launch receipt")
    if run_dir and Path(run_dir).resolve() != Path(receipt["run_dir"]).resolve():
        raise ValueError("run directory differs from launch receipt")
    if collector_path and Path(collector_path).resolve() != Path(receipt["collector_path"]).resolve():
        raise ValueError("collector differs from launch receipt")
    if harness_path and Path(harness_path).resolve() != Path(receipt["harness_path"]).resolve():
        raise ValueError("harness differs from launch receipt")
    if Path(rom_path).stat().st_size != ROM_SIZE or file_hash(rom_path) != ROM_SHA:
        raise ValueError("canonical ROM identity mismatch")
    if file_hash(state_path) != STATE_SHA:
        raise ValueError("QuickSave1 identity mismatch")
    env = {"rom_sha256": ROM_SHA, "rom_size": ROM_SIZE,
           "emulator_sha256": receipt["emulator_sha256"], "core_sha256": receipt["core_sha256"],
           "config_sha256": receipt["config_sha256"], "collector_sha256": receipt["collector_sha256"],
           "normalizer_sha256": receipt["normalizer_sha256"], "watch_sha256": receipt["watch_plan_sha256"],
           "harness_sha256": receipt["harness_sha256"],
           "map_sha256": receipt.get("map_sha256", ""), "receipt_sha256": receipt["receipt_sha256"],
           "launch_sha256": receipt["launch_sha256"]}
    scenario = {"state_sha256": STATE_SHA, "specification": {
        **receipt["scenario"], "rom_identity": ROM_SHA, "state_identity": STATE_SHA,
        "raw_schema": RAW_SCHEMA}}
    header = {"schema": SCHEMA, "environment": env, "scenario": scenario}
    normalized = [{"seq": item["seq"], "epoch": item["epoch"], "frame": item["frame"],
                   "actor": item["actor"], "phase": item["phase"], "kind": item["kind"],
                   "data": item["data"], "samples": []} for item in raw_events]
    return write_capture(output_path, header, normalized)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("raw"); parser.add_argument("run_dir"); parser.add_argument("output")
    parser.add_argument("--receipt", required=True); parser.add_argument("--mode")
    args = parser.parse_args(argv)
    print(normalize(args.raw, args.run_dir, args.output, args.mode, receipt_path=args.receipt))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
