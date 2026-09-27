# M14.7B normalized-generic v2 consumption

```json
{
  "adapter": {
    "artifact_sha256": "9221ca104fbe4a20d4c01a475c2b5f7b66db5b3e9d07785b03d9b7b3a6726ca8",
    "historical_receipt_records": 3514546,
    "input_records": 3514482,
    "mapped_rom_records": 1213333,
    "outcomes": {
      "ALREADY_KNOWN": 1213333,
      "MERGED": 0,
      "REJECTED": 0,
      "UNRESOLVED": 2301149
    },
    "producer_deduplicated_records": 64,
    "producer_deduplication_rule": "closure_rows['records'].setdefault((kind,cpu_id,pc,opcode,address,opcode_verification), row)",
    "producer_metadata": {
      "corpus_id": "run-1790246422-1952eeb1ade6c3ff",
      "index_sha256": "653c4b74257634cd48a52dd9852e226fe502d837b5978b585ccb63589ca6a1a7",
      "raw_sha256": "1952eeb1ade6c3ff4287b930afd0e62b9d25082b037f8ba5552bb8c32c61cf3d",
      "record_count": 3514546,
      "schema": "oasis.m13.normalized-generic-corpus.v2",
      "sealed": true,
      "source_owned_before": 1487672
    },
    "producer_record_count": 3514546,
    "provenance_rows_inserted": 3514482,
    "record_types_adapted": {},
    "record_types_direct": {
      "instruction": 1213333
    },
    "record_types_seen": {
      "bus": 1303241,
      "instruction": 2211241
    },
    "record_types_unresolved": {
      "bus": 1303241,
      "instruction": 997908
    },
    "run_ids": [
      1790246422
    ],
    "status": "PASS_NORMALIZED_V2_ACCOUNTED",
    "unaccounted_records": 0
  },
  "artifact_bytes": 2395075150,
  "artifact_sha256": "9221ca104fbe4a20d4c01a475c2b5f7b66db5b3e9d07785b03d9b7b3a6726ca8",
  "bytes_freed_logical": 2395075150,
  "cleanup": {
    "automatic_wildcard_delete": false,
    "bytes": 2395075150,
    "files": [
      {
        "bytes": 2395075150,
        "experiment_id": "normalized-v2:run-1790246422-1952eeb1ade6c3ff:9221ca104fbe4a20",
        "kind": "raw",
        "path": "C:\\Github\\Sega-Thor\\build\\thor-evidence\\live-worker-control\\campaign-desktop-20260924-101406-764\\master-v2-work\\post-run-analysis\\run-1790246422-651749c647713967\\generic-recursive-closure\\normalized_generic_corpus.json",
        "receipt": "C:\\Github\\Sega-Thor-M14-7B\\docs\\reports\\m14-7b-closure-receipts\\normalized-v2-9221ca104fbe4a20.experiment.json"
      }
    ],
    "status": "DELETED"
  },
  "experiment_closed": true,
  "free_bytes_after": 123130904576,
  "free_bytes_before": 120730935296,
  "free_bytes_before_cleanup": 120735895552,
  "free_bytes_delta": 2395009024,
  "historical_receipt_records": 3514546,
  "lifecycle_checks": {
    "alternate_tails_preserved": true,
    "capture_boundaries_valid": true,
    "conflicts_preserved": true,
    "edge_endpoints_valid": true,
    "loop_multiplicity_preserved": true,
    "objects_valid": true,
    "occurrences_valid": true,
    "ordering_valid": true,
    "paths_reference_occurrences": true,
    "rom_identities_valid": true,
    "unresolved_preserved": true
  },
  "map_generation_after": "gen-9221ca104fbe4a20-6d4a7e2d",
  "map_generation_before": "gen-6f8777084fd64470-ece7acab",
  "map_hash_after": "982fd36d18df8f98012402b15e67ead22c1b9af05ad83771508e74bcf6cc4165",
  "map_hash_before": "2a9dd405b6670404df7b93f955f18e741fb3edb3a8c10507afa3e7d4f6c40e91",
  "map_self_check": {
    "logical_hashes": {
      "emission_hash": "4c8a1951a65a5683012f25cd037c918dcbeaad2b3806a5deaa6d8b5d8e9e317a",
      "evidence_index_hash": "c0b241e257cd0ea8beebb2d7f530090e8b5c027b6e696cf95a9500c72e94c37b",
      "graph_structure_hash": "4b77d26778e52a620602c556e25ecba3382c8a309f13a7ab27525a6114d341bc",
      "map_hash": "982fd36d18df8f98012402b15e67ead22c1b9af05ad83771508e74bcf6cc4165",
      "proposal_set_hash": "292742bd6b6dc2ff372f383fc23647621fcc28a651e1f5c2693c3b005846ab01",
      "structure_hash": "4b77d26778e52a620602c556e25ecba3382c8a309f13a7ab27525a6114d341bc"
    },
    "map_hash": "982fd36d18df8f98012402b15e67ead22c1b9af05ad83771508e74bcf6cc4165",
    "metrics": {
      "asm_bytes": 56678,
      "asset_bytes": 1085110,
      "audio_objects": 3,
      "claims_by_status": {
        "DERIVED_EXACT": 1524,
        "HYPOTHESIS": 1007,
        "OBSERVED_RUNTIME": 1940,
        "STATIC_VERIFIED": 3500
      },
      "classification_bytes": {
        "68000_CODE_CONFIRMED": 1426,
        "ABSOLUTE_STATE_DISPATCH_POINTER_TABLE": 356,
        "ANCIENT_COMPRESSED_RESOURCE": 47389,
        "ASM_ROUNDTRIP_EXACT": 1574,
        "BCEA_FIELD3_SENTINEL_LIST": 7516,
        "BOUNDED_WORD_COPY_TABLE": 94,
        "BOUNDED_WORD_TRANSFORM_TABLE": 128,
        "BYTE_BIT7_LOOKUP_TABLE": 64,
        "BYTE_ENUM_LOOKUP_TABLE": 64,
        "BYTE_LOOKUP_TABLE": 512,
        "CALLER_BACKED_RUNTIME_EXACT_ASM_ROUNDTRIP": 418,
        "CCB0_RELATIVE_TARGET_TABLE_256X16": 7238,
        "CODE_EXECUTED": 734,
        "CODE_STATIC_SUPPORTED": 894,
        "CODE_VERIFIED": 37680,
        "COMPRESSED_RESOURCE_POINTER_TABLE": 432,
        "CONSUMER_BACKED_GRAPHICS_STREAM": 79176,
        "COUNT_BOUNDED_SIX_BYTE_RECORD_STREAM": 46858,
        "DESCRIPTOR": 428,
        "DIRECT_GRAPHICS_STREAM": 61081,
        "DIRECT_MENU_GRAPHICS_STREAM": 5085,
        "ERASED_ROM_ALIGNMENT_PADDING": 132630,
        "EXACT_3820_GRAPHICS_STREAM": 1063,
        "EXACT_SMALL_LOOKUP_TABLE": 58,
        "FIXED_CHECKSUM_RECORD_1208": 9664,
        "FIXED_STRIDE_LOOKUP_TABLE": 1600,
        "FIXED_STRIDE_RECORD_TABLE": 3168,
        "FIXED_WIDTH_ITEM_LABEL_TABLE": 512,
        "FIXED_WIDTH_MENU_LABEL_TABLE": 48,
        "FIXED_WORD_LOOKUP_TABLE": 128,
        "GRAPHICS_COMPRESSED_STREAM": 162331,
        "GRAPHICS_DESCRIPTOR_RECORD": 242,
        "GRAPHICS_DIRECT_LOADER_STREAM": 25207,
        "GRAPHICS_RESOURCE_DESCRIPTOR_TABLE": 124,
        "GROUP_POINTER_TABLE_32X32": 128,
        "HEADER_VECTOR_ASM": 512,
        "INDEXED_SCRIPT_STREAM": 11406,
        "INDEXED_WORD_OFFSET_TABLE": 32,
        "INDIRECT_CALLBACK_ENTRYPOINT": 54,
        "LOCAL_ROM_DERIVED_ASSET": 238087,
        "M12_EXACT_STATIC_ISLAND": 156,
        "MAP_DRIVEN_EXECUTED_ASM_CLOSURE": 284,
        "MENU_OFFSET_TABLE_AND_COUNT_BOUNDED_RECORD_STREAMS": 238,
        "NESTED_LEVEL_LABEL_TABLE": 2169,
        "NUL-terminated text literal": 9,
        "PADDING_ALIGNMENT_CONFIRMED": 47,
        "PC_RELATIVE_DISPATCH_TABLE": 16,
        "PC_RELATIVE_LOOKUP_TABLE": 552,
        "PC_RELATIVE_NIBBLE_LOOKUP_TABLE": 32,
        "PC_RELATIVE_WORD_LOOKUP_TABLE_PREFIX": 24,
        "RELATIVE_SELECTOR_OFFSET_TABLE_5X16": 10,
        "RUNTIME_CORRELATED_EXACT_PROBE_SLICE": 1430,
        "RUNTIME_CORRELATED_GRAPHICS_STREAM": 13166,
        "RUNTIME_COUNT_BOUNDED_6BYTE_RECORD_STREAMS": 1732,
        "RUNTIME_OBSERVED_EXACT_ASM_ROUNDTRIP": 726,
        "SAVE_SLOT_SERIALIZATION_RANGE": 296,
        "SCREEN_DESCRIPTOR_26_BYTE": 4238,
        "SCREEN_DESCRIPTOR_PRIMARY_STREAM": 337514,
        "SCREEN_GROUP_POINTER_TABLE": 84,
        "SELECTED_RELATIVE_POINTER_SLOT_16BIT": 30,
        "SELECTOR_CHILD_TABLE_RECORD_STREAMS": 232,
        "SENTINEL_TERMINATED_THRESHOLD_TABLE": 18,
        "SIGNED_RELATIVE_EVENT_DISPATCH_TABLE": 76,
        "SOUND_DATA_CONTAINER_CONFIRMED": 112468,
        "STATIC_DIRECT_CALLER_RTS_ISLAND": 1158,
        "STATIC_DISPATCH_POINTER_TABLE": 16,
        "STATIC_DISPATCH_TARGET": 230,
        "STATIC_EXACT_BOUNDED_ROUTINE": 136,
        "STATIC_MULTI_DISPATCH_POINTER_TABLE": 60,
        "STATIC_MULTI_DISPATCH_TARGET": 966,
        "STRUCTURED_DATA_CONFIRMED": 108,
        "TABLE_SELECTED_GRAPHICS_STREAM": 115011,
        "UNKNOWN": 1657796,
        "Z80_ASM_SOURCE_OWNED": 8192,
        "eight 4-byte parameter records": 32,
        "eight 4-byte status records": 32,
        "eight-word coordinate lookup": 16,
        "four 16-byte transform records": 64,
        "four 4-byte branch-selector records": 16,
        "four 8-byte transform records": 64,
        "sixteen 4-byte VDP selector records": 64,
        "sixteen-byte nibble lookup": 16,
        "space-filled NUL-terminated text literal": 9,
        "three 12-byte selector records": 36,
        "three-word status lookup": 18
      },
      "conflicts": 0,
      "data_bytes": 245464,
      "emission_bytes_by_type": {
        "ASM": 56678,
        "ASSET": 1085110,
        "DATA": 245464,
        "INCBIN": 1758476
      },
      "emission_ranges": 2490,
      "evidence_refs": 3576646,
      "executed_instruction_objects": 1940,
      "executed_not_fully_owned_objects": 777,
      "executed_unique_rom_bytes": 7636,
      "graphics_objects": 138,
      "hypothesis_claims": 1007,
      "incbin_bytes": 1758476,
      "objects_by_type": {
        "AUDIO_DATA": 3,
        "GRAPHICS_STREAM": 138,
        "M68K_INSTRUCTION": 1966,
        "POINTER_TABLE": 35,
        "ROM_DATA": 737,
        "ROM_RANGE": 1794,
        "UNKNOWN": 759,
        "Z80_PROGRAM": 1
      },
      "relations_by_type": {
        "ASM_CFG_EDGE": 11,
        "DMA_TO_HARDWARE_SAT": 2,
        "EXECUTED_NEXT": 2094,
        "OBSERVED_NEXT_PC": 174,
        "RAM_SHADOW_TO_DMA": 36,
        "STATIC_ENTRY_FALLTHROUGH": 3
      },
      "rom_bytes_total": 3145728,
      "runtime_instruction_range_bytes": 7728,
      "runtime_occurrences_referenced": 3954888,
      "semantic_objects": 5433,
      "source_kind_bytes": {
        "ASM_ROUNDTRIP_EXACT": 260,
        "CODE_VERIFIED": 55798,
        "DATA_KNOWN": 112468,
        "HEADER_VECTOR_ASM": 512,
        "LOCAL_ROM_DERIVED_ASSET": 1085110,
        "PADDING_ALIGNMENT_CONFIRMED": 132677,
        "STRUCTURED_DATA_CONFIRMED": 101107,
        "UNKNOWN": 1657796
      },
      "source_owned_bytes": 1487672,
      "source_owned_code_bytes_unobserved": 45304,
      "source_owned_code_ranges": 516,
      "source_owned_code_ranges_unobserved": 452,
      "source_owned_percent": "47.2918192546",
      "status_coverage_bytes": {
        "DERIVED_EXACT": 1670130,
        "HYPOTHESIS": 1133212,
        "OBSERVED_RUNTIME": 7636,
        "STATIC_VERIFIED": 1487686
      },
      "typed_data_objects": 772,
      "unknown_bytes": 1657796,
      "z80_objects": 1
    },
    "status": "PASS"
  },
  "normalized_deleted": true,
  "provenance_query": {
    "bad_rom_links": 0,
    "complete": true,
    "distinct_ordinals": 3514482,
    "distinct_runs": 1,
    "first_ordinal": 0,
    "last_ordinal": 3514481,
    "outcomes_by_type": {
      "NORMALIZED_V2_RECORD:ALREADY_KNOWN": 1213333,
      "NORMALIZED_V2_RECORD:UNRESOLVED": 2301149
    },
    "references": 3514482
  },
  "replay": {
    "artifact_sha256": "9221ca104fbe4a20d4c01a475c2b5f7b66db5b3e9d07785b03d9b7b3a6726ca8",
    "historical_receipt_records": 3514546,
    "input_records": 3514482,
    "mapped_rom_records": 1213333,
    "outcomes": {
      "ALREADY_KNOWN": 3514482,
      "MERGED": 0,
      "REJECTED": 0,
      "UNRESOLVED": 0
    },
    "producer_deduplicated_records": 64,
    "producer_deduplication_rule": "closure_rows['records'].setdefault((kind,cpu_id,pc,opcode,address,opcode_verification), row)",
    "producer_metadata": {
      "corpus_id": "run-1790246422-1952eeb1ade6c3ff",
      "index_sha256": "653c4b74257634cd48a52dd9852e226fe502d837b5978b585ccb63589ca6a1a7",
      "raw_sha256": "1952eeb1ade6c3ff4287b930afd0e62b9d25082b037f8ba5552bb8c32c61cf3d",
      "record_count": 3514546,
      "schema": "oasis.m13.normalized-generic-corpus.v2",
      "sealed": true,
      "source_owned_before": 1487672
    },
    "producer_record_count": 3514546,
    "provenance_rows_inserted": 0,
    "record_types_adapted": {},
    "record_types_direct": {
      "instruction": 1213333
    },
    "record_types_seen": {
      "bus": 1303241,
      "instruction": 2211241
    },
    "record_types_unresolved": {},
    "run_ids": [
      1790246422
    ],
    "status": "PASS_NORMALIZED_V2_ACCOUNTED",
    "unaccounted_records": 0
  },
  "source_owned_after": 1487672,
  "source_owned_before": 1487672,
  "status": "PASS_NORMALIZED_V2_CAPTURE_CONSUMED"
}
```
