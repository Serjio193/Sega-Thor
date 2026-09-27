"""Run the developer-only W1 sideband probe in an isolated BizHawk install."""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import time
from pathlib import Path

from live_forward_scaling_runtime import configure_install


def parse_output(path: Path) -> dict[str, str]:
    if not path.is_file():
        return {}
    values: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            values[key] = value
    return values


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--install", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--script", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--timeout", type=float, default=120.0)
    args = parser.parse_args()
    if not (args.install / "EmuHawk.exe").is_file():
        raise FileNotFoundError(args.install / "EmuHawk.exe")
    args.output_dir.mkdir(parents=True, exist_ok=True)
    raw = args.output_dir / "probe-output.txt"
    records = args.output_dir / "probe-records.bin"
    log = args.output_dir / "bizhawk-lua.log"
    config = configure_install(args.install, args.output_dir)
    env = os.environ.copy()
    env.update({"LF_OUTPUT": str(raw), "LF_PROBE_RECORDS": str(records),
                "BH_TEST_LOG": str(log), "LF_RUN_ID": str(int(time.time()))})
    command = [str(args.install / "EmuHawk.exe"), f"--config={config}",
               f"--lua={args.script}", str(args.rom)]
    process = subprocess.Popen(command, cwd=args.install, env=env,
                               stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    started = time.monotonic()
    while time.monotonic() - started < args.timeout:
        values = parse_output(raw)
        if values.get("RESULT") in ("PASS", "FAIL"):
            break
        if process.poll() is not None:
            raise RuntimeError(f"EmuHawk exited before probe result: {process.returncode}")
        time.sleep(0.05)
    else:
        process.terminate()
        raise TimeoutError("sideband probe timed out")
    return_code = process.wait(timeout=15)
    values = parse_output(raw)
    receipt = {"status": values.get("RESULT", "UNKNOWN"), "output": str(raw.resolve()),
               "records": str(records.resolve()), "lua_log": str(log.resolve()),
               "values": values, "emuhawk_exit_code": return_code}
    (args.output_dir / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n",
                                                   encoding="utf-8")
    print(json.dumps(receipt, indent=2))
    if return_code != 0 or values.get("RESULT") != "PASS":
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
