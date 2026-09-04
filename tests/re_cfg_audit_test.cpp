#include "tools/re_cfg_audit.hpp"

#include <cassert>
#include <string>

using namespace oasis::tools;

AuditRecord record(std::uint32_t address, CfgAuditClassification classification) {
    AuditRecord item;
    item.instruction_address = address;
    item.byte_end = address + 2;
    item.block_start = address;
    item.opcode = 0x4E75;
    item.bytes = {0x4E, 0x75};
    item.classification = classification;
    item.confidence = classification == CfgAuditClassification::unknown ? "UNKNOWN" : "HYPOTHESIS";
    return item;
}

int main() {
    CfgAuditReport report;
    report.target_entry = 0x60004;
    report.window_start = 0x60004;
    report.window_end = 0x61204;
    report.raw_static_evidence_records = 5;
    report.outside_reachable_records = 5;
    report.nonreachable_unresolved = 5;
    report.classification_counts = {
        {"embedded_data_candidate", 1, 2},
        {"secondary_entry_candidate", 1, 2},
        {"unreachable_code_candidate", 1, 2},
        {"decoder_artifact_candidate", 1, 2},
        {"unknown", 1, 2},
    };
    report.records = {
        record(0x1000, CfgAuditClassification::unknown),
        record(0x1002, CfgAuditClassification::embedded_data_candidate),
        record(0x1004, CfgAuditClassification::secondary_entry_candidate),
        record(0x1006, CfgAuditClassification::decoder_artifact_candidate),
        record(0x1008, CfgAuditClassification::unreachable_code_candidate),
    };
    report.records[2].incoming_edges.push_back({0x2000, 0x1004, 0x2000, "direct_call"});
    report.records[1].embedded_data_pattern = true;
    report.records[1].alignment_padding_pattern = true;
    report.records[1].mnemonic = "after_return_bytes";
    report.records[0].decoder_supported = true;
    report.records[2].decoder_supported = true;
    report.records[3].decoder_supported = false;
    report.records[4].decoder_supported = true;
    report.records[4].fallthrough_possible = true;
    report.records[4].outgoing_targets.push_back(0x2000);
    report.islands = {
        {.id = "island_0", .start = 0x1002, .end = 0x100A, .byte_count = 8, .instruction_count = 4,
         .record_addresses = {0x1002, 0x1004, 0x1006, 0x1008},
         .classification = CfgAuditClassification::unknown, .confidence = "UNKNOWN"},
    };
    assert(cfg_audit_classification_name(CfgAuditClassification::unknown) == "unknown");
    assert(cfg_audit_classification_name(CfgAuditClassification::secondary_entry_candidate) == "secondary_entry_candidate");
    assert(classify_cfg_audit_record(report.records[0]) == CfgAuditClassification::unknown);
    assert(classify_cfg_audit_record(report.records[1]) == CfgAuditClassification::embedded_data_candidate);
    assert(classify_cfg_audit_record(report.records[2]) == CfgAuditClassification::secondary_entry_candidate);
    assert(classify_cfg_audit_record(report.records[3]) == CfgAuditClassification::decoder_artifact_candidate);
    assert(classify_cfg_audit_record(report.records[4]) == CfgAuditClassification::unreachable_code_candidate);
    AuditRecord tail = record(0x61203, CfgAuditClassification::boundary_window_tail);
    tail.byte_end = 0x61205;
    assert(classify_cfg_audit_record(tail) == CfgAuditClassification::boundary_window_tail);
    const std::vector<AuditRecord> grouping{record(0x2000, CfgAuditClassification::unknown),
                                            record(0x2002, CfgAuditClassification::unknown),
                                            record(0x2008, CfgAuditClassification::unknown)};
    assert(group_cfg_audit_islands(grouping).size() == 2);
    const auto json = cfg_audit_to_json(report);
    const auto text = cfg_audit_to_text(report);
    assert(json.find("oasis.m68k.re-cfg-audit.v1") != std::string::npos);
    assert(json.find("\"records\":[") != std::string::npos);
    assert(text.find("islands=1") != std::string::npos);
    assert(text.find("secondary_entry_candidate") != std::string::npos);
    assert(json == cfg_audit_to_json(report));
    return 0;
}
