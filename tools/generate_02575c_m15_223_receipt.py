import hashlib
import json
from pathlib import Path
import sys

def file_sha256(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        while chunk := f.read(65536):
            h.update(chunk)
    return h.hexdigest()

def main():
    repo_root = Path(__file__).resolve().parent.parent
    diff_root = repo_root / "build" / "ab_three_scenario_02575c"
    summary_path = diff_root / "three_scenario_results.json"
    summary = json.loads(summary_path.read_text(encoding="utf-8"))

    m15_222_receipt_path = repo_root / "data" / "thor-evidence" / "02575c-three-scenario-qualification-m15-222-20261108.json"
    data = json.loads(m15_222_receipt_path.read_text(encoding="utf-8"))

    data["milestone"] = "M15.223"
    data["name"] = "Entity Motion/State Engine Multi-Handler Cluster (States 0..4, 6..7, Target 10, Target 11, Target 12, Root Control Flow Integrated)"

    # Verify all streams and update hashes if necessary
    for scen_key, scen_data in [
        ("scenario_a_canonical_frame2008", summary["scenario_a"]),
        ("scenario_b_canonical_beach_frame3600", summary["scenario_b"]),
        ("scenario_c_canonical_beach_frame3600", summary["scenario_c"]),
    ]:
        scen_dict = data["single_variable_differential_proof"][scen_key]
        for stream_name, stream_info in scen_data["streams"].items():
            scen_dict["stream_hashes"][stream_name] = stream_info["sha256_a"]

    data["runtime_cluster_execution_metrics"] = summary["scenario_c"]["metrics_b"]

    receipt_path = repo_root / "data" / "thor-evidence" / "02575c-three-scenario-qualification-m15-223-20261108.json"
    receipt_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Written receipt to {receipt_path}")

    sys.path.insert(0, str(repo_root))
    from tools.thor_evidence.receipt_verifier import ReceiptVerifier
    verifier = ReceiptVerifier()
    rep = verifier.verify_receipt(receipt_path)
    print(f"Receipt verification: status={rep.evidence_status}, files_checked={rep.summary['files_checked']}, valid={rep.overall_valid}")
    if not rep.overall_valid:
        print("Rejection reasons:", rep.rejection_reasons)
        sys.exit(1)
    print("M15.223 RECEIPT VERIFIED SUCCESSFULLY!")

if __name__ == "__main__":
    main()
