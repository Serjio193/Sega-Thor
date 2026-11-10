#!/usr/bin/env python3
"""
M15.246 Phase 3: Deterministic Extended 3,600-Frame Input Journal Builder.
Preserves the pinned Scenario C 1,800-frame journal (frames 0..1799) verbatim as prefix
and appends a fresh scripted 1,800-frame active gameplay input schedule (frames 1800..3599:
movement sweeps, B sword attacks, X jumps, and A special/interact triggers) executed live by
pinned GPGX (labeled INJECTED_SCRIPTED, never represented as pre-existing human input).
Strictly <= 150 lines per AGENTS.md Rule 3.
"""

import hashlib
import json
import pathlib
import shutil
import subprocess
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))
from tools.thor_auto_migrator.status_adapter import StatusAdapter

# Genesis libretro joypad query order matching existing Scenario C logs
QUERY_ORDER = [
    (0, 1, 0, 10), (0, 1, 0, 9), (0, 1, 0, 11), (0, 1, 0, 2), (0, 1, 0, 1), (0, 1, 0, 0),
    (0, 1, 0, 8), (0, 1, 0, 3), (0, 1, 0, 4), (0, 1, 0, 5), (0, 1, 0, 6), (0, 1, 0, 7),
    (1, 1, 0, 10), (1, 1, 0, 9), (1, 1, 0, 11), (1, 1, 0, 2), (1, 1, 0, 1), (1, 1, 0, 0),
    (1, 1, 0, 8), (1, 1, 0, 3), (1, 1, 0, 4), (1, 1, 0, 5), (1, 1, 0, 6), (1, 1, 0, 7),
]

BIT_B = 1 << 0
BIT_UP = 1 << 4
BIT_DOWN = 1 << 5
BIT_LEFT = 1 << 6
BIT_RIGHT = 1 << 7
BIT_A = 1 << 8
BIT_X = 1 << 9


