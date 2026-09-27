"""Command-line entry point for canonical knowledge-map publication."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

try:
    from .rom_knowledge_pipeline import archive_and_refresh_knowledge
except ImportError:
    from rom_knowledge_pipeline import archive_and_refresh_knowledge


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--session", type=Path, required=True,
                        help="closed exact-ROM Cartographer MAP-1 session")
    parser.add_argument("--master", type=Path, required=True,
                        help="accepted Archivist master used to seed the first generation")
    parser.add_argument("--knowledge-db", type=Path, required=True,
                        help="accepted 2D knowledge DB used to seed the first generation")
    parser.add_argument("--base-receipt", type=Path,
                        default=Path("docs/reports/THOR_M12_CANONICAL_ROM_KNOWLEDGE_MAP_2D.json"))
    parser.add_argument("--campaign-receipt", type=Path,
                        help="optional exact ROM-link campaign receipt to reconcile")
    parser.add_argument("--expected-source-owned", type=int, default=1_475_600,
                        help="expected source-owned bytes for the accepted base map")
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path,
                        default=Path("build/thor-evidence/archivist-knowledge-pipeline-2g"))
    parser.add_argument("--report", type=Path,
                        default=Path("docs/reports/THOR_M12_ARCHIVIST_KNOWLEDGE_PIPELINE_2G.md"))
    parser.add_argument("--receipt", type=Path,
                        default=Path("docs/reports/THOR_M12_ARCHIVIST_KNOWLEDGE_PIPELINE_2G.json"))
    args = parser.parse_args()
    result = archive_and_refresh_knowledge(args.session, args.master, args.knowledge_db,
        args.rom, args.output_dir, args.base_receipt, args.campaign_receipt,
        expected_source_owned=args.expected_source_owned,
        report_path=args.report, receipt_path=args.receipt)
    print(json.dumps({"status": result["status"], "generation_id": result["generation_id"],
        "merge_receipt_sha256": result["archivist_merge"]["receipt_sha256"],
        "hashes": result["knowledge_after"]["hashes"],
        "report": str(args.report.resolve()), "receipt": str(args.receipt.resolve())},
        sort_keys=True, indent=2))
    return 0
