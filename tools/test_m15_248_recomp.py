import json
from pathlib import Path
import struct
import sys

sys.path.insert(0, ".")
from tools.audit_m15_245_candidates import load_production_pcs, load_m15_244_pcs
from tools.audit_m15_246_candidates import load_m15_245_pcs
from tools.audit_m15_247_candidates import load_m15_246_pcs
from tools.thor_recompiler.cfg_reconstructor import reconstruct_cfg
from tools.thor_recompiler.decoder_ext_m15_246 import decode_m15_246_at

root = Path(__file__).resolve().parent.parent
rom = (root / "local-roms/Beyond Oasis (USA).md").read_bytes()
prod_owned = load_production_pcs(root)
m244 = load_m15_244_pcs(root)
m245 = load_m15_245_pcs(root)
m246 = load_m15_246_pcs(root)
m247 = json.loads((root / "data/thor-fixtures/m15_247_ir_manifest.json").read_text(encoding="utf-8"))
m247_pcs = {int(ins["pc"], 16) if isinstance(ins["pc"], str) else int(ins["pc"])
            for blk in m247["blocks"] for ins in blk["instructions"]}
owned_pcs = set(prod_owned.keys()) | m244 | m245 | m246 | m247_pcs

M15_248_ENTRY_ROOTS = {
    0x00FCB8: "callee_fcb8_subroutine",
    0x00D9A4: "callee_d9a4_subroutine",
    0x002D84: "callee_2d84_coord_helper",
    0x002DB6: "callee_2db6_coord_transform",
    0x002E1E: "screen_viewport_math_helper",
    0x002E78: "screen_viewport_bound_helper",
    0x003586: "callee_3586_loop",
    0x00F80E: "callee_f80e_query_helper",
    0x01A184: "state_town_entity_logic",
    0x00FCCE: "scenario_c_subroutine_fcce",
    0x001112: "entity_vector_math_a",
    0x00114A: "entity_vector_math_b",
    0x001E98: "camera_position_transform",
    0x009BC0: "animation_index_comp",
    0x001E46: "camera_bounds_clamp_helper",
}

M15_248_WINDOWS = [
    (0x00FCB8, 0x00FCC4),
    (0x00D9A4, 0x00D9BE),
    (0x002D84, 0x002DB6),
    (0x002DB6, 0x002E1E),
    (0x002E1E, 0x002E78),
    (0x002E78, 0x002EE2),
    (0x003586, 0x0035AC),
    (0x00F80E, 0x00F854),
    (0x01A184, 0x01A1D6),
    (0x00FCCE, 0x00FD5E),
    (0x001112, 0x00114A),
    (0x00114A, 0x001182),
    (0x001E98, 0x001ECE),
    (0x009BC0, 0x009BE2),
    (0x001E46, 0x001E98),
]

cfg = reconstruct_cfg(
    rom,
    M15_248_ENTRY_ROOTS,
    jump_table_base=0x0059BA,
    jump_table_entries=59,
    compiled_targets=set(M15_248_ENTRY_ROOTS.keys()),
    compiled_windows=M15_248_WINDOWS,
    stop_pcs=owned_pcs,
    decode_fn=decode_m15_246_at,
)

dmap = cfg["decoded_map"]
compiled_pcs = set(dmap.keys())
print(f"M15.248 CFG Reconstruction:")
print(f"  Instructions: {len(compiled_pcs)}")
print(f"  Basic Blocks: {len(cfg['basic_blocks'])}")
print(f"  Edges: {len(cfg['edges'])} ({cfg['edge_summary']})")

assert not (compiled_pcs & set(prod_owned.keys())), "Production overlap!"
assert not (compiled_pcs & m244), "M15.244 overlap!"
assert not (compiled_pcs & m245), "M15.245 overlap!"
assert not (compiled_pcs & m246), "M15.246 overlap!"
assert not (compiled_pcs & m247_pcs), "M15.247 overlap!"
print("OVERLAP VERIFICATION: ZERO OVERLAP (100% GREEN)!")
