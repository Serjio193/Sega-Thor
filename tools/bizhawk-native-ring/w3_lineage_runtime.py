"""Publish post-run W3 companion indexes for audited worker waves."""

from pathlib import Path
import sys

REPO_ROOT = Path(__file__).resolve().parents[2]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from src.tools.thor_evidence.w3_lineage_bridge import publish_run_indexes


def publish_indexes(output_dir: Path, audit_path: Path, *, run_id: int,
                    worker_count: int, audit_sha256: str,
                    chunk_prefix: str) -> list[dict[str, object]]:
    return publish_run_indexes(output_dir, audit_path, run_id=run_id,
        worker_count=worker_count, audit_sha256=audit_sha256,
        chunk_prefix=chunk_prefix)


def publish_indexes_if_configured(output_dir: Path, audit_path: Path, run_id: int,
                                  worker_count: int, audit_sha256: str) -> list[dict[str, object]]:
    import os
    chunk_prefix = os.getenv("LF_CHUNK_PREFIX", "")
    if not chunk_prefix:
        return []
    return publish_indexes(output_dir, audit_path, run_id=run_id,
        worker_count=worker_count, audit_sha256=audit_sha256,
        chunk_prefix=chunk_prefix)
