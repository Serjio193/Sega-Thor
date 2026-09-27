import subprocess
import sys
import tempfile
import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src" / "tools" / "thor_evidence"))
from runtime_rom_map_integration import read_range_export


def run(command, success=True):
    result = subprocess.run(command, capture_output=True, text=True, check=False)
    if (result.returncode == 0) != success:
        raise AssertionError(f"unexpected command result {result.returncode}: {result.stderr}")
    return result


def main():
    tool, fixture = sys.argv[1:]
    with tempfile.TemporaryDirectory() as directory_name:
        directory = Path(directory_name)
        run([fixture, str(directory)])
        a, b = directory / "run-a.bin", directory / "run-b.bin"
        first, second = directory / "union-one.bin", directory / "union-two.bin"
        run([tool, "merge", str(first), str(b), str(a)])
        run([tool, "merge", str(second), str(a), str(b)])
        assert first.read_bytes() == second.read_bytes()
        manifest = Path(str(first) + ".manifest").read_text(encoding="utf-8")
        assert 'input_run_id="run-a"' in manifest and 'input_run_id="run-b"' in manifest
        assert manifest.count("input_run_id=") == 2
        assert hashlib.sha256(a.read_bytes()).hexdigest() in manifest
        export = directory / "union.tsv"
        run([tool, "export", str(first), str(export)])
        metadata, ranges = read_range_export(export)
        assert metadata["run_id"].startswith("union-")
        assert [(r.start, r.end_exclusive, r.property_mask) for r in ranges] == [
            (0, 2, 1), (2, 4, 1 << 5), (4, 16, 0)]
        assert not run([tool, "merge", str(directory / "rejected.bin"),
                        str(a), str(directory / "run-incompatible.bin")], success=False).stdout
        assert not (directory / "rejected.bin").exists()
        assert not run([tool, "merge", str(directory / "duplicate.bin"), str(a), str(a)],
                       success=False).stdout

        rom = directory / "fixture.rom"
        rom.write_bytes(bytes(range(16)))
        manifest = directory / "core-manifest.txt"
        manifest.write_text("pinned-test-core\n", encoding="ascii")
        live_map = directory / "live-map.hex"
        live_map.write_text("0001\n0001\n0004\n0004\n" + "0000\n" * 12,
                            encoding="ascii")
        imported = directory / "live.v1"
        run([tool, "import-hex", str(live_map), str(rom), str(manifest),
             str(imported), "live-run", "1", "5", "PARTIAL"])
        exported = directory / "live.tsv"
        run([tool, "export", str(imported), str(exported)])
        metadata, ranges = read_range_export(exported)
        assert metadata["rom_size"] == "16"
        assert metadata["capabilities"] == "5"
        assert metadata["validation_state"] == "PARTIAL"
        assert [(r.start, r.end_exclusive, r.property_mask) for r in ranges] == [
            (0, 2, 1), (2, 4, 4), (4, 16, 0)]
        bad_capability = directory / "bad-capability.hex"
        bad_capability.write_text("0002\n" + "0000\n" * 15, encoding="ascii")
        assert run([tool, "import-hex", str(bad_capability), str(rom),
                    str(manifest), str(directory / "rejected.v1"),
                    "bad-run", "1", "1", "PARTIAL"], success=False).returncode != 0
        assert not (directory / "rejected.v1").exists()


if __name__ == "__main__":
    main()
