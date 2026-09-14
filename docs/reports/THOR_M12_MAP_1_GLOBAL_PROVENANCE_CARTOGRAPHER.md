# THOR M12 MAP-1 — Global Provenance Cartographer

## Result

**PASS (bounded existing-evidence corpus).** The first persistent global provenance
map was built from existing machine-readable evidence only. No BizHawk campaign,
new capture mechanism, semantic merging, or `SOURCE_OWNED` change was performed.

The authoritative local graph is `build/thor-evidence/map-1/global-provenance-v3.sqlite`
and its machine-readable proof is `build/thor-evidence/map-1/map-1-proof-v3.json`.
These build artifacts are intentionally ignored and are not repository truth by
themselves; this report records the reproducible importer and the measured result.

## Imported corpus

| Domain | Existing source | Imported |
|---|---|---:|
| CPU/register | `THOR_M12_AUTO67_6R3B_TARGETED_BURST_FEASIBILITY.json` | 5 nodes / 4 edges |
| SAT/DMA | `build/m12-gfx-runtime/sat-provenance-current-v5.json` | 15 nodes / 12 edges |
| graphics/resource | materialized AUTO60 manifest + confirmed AUTO64 static program | 6 nodes / 5 edges |
| selector/control | materialized AUTO60 manifest | 18 nodes / 12 edges |

The importers read source files and their hashes. They do not contain a hard-coded
truth table. Runtime occurrences and value versions remain scoped by frame/epoch;
static ROM instructions are canonicalized across compatible evidence sources.

## Persistent schema and proof

SQLite schema `m12.map1.v1` uses `map_meta`, `map_node`, `map_edge`,
`map_frontier`, `map_conflict`, and `map_import`. Every proven edge carries
source lineage and a derivation rule. The graph hash is calculated over sorted
canonical graph rows, excluding import timestamps/order.

| Checkpoint | nodes | edges | proven nodes | proven edges | conflicts | graph hash |
|---|---:|---:|---:|---:|---:|---|
| BEFORE | 0 | 0 | 0 | 0 | 0 | `afa9da796f4e68d27173278ea962cfe3dc266ca06fc3304de534860d5e99977c` |
| FIRST IMPORT | 39 | 33 | 39 | 33 | 0 | `11324348b7da6c2e187c503e3b168ba67a09f464e62e4aa2943262f5794ae3f4` |
| SECOND IMPORT | 39 | 33 | 39 | 33 | 0 | `11324348b7da6c2e187c503e3b168ba67a09f464e62e4aa2943262f5794ae3f4` |

The real corpus has 11 proven ROM ranges covering 8,414 bytes, 8 proven connected
components, no open frontiers, no resolved frontiers, and no conflicts. `SOURCE_OWNED`
is not represented in this graph and remains 0 bytes in the map metric.

The CPU and SAT/DMA paths share the canonical ROM instruction identity for the
runtime DMA launch PC; register versions remain occurrence-specific. No temporal
adjacency, equal values, or semantic graph merge was inferred.

## Idempotence and focused tests

The synthetic C/tail gate produced one positive delta (`1` node, `1` edge) and
replayed with zero delta and an unchanged hash. The 16 focused MAP-1 cases cover
new chains, replay, prefix/tail, prefix/subtree, branching, occurrence and epoch
identity, observed/proven separation and promotion, frontier dedupe/resolution,
conflict retention, component joins, lineage union, deterministic import order,
and separate `SOURCE_OWNED` accounting.

The real run was a bounded file import, not a new runtime campaign. No new
BizHawk/runtime capture was required; raw backlog is therefore **NONEXISTENT** for
this checkpoint. Older AUTO67 reports remain historical and are not superseded by
this graph except for the MAP-1 result recorded here.