def scripted_extension_mask(f: int) -> int:
    """Rich gameplay exploration + combat + jump + interact script for frames 1800..3599."""
    rel = f - 1800
    phase = (rel // 45) % 10
    sub = rel % 45
    mask = 0
    if phase == 0:  # Move Right + Attack
        mask = BIT_RIGHT | (BIT_B if sub % 12 < 3 else 0)
    elif phase == 1:  # Move Down-Right + Jump
        mask = BIT_DOWN | BIT_RIGHT | (BIT_X if 10 <= sub < 14 else 0)
    elif phase == 2:  # Attack + Special combo
        mask = BIT_B if sub % 10 < 3 else (BIT_A if 25 <= sub < 29 else 0)
    elif phase == 3:  # Walk Up + Attack (trigger scroll / north beach / town door check)
        mask = BIT_UP | (BIT_B if sub % 15 < 3 else 0)
    elif phase == 4:  # Walk Left + Jump + Attack
        mask = BIT_LEFT | (BIT_X if 5 <= sub < 9 else 0) | (BIT_B if 22 <= sub < 26 else 0)
    elif phase == 5:  # Up-Left combat sweep
        mask = BIT_UP | BIT_LEFT | (BIT_B if sub % 11 < 3 else 0)
    elif phase == 6:  # Walk Right + A interact
        mask = BIT_RIGHT | (BIT_A if sub == 20 else 0)
    elif phase == 7:  # Walk Down + B slash
        mask = BIT_DOWN | (BIT_B if sub % 14 < 3 else 0)
    elif phase == 8:  # Jump + Left
        mask = BIT_LEFT | (BIT_X if sub % 20 < 4 else 0)
    else:  # Recovery / Up-Right sweep
        mask = BIT_UP | BIT_RIGHT | (BIT_B if 15 <= sub < 18 else 0)
    return mask


def format_poll_line(frame: int, mask: int, source: str) -> str:
    q = " ".join(
        f"{p}:{d}:{ix}:{bid}={1 if (p == 0 and bid < 16 and (mask & (1 << bid))) else 0}"
        for (p, d, ix, bid) in QUERY_ORDER
    )
    return f"P {frame} 0 {mask:04x} {source} {len(QUERY_ORDER)} {q}"


def sha(p: pathlib.Path) -> str:
    return hashlib.sha256(p.read_bytes()).hexdigest()


def to_wsl(p: pathlib.Path) -> str:
    r = p.resolve()
    drv = r.drive.replace(":", "").lower()
    posix = r.as_posix()[len(r.drive):] if r.drive else r.as_posix()
    return f"/mnt/{drv}{posix}"


def write_session_manifest(out_dir: pathlib.Path, runner: pathlib.Path, core: pathlib.Path, rom: pathlib.Path) -> None:
    cfg_str = (
        "cold_retro_load_game;frames=3600;input=scripted;poll=retro_input_poll+ordered_retro_input_state;"
        "landmark=60;watch=E376,9C40,E37A,E396,94A2,8EF8,B730,16A46;call=E376:9C40:E37A:E396;"
        "rts=9C7A;video=discard;audio=discard;sram=isolated-per-run"
    )
    lines = [
        "OASIS_GAMEPLAY_SESSION 1",
        "auto_full_calls_sha256=0000000000000000000000000000000000000000000000000000000000000000",
        "call_census=M68K_DIRECT_BSR_JSR_ABS_JSR_AN_V1",
        "call_census_sha256=0000000000000000000000000000000000000000000000000000000000000000",
        "call_watch=0xE376:0x9C40:0xE37A:0xE396;RTS=0x9C7A",
        "calls_sha256=0000000000000000000000000000000000000000000000000000000000000000",
        f"config={cfg_str}",
        f"config_sha256={hashlib.sha256(cfg_str.encode('utf-8')).hexdigest()}",
        f"core_path={to_wsl(core)}",
        f"core_sha256={sha(core)}",
        "cross_cpu_landmark=UNAVAILABLE_M68K_HOOK_ONLY",
        "frames=3600",
        f"initial_state_sha256={sha(out_dir / 'initial.state')}",
        f"initial_state_size={(out_dir / 'initial.state').stat().st_size}",
        "input_mode=scripted",
        "input_poll_count=3600",
        f"input_sha256={sha(out_dir / 'input.polls')}",
        "landmark_period=60",
        f"landmarks_sha256={sha(out_dir / 'landmarks.jsonl')}",
        "reset_method=retro_load_game_cold_start",
        f"rom_sha256={sha(rom)}",
        f"rom_size={rom.stat().st_size}",
        f"runner_path={to_wsl(runner)}",
        f"runner_sha256={sha(runner)}",
        "target_watch=0xE376,0x9C40,0x94A2,0x8EF8,0xB730,0x16A46",
    ]
    (out_dir / "session.manifest").write_bytes(("\n".join(lines) + "\n").encode("utf-8"))


def main() -> int:
    root = pathlib.Path(__file__).resolve().parent.parent
    c_polls = root / "build/ab_portable_m15_244/scenario_c/run_p0/input.polls"
    out_dir = root / "build/m15_246/longrun_3600f"
    out_dir.mkdir(parents=True, exist_ok=True)

    c_lines = c_polls.read_text(encoding="utf-8").splitlines()
    assert c_lines[0] == "OASIS_INPUT_POLL_LOG 1" and len(c_lines) == 1801
    ext_lines = [format_poll_line(f, scripted_extension_mask(f), "INJECTED_SCRIPTED") for f in range(1800, 3600)]
    out_polls = out_dir / "input.polls"
    out_polls.write_bytes(("\n".join(c_lines + ext_lines) + "\n").encode("utf-8"))

    shutil.copyfile(root / "build/checkpoints/canonical_beach_frame3600.state", out_dir / "initial.state")
    runner = root / "build/auto-migrator/m15_245/linux-runner/oasis_gameplay_session"
    core = root / "build/scheduler-fixture-serializerfix-20261013/genesis_plus_gx_libretro.so"
    rom = root / "local-roms/Beyond Oasis (USA).md"
    lm = out_dir / "landmarks.jsonl"
    if not lm.exists() or len(lm.read_text(encoding="utf-8").splitlines()) != 3600:
        if (out_dir / "replay-run").exists():
            shutil.rmtree(out_dir / "replay-run")
        lm.write_text("", encoding="utf-8")
        write_session_manifest(out_dir, runner, core, rom)
        prod_csv = ",".join(f"{h:06x}" for h in sorted(StatusAdapter().active_set))
        cmd = ["wsl", to_wsl(runner), "replay", to_wsl(core), to_wsl(rom), to_wsl(out_dir),
               "--normalized-rom", "--restore-initial-state", f"--native-island=step:{prod_csv}"]
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
        if not (out_dir / "replay-run" / "observed-landmarks.jsonl").exists():
            raise RuntimeError(proc.stderr[-500:] + proc.stdout[-500:])
        shutil.copyfile(out_dir / "replay-run" / "observed-landmarks.jsonl", lm)
    write_session_manifest(out_dir, runner, core, rom)

    prof = json.loads((root / "build/m15_246/longrun_3600f_profile.json").read_text(encoding="utf-8"))
    manifest = {
        "schema": "oasis.m15_246.longrun-3600f-capture-manifest.v1",
        "milestone": "M15.246",
        "initial_state_sha256": sha(out_dir / "initial.state"),
        "input_journal_sha256": sha(out_polls),
        "landmarks_sha256": sha(lm),
        "rom_sha256": sha(rom),
        "core_sha256": sha(core),
        "input_provenance": {"frames_0_to_1799": "PINNED_SCENARIO_C_PREFIX", "frames_1800_to_3599": "INJECTED_SCRIPTED"},
        "frame_count": prof["frames"],
        "poll_count": 3600,
        "game_tick_count": prof["game_vbl_ticks"],
        "total_original_steps": prof["total_original_steps"],
        "idle_steps": prof["idle_steps"],
        "non_idle_steps": prof["non_idle_steps"],
        "original_cpu_trace_sha256": prof["original_cpu_trace_sha256"],
        "video_stream_sha256": prof["video_stream_sha256"],
        "audio_stream_sha256": prof["audio_stream_sha256"],
        "checkpoint_hashes": prof["checkpoints"],
    }
    ev_out = root / "data/thor-evidence/longrun_3600f_capture_manifest_m15_246.json"
    ev_out.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(f"Wrote {out_dir} & {ev_out} (3600f, input_sha={manifest['input_journal_sha256'][:12]})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
