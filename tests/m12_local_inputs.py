"""Optional local-only inputs for ROM-backed M12 helper tests."""

from pathlib import Path


def optional_bytes(path: Path, label: str):
    if not path.is_file():
        print(f"SKIPPED {label}: local evidence input is unavailable")
        return None
    return path.read_bytes()


def optional_path(path: Path, label: str):
    if not path.is_file():
        print(f"SKIPPED {label}: local evidence input is unavailable")
        return None
    return path
