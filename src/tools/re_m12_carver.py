"""Build the deterministic M12 Stage 1 IntervalDB and gap report."""

import argparse
import hashlib
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
if str(HERE) not in sys.path:
    sys.path.insert(0, str(HERE))
from m12_carver import IntervalDB, canonical


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--rom")
    parser.add_argument("--evidence", action="append", default=[])
    args = parser.parse_args()
    manifest_path = Path(args.manifest).resolve()
    manifest = json.loads(manifest_path.read_text())
    if args.rom:
        rom_path = Path(args.rom).resolve()
        rom = rom_path.read_bytes()
        digest = hashlib.sha256(rom).hexdigest()
        if digest != manifest.get("rom_sha256"):
            raise ValueError("canonical ROM SHA-256 does not match manifest")
        if len(rom) != int(manifest["rom_size"]):
            raise ValueError("canonical ROM size does not match manifest")
    db = IntervalDB.from_manifest(manifest, manifest_path)
    for evidence_path in sorted((Path(value).resolve() for value in args.evidence), key=str):
        payload = json.loads(evidence_path.read_text())
        db.ingest(payload, str(evidence_path))
    report = db.report()
    output = Path(args.output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    (output / "interval_db.json").write_text(json.dumps(db.interval_db(), indent=2, sort_keys=True) + "\n")
    (output / "gap_report.json").write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    print(json.dumps({"output": str(output), "gaps": len(report["gaps"]),
                      "campaigns": len(report["campaigns"]),
                      "source_owned_bytes": report["source_owned_bytes"],
                      "conflicts": len(report["conflicts"]),
                      "fixed_point": report["fixed_point"]}, sort_keys=True))


if __name__ == "__main__":
    main()
