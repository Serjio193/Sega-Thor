"""Run the bounded developer-only M12 controlled-entity observer."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time

from live_forward_scaling_runtime import configure_install


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--install", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--state", type=Path, required=True)
    parser.add_argument("--script", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--hold-frames", type=int, default=5)
    parser.add_argument("--timeout", type=float, default=300.0)
    args = parser.parse_args()
    for path in (args.install / "EmuHawk.exe", args.rom, args.state, args.script):
        if not path.is_file():
            raise FileNotFoundError(path)
    args.output_dir.mkdir(parents=True, exist_ok=True)
    output = args.output_dir / "targeted-controlled-entity.json"
    log = args.output_dir / "emuhawk-console.log"
    config = configure_install(args.install, args.output_dir)
    run_id = str(int(time.time()))
    env = os.environ.copy()
    env.update({
        "OASIS_TARGETED_OUTPUT": str(output.resolve()),
        "OASIS_TARGETED_STATE": str(args.state.resolve()),
        "OASIS_TARGETED_RUN_ID": run_id,
        "OASIS_TARGETED_HOLD_FRAMES": str(args.hold_frames),
    })
    command = [str(args.install / "EmuHawk.exe"), "--chromeless", f"--config={config}",
               f"--lua={args.script.resolve()}", str(args.rom.resolve())]
    with log.open("wb") as console:
        process = subprocess.Popen(command, cwd=args.install, env=env,
                                   stdout=console, stderr=subprocess.STDOUT)
        started = time.monotonic()
        while process.poll() is None and time.monotonic() - started < args.timeout:
            time.sleep(0.25)
        if process.poll() is None:
            process.terminate()
            process.wait(timeout=15)
            raise TimeoutError("targeted controlled-entity capture timed out")
        return_code = process.returncode
    if return_code != 0 or not output.is_file():
        raise RuntimeError(f"targeted capture failed: exit={return_code} output={output}")
    document = json.loads(output.read_text(encoding="utf-8"))
    receipt = {
        "schema": "oasis.m12.targeted-controlled-entity-capture-receipt.v1",
        "status": "PASS_CAPTURE_ARTIFACT" if document.get("worker_disabled") else "FAIL",
        "run_id": run_id,
        "output": str(output.resolve()),
        "output_sha256": sha256(output),
        "rom_sha256": sha256(args.rom),
        "state_sha256": sha256(args.state),
        "emuhawk_exit_code": return_code,
        "frame_count": len(document.get("frame_boundaries", [])),
        "execution_count": len(document.get("executions", [])),
        "bus_event_count": len(document.get("bus_events", [])),
        "snapshot_count": len(document.get("snapshots", [])),
    }
    (args.output_dir / "capture-receipt.json").write_text(
        json.dumps(receipt, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(json.dumps(receipt, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
