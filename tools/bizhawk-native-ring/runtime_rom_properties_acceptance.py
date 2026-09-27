"""Run matched cold-boot parity and ROM-property overhead measurements."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import statistics
import subprocess
import sys


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def normalized_config_hash(path: Path) -> str:
    config = json.loads(path.read_text(encoding="utf-8-sig"))
    for key in ("PathEntries", "RecentRoms", "RecentMovies", "RecentLua",
                "RecentLuaSession"):
        config.pop(key, None)
    config.update({"ClockThrottle": False, "VSync": False, "VSyncThrottle": False,
                   "Unthrottled": False, "RunInBackground": True,
                   "StartPaused": False, "SingleInstanceMode": False,
                   "AutoLoadLastSaveSlot": False, "AutoSaveLastSaveSlot": False,
                   "AutosaveSaveRAM": False})
    encoded = json.dumps(config, sort_keys=True, separators=(",", ":")).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest()


def load_config(path: Path, isolated: Path) -> dict:
    config = json.loads(path.read_text(encoding="utf-8-sig"))
    config.update({"ClockThrottle": False, "VSync": False, "VSyncThrottle": False,
                   "Unthrottled": False, "RunInBackground": True,
                   "StartPaused": False, "SingleInstanceMode": False,
                   "AutoLoadLastSaveSlot": False, "AutoSaveLastSaveSlot": False,
                   "AutosaveSaveRAM": False})
    for key in ("RecentRoms", "RecentMovies", "RecentLua", "RecentLuaSession"):
        if isinstance(config.get(key), dict):
            config[key]["AutoLoad"] = False
    (isolated / "SaveRAM").mkdir(parents=True, exist_ok=True)
    (isolated / "State").mkdir(parents=True, exist_ok=True)
    for entry in config.get("PathEntries", {}).get("Paths", []):
        if entry.get("System") == "GEN" and entry.get("Type") == "Save RAM":
            entry["Path"] = str(isolated / "SaveRAM")
        if entry.get("System") == "GEN" and entry.get("Type") == "Savestates":
            entry["Path"] = str(isolated / "State")
    return config


def run_lua(install: Path, rom: Path, script: Path, config: Path,
            environment: dict[str, str], timeout: int) -> None:
    env = os.environ.copy()
    env.update(environment)
    process = subprocess.Popen([str(install / "EmuHawk.exe"), f"--config={config}",
                                f"--lua={script}", str(rom)], cwd=install, env=env,
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                               text=True)
    try:
        output, _ = process.communicate(timeout=timeout)
    except subprocess.TimeoutExpired as error:
        process.kill()
        output, _ = process.communicate()
        raise TimeoutError(f"EmuHawk timed out; output: {output[-4000:]}") from error
    if process.returncode != 0:
        raise subprocess.CalledProcessError(process.returncode, process.args, output=output)


def config_for(install: Path, output: Path, label: str) -> Path:
    isolated = output / "runs" / label
    config = load_config(install / "config.ini", isolated)
    destination = output / "runs" / f"{label}.config.json"
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(config), encoding="utf-8")
    return destination


def run_parity(install: Path, rom: Path, script: Path, output: Path,
               label: str, frames: int, mode: str) -> Path:
    trace = output / f"{label}.csv"
    run_lua(install, rom, script, config_for(install, output, label), {
        "THOR_ROM_PROPERTIES_MODE": mode,
        "THOR_ROM_PROPERTIES_PARITY_OUTPUT": str(trace),
        "THOR_ROM_PROPERTIES_PARITY_FRAMES": str(frames),
    }, max(60, frames // 80))
    return trace


def run_benchmark(install: Path, rom: Path, script: Path, output: Path,
                  label: str, frames: int, mode: str) -> str:
    result = output / f"{label}.txt"
    run_lua(install, rom, script, config_for(install, output, label), {
        "THOR_ROM_PROPERTIES_MODE": mode,
        "THOR_ROM_PROPERTIES_BENCH_OUTPUT": str(result),
        "THOR_ROM_PROPERTIES_BENCH_FRAMES": str(frames),
    }, max(60, frames // 80))
    return result.read_text(encoding="utf-8").strip()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline-install", type=Path, required=True)
    parser.add_argument("--instrumented-install", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--frames", type=int, default=1200)
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--long-frames", type=int, default=10000)
    args = parser.parse_args()
    args.baseline_install = args.baseline_install.resolve()
    args.instrumented_install = args.instrumented_install.resolve()
    args.rom = args.rom.resolve()
    args.output_dir = args.output_dir.resolve()

    if args.frames < 120 or args.repeats < 3 or args.long_frames <= args.frames:
        raise SystemExit("require frames >= 120, repeats >= 3, and long-frames > frames")
    baseline_core = args.baseline_install / "dll" / "gpgx.wbx"
    corrected_core = args.instrumented_install / "dll" / "gpgx.wbx"
    if not baseline_core.is_file() or not corrected_core.is_file():
        raise SystemExit("both installs must contain dll/gpgx.wbx")
    args.output_dir.mkdir(parents=True, exist_ok=True)
    scripts = Path(__file__).resolve().parent
    parity_script = scripts / "runtime_rom_properties_parity.lua"
    benchmark_script = scripts / "runtime_rom_properties_benchmark.lua"
    corrected_bytes = corrected_core.read_bytes()
    baseline_bytes = baseline_core.read_bytes()
    identities = {"baseline_core_sha256": sha256(baseline_core),
                  "corrected_release_core_sha256": sha256(corrected_core),
                  "baseline_emuhawk_sha256": sha256(args.baseline_install / "EmuHawk.exe"),
                  "instrumented_emuhawk_sha256": sha256(args.instrumented_install / "EmuHawk.exe"),
                  "baseline_managed_cores_sha256": sha256(args.baseline_install / "dll" / "BizHawk.Emulation.Cores.dll"),
                  "instrumented_managed_cores_sha256": sha256(args.instrumented_install / "dll" / "BizHawk.Emulation.Cores.dll"),
                  "rom_sha256": sha256(args.rom), "rom_size": args.rom.stat().st_size}
    if identities["baseline_emuhawk_sha256"] != identities["instrumented_emuhawk_sha256"]:
        raise SystemExit("EmuHawk executables differ; matched host runner is unavailable")
    baseline_config_hash = normalized_config_hash(args.baseline_install / "config.ini")
    instrumented_config_hash = normalized_config_hash(args.instrumented_install / "config.ini")
    if baseline_config_hash != instrumented_config_hash:
        raise SystemExit("normalized emulator configuration differs between installs")
    parity: dict[str, dict] = {}
    benchmarks: dict[str, list[str]] = {"A_baseline": [], "B_instrumented_off": [],
                                         "C_map_enabled": []}
    modes = {"A_baseline": (baseline_bytes, "BASELINE", args.baseline_install),
             "B_instrumented_off": (corrected_bytes, "OFF", args.instrumented_install),
             "C_map_enabled": (corrected_bytes, "ON_COLD", args.instrumented_install)}
    try:
        for frames in (120, args.frames, args.long_frames):
            for label, (core, mode, install) in (("baseline", modes["A_baseline"]),
                                                  ("instrumented", modes["B_instrumented_off"])):
                corrected_core.write_bytes(core)
                trace = run_parity(install, args.rom, parity_script,
                                   args.output_dir, f"parity-{frames}-{label}", frames, mode)
                parity.setdefault(str(frames), {})[label] = {
                    "trace": trace.name,
                    "trace_sha256": sha256(trace),
                    "state_markers": trace.read_text(encoding="utf-8").splitlines()[-1],
                }
            left = args.output_dir / parity[str(frames)]["baseline"]["trace"]
            right = args.output_dir / parity[str(frames)]["instrumented"]["trace"]
            parity[str(frames)]["trace_equal"] = left.read_bytes() == right.read_bytes()

        orders = (("A_baseline", "B_instrumented_off", "C_map_enabled"),
                  ("C_map_enabled", "A_baseline", "B_instrumented_off"),
                  ("B_instrumented_off", "C_map_enabled", "A_baseline"))
        for repeat in range(args.repeats):
            for label in orders[repeat % len(orders)]:
                core, mode, install = modes[label]
                corrected_core.write_bytes(core)
                benchmarks[label].append(run_benchmark(
                    install, args.rom, benchmark_script,
                    args.output_dir, f"bench-{repeat + 1}-{label}", args.frames, mode))
    finally:
        corrected_core.write_bytes(corrected_bytes)

    fps: dict[str, list[float]] = {}
    for label, lines in benchmarks.items():
        fps[label] = [float(next(part.split("=", 1)[1] for part in line.split()
                                 if part.startswith("fps="))) for line in lines]
    means = {label: statistics.mean(values) for label, values in fps.items()}
    deviations = {label: statistics.stdev(values) for label, values in fps.items()}
    ranges = {label: {"min": min(values), "max": max(values)}
              for label, values in fps.items()}
    baseline, off, on = means["A_baseline"], means["B_instrumented_off"], means["C_map_enabled"]
    report = {"schema": "thor.runtime-rom-property-parity-performance.v1",
              "host": {"platform": platform.platform(), "processor": platform.processor()},
              "build_identities": identities,
              "settings": {"build_mode": "Release", "start_state": "fresh cold boot per run",
                           "normalized_config_sha256": baseline_config_hash,
                           "configuration_settings_match": baseline_config_hash == instrumented_config_hash,
                           "managed_cores_assemblies_match": identities["baseline_managed_cores_sha256"] == identities["instrumented_managed_cores_sha256"],
                           "input": "neutral, joypad.set({}, 1) every frame",
                           "frames": args.frames, "repeats": args.repeats,
                           "clock_throttle": False, "vsync": False,
                           "vsync_throttle": False, "run_in_background": True},
              "parity": parity,
              "benchmark_raw": benchmarks,
              "fps_runs": fps,
              "fps_mean": means, "fps_sample_stddev": deviations, "fps_range": ranges,
              "overhead_percent": {"instrumentation_A_to_B": (baseline-off)/baseline*100,
                                   "property_map_B_to_C": (off-on)/off*100,
                                   "total_A_to_C": (baseline-on)/baseline*100},
              "corrected_core_restored_sha256": sha256(corrected_core)}
    receipt = args.output_dir / "corrected-parity-performance.json"
    receipt.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n", encoding="utf-8")
    print(receipt)
    print(json.dumps({"fps_mean": means, "overhead_percent": report["overhead_percent"],
                      "parity": {k: v["trace_equal"] for k, v in parity.items()}}, indent=2))


if __name__ == "__main__":
    main()
