// Developer-only Ghidra post-script. It exports Ghidra's own discoveries;
// it does not infer game semantics or replace the project RE tooling.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSetView;
import ghidra.program.model.block.BasicBlockModel;
import ghidra.program.model.block.CodeBlockIterator;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;
import ghidra.program.model.symbol.ReferenceManager;
import ghidra.program.model.listing.FunctionManager;
import ghidra.program.model.symbol.FlowType;
import ghidra.util.task.TaskMonitor;
import java.io.File;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;
import java.util.TreeSet;
public class OasisGhidraMap extends GhidraScript {
    private static final long[] KNOWN_ENTRIES = {
        0x3820L, 0x7A28L, 0x82AEL, 0x8E90L, 0x938EL, 0x9BF2L,
        0xA6A4L, 0xD3B2L, 0x60004L, 0x604BCL, 0x6121AL
    };
    private static final long[] TABLES = {0x5CE96L, 0x96E8L, 0x96F8L, 0xC92CL};
    private static final Map<Long, long[]> KNOWN_RANGES = new TreeMap<>();
    private final List<CallEdge> directCalls = new ArrayList<>();
    private final Set<Long> directBsrTargets = new TreeSet<>();
    private final Set<Long> directJsrTargets = new TreeSet<>();
    private final Set<Long> vectorTargets = new TreeSet<>();
    static {
        KNOWN_RANGES.put(0x3820L, new long[] {0x3820L, 0x3B3EL});
        KNOWN_RANGES.put(0x604BCL, new long[] {0x604BCL, 0x604E6L});
        KNOWN_RANGES.put(0xD3B2L, new long[] {0xD3B2L, 0xD406L});
    }
    private static final class CallEdge {
        final long source;
        final long target;
        final String mnemonic;
        CallEdge(long source, long target, String mnemonic) {
            this.source = source;
            this.target = target;
            this.mnemonic = mnemonic;
        }
    }
    @Override
    public void run() throws Exception {
        if (currentProgram == null) {
            throw new IllegalStateException("OasisGhidraMap requires an imported program");
        }
        String[] args = getScriptArgs();
        if (args.length != 1) {
            throw new IllegalArgumentException("usage: OasisGhidraMap <output.json>");
        }
        collectVectors();
        collectDirectCalls();
        writeJson(new File(args[0]));
    }
    private Address address(long value) {
        try {
            return currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(value);
        } catch (RuntimeException ex) {
            return null;
        }
    }
    private boolean inProgram(long value) {
        Address candidate = address(value);
        return candidate != null && currentProgram.getMemory().contains(candidate);
    }
    private long readVector(int index) {
        try {
            Address start = address(index * 4L);
            return Integer.toUnsignedLong(currentProgram.getMemory().getInt(start));
        } catch (Exception ex) {
            return -1L;
        }
    }
    private void collectVectors() {
        int count = Math.min(64, currentProgram.getMemory().getSize() / 4 > Integer.MAX_VALUE
            ? 64 : (int) currentProgram.getMemory().getSize() / 4);
        for (int index = 0; index < count; index++) {
            long target = readVector(index);
            if (target >= 0 && inProgram(target) && (target & 1L) == 0) {
                vectorTargets.add(target);
            }
        }
    }
    private void collectDirectCalls() {
        Listing listing = currentProgram.getListing();
        InstructionIterator iterator = listing.getInstructions(true);
        while (iterator.hasNext()) {
            Instruction instruction = iterator.next();
            FlowType flow = instruction.getFlowType();
            if (!flow.isCall()) {
                continue;
            }
            String mnemonic = instruction.getMnemonicString().toUpperCase();
            for (Address target : instruction.getFlows()) {
                if (target == null) {
                    continue;
                }
                long targetOffset = target.getOffset();
                directCalls.add(new CallEdge(instruction.getAddress().getOffset(),
                    targetOffset, mnemonic));
                if (mnemonic.startsWith("BSR")) {
                    directBsrTargets.add(targetOffset);
                } else if (mnemonic.startsWith("JSR")) {
                    directJsrTargets.add(targetOffset);
                }
            }
        }
        Collections.sort(directCalls, (a, b) -> {
            int source = Long.compare(a.source, b.source);
            return source != 0 ? source : Long.compare(a.target, b.target);
        });
    }
    private Function functionAt(long entry) {
        Address candidate = address(entry);
        return candidate == null ? null : currentProgram.getFunctionManager().getFunctionAt(candidate);
    }
    private Function functionContaining(long point) {
        Address candidate = address(point);
        return candidate == null ? null : currentProgram.getFunctionManager().getFunctionContaining(candidate);
    }
    private String hex(long value) {
        return String.format("0x%06X", value);
    }
    private String range(Function function) {
        if (function == null || function.getBody() == null) {
            return "UNKNOWN";
        }
        AddressSetView body = function.getBody();
        return hex(body.getMinAddress().getOffset()) + ".." +
            hex(body.getMaxAddress().getOffset() + 1L);
    }
    private int instructionCount(Function function) {
        if (function == null) {
            return 0;
        }
        int count = 0;
        InstructionIterator iterator = currentProgram.getListing().getInstructions(function.getBody(), true);
        while (iterator.hasNext()) {
            iterator.next();
            count++;
        }
        return count;
    }
    private int blockCount(Function function) {
        if (function == null) {
            return 0;
        }
        try {
            BasicBlockModel model = new BasicBlockModel(currentProgram);
            CodeBlockIterator iterator = model.getCodeBlocksContaining(function.getBody(), monitor);
            int count = 0;
            while (iterator.hasNext()) {
                iterator.next();
                count++;
            }
            return count;
        } catch (Exception ex) {
            return -1;
        }
    }
    private boolean hasReturn(Function function) {
        if (function == null) {
            return false;
        }
        InstructionIterator iterator = currentProgram.getListing().getInstructions(function.getBody(), true);
        while (iterator.hasNext()) {
            String mnemonic = iterator.next().getMnemonicString().toUpperCase();
            if (mnemonic.equals("RTS") || mnemonic.equals("RTE") || mnemonic.equals("RTR") ||
                mnemonic.equals("RTD") || mnemonic.equals("RTT")) {
                return true;
            }
        }
        return false;
    }
    private List<Long> callsFrom(long entry) {
        Set<Long> result = new TreeSet<>();
        Function function = functionAt(entry);
        if (function == null) {
            return new ArrayList<>(result);
        }
        for (CallEdge edge : directCalls) {
            Function caller = functionContaining(edge.source);
            if (caller != null && caller.getEntryPoint().getOffset() == entry) {
                result.add(edge.target);
            }
        }
        return new ArrayList<>(result);
    }
    private List<Long> calledBy(long entry) {
        Set<Long> result = new TreeSet<>();
        for (CallEdge edge : directCalls) {
            if (edge.target != entry) {
                continue;
            }
            Function caller = functionContaining(edge.source);
            result.add(caller == null ? edge.source : caller.getEntryPoint().getOffset());
        }
        return new ArrayList<>(result);
    }
    private boolean knownEntry(long value) {
        for (long known : KNOWN_ENTRIES) {
            if (known == value) {
                return true;
            }
        }
        return false;
    }
    private boolean overlapsTable(long value) {
        return value == 0x5CE96L || value == 0x96E8L || value == 0x96F8L || value == 0xC92CL;
    }
    private String entryClassification(long entry, Function function, Instruction instruction) {
        long[] known = KNOWN_RANGES.get(entry);
        if (function == null) {
            return instruction == null ? "MISSED_ENTRY" : "CODE_ONLY";
        }
        if (known != null) {
            AddressSetView body = function.getBody();
            if (body.getMinAddress().getOffset() != known[0] ||
                body.getMaxAddress().getOffset() + 1L != known[1]) {
                return "WRONG_BOUNDARY";
            }
        }
        return "EXACT_FUNCTION_MATCH";
    }
    private void writeJson(File output) throws Exception {
        File parent = output.getParentFile();
        if (parent != null) {
            parent.mkdirs();
        }
        try (PrintWriter out = new PrintWriter(new OutputStreamWriter(
            new FileOutputStream(output), StandardCharsets.UTF_8))) {
            out.println("{");
            field(out, 1, "schema", "oasis.m68k.ghidra-map.v1", true);
            out.println("  \"metadata\": {");
            field(out, 2, "program_name", currentProgram.getName(), true);
            field(out, 2, "language_id", currentProgram.getLanguageID().toString(), true);
            field(out, 2, "compiler_spec", currentProgram.getCompilerSpec().getCompilerSpecID().toString(), true);
            field(out, 2, "image_base", hex(currentProgram.getImageBase().getOffset()), true);
            field(out, 2, "analysis_mode", "headless_post_script", true);
            field(out, 2, "semantic_status", "GHIDRA_CANDIDATE_ONLY", false);
            out.println("  },");
            arrayLongs(out, 1, "vector_derived_targets", vectorTargets, true);
            arrayLongs(out, 1, "direct_bsr_targets", directBsrTargets, true);
            arrayLongs(out, 1, "direct_jsr_targets", directJsrTargets, true);
            writeFunctions(out);
            writeCandidates(out);
            writeKnownBenchmark(out);
            writeCallBenchmark(out);
            writeDataTables(out);
            writeIndirectObservation(out);
            writeUnresolvedCalls(out);
            out.println("  \"probable_code_data_conflicts\": [],");
            writeSample(out);
            out.println("  \"notes\": [\"Decompiler output is not exported as evidence.\",\n" +
                "    \"Ghidra findings require independent oasis_re and dynamic verification.\"\n  ]");
            out.println("}");
        }
    }
    private void writeFunctions(PrintWriter out) {
        out.println("  \"functions\": [");
        List<Function> functions = new ArrayList<>();
        FunctionIterator iterator = currentProgram.getFunctionManager().getFunctions(true);
        while (iterator.hasNext()) {
            functions.add(iterator.next());
        }
        Collections.sort(functions, (a, b) -> a.getEntryPoint().compareTo(b.getEntryPoint()));
        for (int index = 0; index < functions.size(); index++) {
            Function function = functions.get(index);
            out.println("    {");
            field(out, 3, "entry", hex(function.getEntryPoint().getOffset()), true);
            field(out, 3, "name", function.getName(), true);
            field(out, 3, "range", range(function), true);
            field(out, 3, "source", "GHIDRA_RECOGNIZED_FUNCTION", true);
            arrayLongs(out, 3, "called_by", calledBy(function.getEntryPoint().getOffset()), true);
            arrayLongs(out, 3, "calls", callsFrom(function.getEntryPoint().getOffset()), true);
            boolField(out, 3, "has_return", hasReturn(function), true);
            intField(out, 3, "instruction_count", instructionCount(function), true);
            intField(out, 3, "basic_block_count", blockCount(function), true);
            boolField(out, 3, "ghidra_function_flag", true, true);
            field(out, 3, "ghidra_confidence", "NOT_EXPOSED_BY_API", true);
            field(out, 3, "notes", "No semantic name promotion", false);
            out.println(index + 1 == functions.size() ? "    }" : "    },");
        }
        out.println("  ],");
    }
    private void writeCandidates(PrintWriter out) {
        Set<Long> candidates = new TreeSet<>();
        candidates.addAll(vectorTargets);
        candidates.addAll(directBsrTargets);
        candidates.addAll(directJsrTargets);
        List<Long> sorted = new ArrayList<>(candidates);
        out.println("  \"candidates\": [");
        for (int index = 0; index < sorted.size(); index++) {
            long entry = sorted.get(index);
            Function function = functionAt(entry);
            Instruction instruction = currentProgram.getListing().getInstructionAt(address(entry));
            out.println("    {");
            field(out, 3, "entry", hex(entry), true);
            field(out, 3, "name", function == null ? "GHIDRA_CANDIDATE" : function.getName(), true);
            field(out, 3, "range", range(function), true);
            field(out, 3, "source", function == null ? "VECTOR_OR_DIRECT_CALL_TARGET" :
                "GHIDRA_RECOGNIZED_FUNCTION", true);
            boolField(out, 3, "decoded_as_code", instruction != null, true);
            field(out, 3, "notes", "Candidate; independent verification required", false);
            out.println(index + 1 == sorted.size() ? "    }" : "    },");
        }
        out.println("  ],");
    }
    private void writeKnownBenchmark(PrintWriter out) {
        out.println("  \"known_entry_benchmark\": [");
        Listing listing = currentProgram.getListing();
        for (int index = 0; index < KNOWN_ENTRIES.length; index++) {
            long entry = KNOWN_ENTRIES[index];
            Function function = functionAt(entry);
            Instruction instruction = listing.getInstructionAt(address(entry));
            out.println("    {");
            field(out, 3, "address", hex(entry), true);
            field(out, 3, "project_status", knownEntry(entry) ? "ATLAS_KNOWN" : "UNKNOWN", true);
            boolField(out, 3, "ghidra_function", function != null, true);
            field(out, 3, "ghidra_range", range(function), true);
            String classification = entryClassification(entry, function, instruction);
            field(out, 3, "classification", classification, true);
            field(out, 3, "known_project_range", knownRange(entry), true);
            field(out, 3, "notes", instruction == null ? "No instruction at entry" :
                "Function/code presence measured from Ghidra listing", false);
            out.println(index + 1 == KNOWN_ENTRIES.length ? "    }" : "    },");
        }
        out.println("  ],");
    }
    private String knownRange(long entry) {
        long[] range = KNOWN_RANGES.get(entry);
        return range == null ? "UNKNOWN" : hex(range[0]) + ".." + hex(range[1]);
    }
    private void writeCallBenchmark(PrintWriter out) {
        long[][] expected = {
            {0x60004L, 0x6042AL}, {0x60B8CL, 0x6121AL}, {0x60D4AL, 0x6121AL},
            {0x611EEL, 0x6121AL}, {0x60BCCL, 0x604BCL}, {0xD3B2L, 0x3820L}
        };
        out.println("  \"known_call_edge_benchmark\": [");
        for (int index = 0; index < expected.length; index++) {
            boolean found = false;
            for (CallEdge edge : directCalls) {
                found |= edge.source == expected[index][0] && edge.target == expected[index][1];
            }
            out.println("    {");
            field(out, 3, "expected_edge", hex(expected[index][0]) + " -> " + hex(expected[index][1]), true);
            field(out, 3, "ghidra_result", found ? "GHIDRA_FOUND" : "GHIDRA_MISSED", false);
            out.println(index + 1 == expected.length ? "    }" : "    },");
        }
        out.println("  ],");
    }
    private void writeDataTables(PrintWriter out) {
        out.println("  \"known_data_table_check\": [");
        Listing listing = currentProgram.getListing();
        ReferenceManager references = currentProgram.getReferenceManager();
        for (int index = 0; index < TABLES.length; index++) {
            long value = TABLES[index];
            Address target = address(value);
            Data data = target == null ? null : listing.getDefinedDataAt(target);
            boolean code = target != null && listing.getInstructionAt(target) != null;
            int xrefs = 0;
            if (target != null) {
                ReferenceIterator iterator = references.getReferencesTo(target);
                while (iterator.hasNext()) {
                    iterator.next();
                    xrefs++;
                }
            }
            out.println("    {");
            field(out, 3, "address", hex(value), true);
            boolField(out, 3, "recognized_as_data", data != null, true);
            boolField(out, 3, "decoded_as_code", code, true);
            boolField(out, 3, "useful_xrefs", xrefs > 0, true);
            intField(out, 3, "xref_count", xrefs, false);
            out.println(index + 1 == TABLES.length ? "    }" : "    },");
        }
        out.println("  ],");
    }
    private void writeIndirectObservation(PrintWriter out) {
        Address target = address(0xA7E2L);
        Instruction instruction = target == null ? null : currentProgram.getListing().getInstructionAt(target);
        FlowType flow = instruction == null ? null : instruction.getFlowType();
        out.println("  \"indirect_flow_observation\": {");
        field(out, 2, "address", hex(0xA7E2L), true);
        boolField(out, 2, "instruction_present", instruction != null, true);
        field(out, 2, "mnemonic", instruction == null ? "UNKNOWN" : instruction.getMnemonicString(), true);
        boolField(out, 2, "ghidra_identifies_indirect_flow", flow != null && flow.isIndirect(), true);
        boolField(out, 2, "creates_references_or_targets", instruction != null && instruction.getFlows().length > 0, true);
        field(out, 2, "dynamic_evidence_conflict", "NOT_ASSESSED", false);
        out.println("  },");
    }
    private void writeUnresolvedCalls(PrintWriter out) {
        out.println("  \"external_unresolved_call_targets\": [");
        out.println("  ],");
    }
    private void writeSample(PrintWriter out) {
        Set<Long> selected = new TreeSet<>();
        FunctionIterator iterator = currentProgram.getFunctionManager().getFunctions(true);
        while (iterator.hasNext() && selected.size() < 10) {
            Function function = iterator.next();
            long entry = function.getEntryPoint().getOffset();
            if (!knownEntry(entry)) {
                selected.add(entry);
            }
        }
        for (long target : directBsrTargets) {
            if (selected.size() >= 20) {
                break;
            }
            selected.add(target);
        }
        for (long target : directJsrTargets) {
            if (selected.size() >= 20) {
                break;
            }
            selected.add(target);
        }
        out.println("  \"false_positive_sample\": [");
        List<Long> values = new ArrayList<>(selected);
        for (int index = 0; index < values.size(); index++) {
            long entry = values.get(index);
            Function function = functionAt(entry);
            Instruction instruction = currentProgram.getListing().getInstructionAt(address(entry));
            boolean code = instruction != null;
            boolean directTarget = directBsrTargets.contains(entry) || directJsrTargets.contains(entry);
            String classification = !code ? "AMBIGUOUS" :
                (function != null && hasReturn(function) || directTarget ? "LIKELY_CODE" : "AMBIGUOUS");
            out.println("    {");
            field(out, 3, "entry", hex(entry), true);
            field(out, 3, "selection", function != null && !knownEntry(entry) ?
                "FIRST_FUNCTION_BY_ADDRESS" : "DIRECT_BSR_OR_JSR_TARGET", true);
            boolField(out, 3, "entry_decodes_cleanly", code, true);
            boolField(out, 3, "target_of_direct_call", directTarget, true);
            boolField(out, 3, "has_return_in_function_body", function != null && hasReturn(function), true);
            boolField(out, 3, "overlaps_known_table", overlapsTable(entry), true);
            field(out, 3, "classification", classification, false);
            out.println(index + 1 == values.size() ? "    }" : "    },");
        }
        out.println("  ],");
    }
    private void field(PrintWriter out, int indent, String name, String value, boolean comma) {
        out.println("  ".repeat(indent) + quote(name) + ": " + quote(value) + (comma ? "," : ""));
    }
    private void boolField(PrintWriter out, int indent, String name, boolean value, boolean comma) {
        out.println("  ".repeat(indent) + quote(name) + ": " + value + (comma ? "," : ""));
    }
    private void intField(PrintWriter out, int indent, String name, int value, boolean comma) {
        out.println("  ".repeat(indent) + quote(name) + ": " + value + (comma ? "," : ""));
    }
    private void arrayLongs(PrintWriter out, int indent, String name, Set<Long> values, boolean comma) {
        arrayLongs(out, indent, name, new ArrayList<>(values), comma);
    }
    private void arrayLongs(PrintWriter out, int indent, String name, List<Long> values, boolean comma) {
        out.println("  ".repeat(indent) + quote(name) + ": [");
        for (int index = 0; index < values.size(); index++) {
            out.println("  ".repeat(indent + 1) + quote(hex(values.get(index))) +
                (index + 1 == values.size() ? "" : ","));
        }
        out.println("  ".repeat(indent) + "]" + (comma ? "," : ""));
    }
    private String quote(String value) {
        StringBuilder result = new StringBuilder("\"");
        for (char character : value.toCharArray()) {
            if (character == '\\' || character == '\"') {
                result.append('\\');
            }
            if (character == '\n') {
                result.append("\\n");
            } else if (character == '\r') {
                result.append("\\r");
            } else if (character == '\t') {
                result.append("\\t");
            } else {
                result.append(character);
            }
        }
        return result.append('"').toString();
    }
}
