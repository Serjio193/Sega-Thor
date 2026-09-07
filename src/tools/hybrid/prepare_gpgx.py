"""Install only the read-only developer bridge in an existing external hook tree."""
import argparse
from pathlib import Path
import shutil


def prepare(root):
    root = root.resolve()
    coverage = root / "core/debug/coverage.c"
    text = coverage.read_text(encoding="utf-8")
    anchor = '  const char *directory = getenv("GPGX_COVERAGE_DIR");'
    guard = '  if (getenv("GPGX_HYBRID_ONLY")) return; /* M11.28: no coverage/UI */'
    if guard not in text:
        if text.count(anchor) != 1:
            raise ValueError("unsupported external coverage initialization")
        coverage.write_text(text.replace(anchor, anchor + "\n" + guard), encoding="utf-8")
    shutil.copyfile(Path(__file__).with_name("gpgx_bridge.c"), root / "core/debug/gpgx_bridge.c")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("gpgx_source", type=Path)
    prepare(parser.parse_args().gpgx_source)
