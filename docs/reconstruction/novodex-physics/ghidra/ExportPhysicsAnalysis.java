/* Export Ghidra's semantic view of NxPhysics.dll as one JSON-lines stream.
 *
 * The stream is raw evidence: addresses stay absolute and Ghidra's own metadata
 * map is emitted whole. normalize_ghidra.py converts addresses to RVAs and drops
 * the session-local metadata, so anything volatile is removed in one reviewable
 * place rather than here.
 *
 * Usage: -postScript ExportPhysicsAnalysis.java <output.jsonl>
 */
//@category Novodex

import java.io.BufferedWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.framework.options.Options;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressRange;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Program;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceManager;
import ghidra.program.model.symbol.Symbol;
import ghidra.program.model.symbol.SymbolIterator;

public class ExportPhysicsAnalysis extends GhidraScript {

	/** The analyzer contract analysis_toolchain.json pins. */
	private static final String[][] PINNED_ANALYZERS = {
		{ "Aggressive Instruction Finder", "false" },
		{ "Decompiler Parameter ID", "true" },
		{ "Demangler Microsoft", "true" },
		{ "Non-Returning Functions - Discovered", "true" },
		{ "Windows x86 PE Exception Handling", "true" },
		{ "Windows x86 PE RTTI Analyzer", "true" },
	};

	private static final int DECOMPILE_TIMEOUT_SECONDS = 60;
	private static final String SIMPLIFICATION_STYLE = "decompile";

	/**
	 * The data type names Ghidra 12.1.2's "Windows x86 PE RTTI Analyzer" applies.
	 * The legacy `RTTI_0`…`RTTI_4` names this analyzer never applies would make
	 * the rtti table look empty on a binary full of RTTI.
	 */
	private static final String TYPE_DESCRIPTOR = "TypeDescriptor";
	private static final java.util.Set<String> RTTI_TYPE_NAMES = java.util.Set.of(
		TYPE_DESCRIPTOR,
		"RTTICompleteObjectLocator",
		"RTTIBaseClassDescriptor",
		"RTTIBaseClassArray",
		"RTTIClassHierarchyDescriptor");

	private PrintWriter out;

	@Override
	public void run() throws Exception {
		String[] args = getScriptArgs();
		if (args.length != 1) {
			throw new IllegalArgumentException(
				"ExportPhysicsAnalysis.java requires exactly one argument: the output path");
		}
		Map<String, String> analyzers = verifyPinnedAnalyzers();

		BufferedWriter writer = Files.newBufferedWriter(
			Paths.get(args[0]), StandardCharsets.UTF_8);
		try (PrintWriter print = new PrintWriter(writer)) {
			out = print;
			emitProgram(analyzers);
			emitTypeCoverage();
			emitFunctions();
			emitInstructionRanges();
			emitSymbols();
			emitDefinedData();
			emitReferences();
			emitVtables();
			emitRtti();
		}
	}

	/**
	 * Confirm every pinned analyzer name resolves in this Ghidra and still holds
	 * its pinned value. A name Ghidra does not register would be ignored in
	 * silence, so the pin would certify a configuration that never ran.
	 */
	private Map<String, String> verifyPinnedAnalyzers() {
		Options options = currentProgram.getOptions(Program.ANALYSIS_PROPERTIES);
		List<String> registered = options.getOptionNames();
		Map<String, String> effective = new LinkedHashMap<>();
		for (String[] pinned : PINNED_ANALYZERS) {
			if (!registered.contains(pinned[0])) {
				throw new IllegalStateException("analyzer '" + pinned[0]
					+ "' is not registered by this Ghidra; the pin names an analyzer that "
					+ "analyzeHeadless would ignore");
			}
			String actual = options.getValueAsString(pinned[0]);
			if (!pinned[1].equals(actual)) {
				throw new IllegalStateException("analyzer '" + pinned[0] + "' is " + actual
					+ " but the pin requires " + pinned[1]);
			}
			effective.put(pinned[0], actual);
		}
		return effective;
	}

	private void emitProgram(Map<String, String> analyzers) {
		Json program = new Json("program")
			.put("name", currentProgram.getName())
			.put("image_base", hex(currentProgram.getImageBase()))
			.put("language_id", currentProgram.getLanguageID().getIdAsString())
			.put("compiler_spec_id",
				currentProgram.getCompilerSpec().getCompilerSpecID().getIdAsString())
			.put("address_space",
				currentProgram.getAddressFactory().getDefaultAddressSpace().getName())
			.put("ghidra_version", ghidra.framework.Application.getApplicationVersion())
			// The decompiler settings decide decompiler_status, so a hash that
			// differs because a slower host timed out stays diagnosable.
			.put("decompiler_timeout_seconds", DECOMPILE_TIMEOUT_SECONDS)
			.put("decompiler_simplification_style", SIMPLIFICATION_STYLE)
			.put("decompiler_options", "DecompileOptions defaults")
			// getAllSymbols(false) omits dynamic symbols, and the iterator walks
			// symbols by address, which omits variable and parameter symbols. The
			// total is recorded so a reader can see how filtered this view is.
			.put("symbol_table_total", currentProgram.getSymbolTable().getNumSymbols())
			.put("symbol_scope", "non-dynamic symbols that have an address");

		StringBuilder options = new StringBuilder("{");
		for (Map.Entry<String, String> entry : analyzers.entrySet()) {
			if (options.length() > 1) {
				options.append(",");
			}
			options.append(Json.quote(entry.getKey())).append(":").append(entry.getValue());
		}
		program.raw("analysis_options", options.append("}").toString());

		// Ghidra's metadata map is emitted whole; the normalizer selects from it.
		StringBuilder metadata = new StringBuilder("{");
		for (Map.Entry<String, String> entry : new TreeMap<>(currentProgram.getMetadata())
				.entrySet()) {
			if (metadata.length() > 1) {
				metadata.append(",");
			}
			metadata.append(Json.quote(entry.getKey())).append(":")
					.append(Json.quote(entry.getValue()));
		}
		program.raw("metadata", metadata.append("}").toString());

		StringBuilder blocks = new StringBuilder("[");
		for (MemoryBlock block : currentProgram.getMemory().getBlocks()) {
			if (blocks.length() > 1) {
				blocks.append(",");
			}
			blocks.append(new Json(null)
				.put("name", block.getName())
				.put("start", hex(block.getStart()))
				.put("end", hex(block.getEnd()))
				.put("executable", block.isExecute())
				.put("initialized", block.isInitialized())
				.toString());
		}
		program.raw("memory_blocks", blocks.append("]").toString());
		write(program);
	}

	/**
	 * Re-emit the coverage ApplyPhysicsTypes recorded, when the typed pass has run.
	 * These option names are the contract with ApplyPhysicsTypes.java; they are
	 * spelled out here so neither script has to compile against the other.
	 */
	private void emitTypeCoverage() {
		Options options = currentProgram.getOptions("Novodex");
		if (!options.contains("type_coverage.header_sha256")) {
			return;
		}
		write(new Json("type_coverage")
			.put("header_sha256", options.getString("type_coverage.header_sha256", ""))
			.put("types_parsed", options.getInt("type_coverage.types_parsed", 0))
			.put("applied_exports", options.getInt("type_coverage.applied_exports", 0))
			.put("applied_vtables", options.getInt("type_coverage.applied_vtables", 0))
			.put("typed_vtable_slots", options.getInt("type_coverage.typed_vtable_slots", 0))
			.put("skipped_vtable_slots", options.getInt("type_coverage.skipped_vtable_slots", 0))
			.raw("skipped_slot_reasons", Json.array(Json.lines(
				options.getString("type_coverage.skipped_slot_reasons", ""))))
			.raw("unresolved",
				Json.stringList(options.getString("type_coverage.unresolved", "")))
			.raw("exports_undeclared_in_headers", Json.stringList(
				options.getString("type_coverage.exports_undeclared_in_headers", "")))
			.raw("non_export_entry_points", Json.stringList(
				options.getString("type_coverage.non_export_entry_points", "")))
			// Types the generator withheld, with the reason, so the caveat reaches
			// the manifest instead of living only in the generator's inputs.
			.raw("excluded_types", Json.pairs(
				options.getString("type_coverage.excluded_type_names", ""),
				options.getString("type_coverage.excluded_type_reasons", ""))));
	}

	private void emitFunctions() throws Exception {
		DecompInterface decompiler = new DecompInterface();
		decompiler.setOptions(new DecompileOptions());
		decompiler.toggleCCode(true);
		decompiler.toggleSyntaxTree(false);
		decompiler.setSimplificationStyle(SIMPLIFICATION_STYLE);
		if (!decompiler.openProgram(currentProgram)) {
			throw new IllegalStateException(
				"the decompiler refused this program: " + decompiler.getLastMessage());
		}
		try {
			for (Function function : currentProgram.getFunctionManager().getFunctions(true)) {
				monitor.checkCancelled();
				write(functionRecord(function, decompiler));
			}
		}
		finally {
			decompiler.dispose();
		}
	}

	private Json functionRecord(Function function, DecompInterface decompiler) {
		StringBuilder body = new StringBuilder("[");
		for (AddressRange range : function.getBody().getAddressRanges(true)) {
			if (body.length() > 1) {
				body.append(",");
			}
			body.append(new Json(null)
				.put("start", hex(range.getMinAddress()))
				.put("end", hex(range.getMaxAddress()))
				.toString());
		}

		// A callee may be an import, which lives in the synthetic EXTERNAL space,
		// so every target carries the space it belongs to.
		List<String> called = new ArrayList<>();
		for (Function callee : function.getCalledFunctions(monitor)) {
			called.add(target(callee.getEntryPoint()));
		}
		called.sort(Comparator.naturalOrder());

		Function thunked = function.getThunkedFunction(false);
		Json record = new Json("function")
			.put("entry", hex(function.getEntryPoint()))
			.put("name", function.getName())
			.put("namespace", function.getParentNamespace().getName(true))
			.put("prototype", function.getPrototypeString(true, true))
			.put("calling_convention", function.getCallingConventionName())
			.put("stack_purge", function.getStackPurgeSize())
			.put("thunk", function.isThunk())
			.raw("thunk_target",
				thunked == null ? "null" : target(thunked.getEntryPoint()))
			.raw("body", body.append("]").toString())
			.raw("called", Json.raws(called));

		DecompileResults results =
			decompiler.decompileFunction(function, DECOMPILE_TIMEOUT_SECONDS, monitor);
		if (results != null && results.decompileCompleted()
				&& results.getDecompiledFunction() != null) {
			return record.put("decompiler_status", "ok")
				.put("decompiler_c", results.getDecompiledFunction().getC())
				.put("decompiler_error", null);
		}
		String message = results == null ? "decompiler returned no result"
				: results.getErrorMessage();
		return record.put("decompiler_status", "failed")
			.put("decompiler_c", null)
			.put("decompiler_error",
				message == null || message.isEmpty() ? "decompilation did not complete" : message);
	}

	/** Coalesce the disassembly into maximal runs of back-to-back instructions. */
	private void emitInstructionRanges() throws Exception {
		Address start = null;
		Address end = null;
		String block = null;
		for (Instruction instruction : currentProgram.getListing().getInstructions(true)) {
			monitor.checkCancelled();
			Address at = instruction.getMinAddress();
			if (start != null && at.equals(end.next())) {
				end = instruction.getMaxAddress();
				continue;
			}
			if (start != null) {
				writeRange(start, end, block);
			}
			start = at;
			end = instruction.getMaxAddress();
			MemoryBlock owner = currentProgram.getMemory().getBlock(at);
			block = owner == null ? null : owner.getName();
		}
		if (start != null) {
			writeRange(start, end, block);
		}
	}

	private void writeRange(Address start, Address end, String block) {
		write(new Json("instruction_range")
			.put("start", hex(start))
			.put("end", hex(end))
			.put("block", block));
	}

	/**
	 * Emit the symbols that carry an address. `getAllSymbols(false)` omits
	 * dynamic symbols and the iterator walks symbols by address, so variable and
	 * parameter symbols are out of scope; `program.symbol_table_total` records
	 * the unfiltered count.
	 */
	private void emitSymbols() throws Exception {
		SymbolIterator symbols = currentProgram.getSymbolTable().getAllSymbols(false);
		while (symbols.hasNext()) {
			monitor.checkCancelled();
			Symbol symbol = symbols.next();
			// Imports live in Ghidra's synthetic EXTERNAL space, where the offset
			// is a slot index rather than an address in the image.
			write(new Json("symbol")
				.put("address", hex(symbol.getAddress()))
				.put("space", symbol.getAddress().getAddressSpace().getName())
				.put("name", symbol.getName())
				.put("namespace", symbol.getParentNamespace().getName(true))
				.put("type", symbol.getSymbolType().toString())
				.put("source", symbol.getSource().toString())
				.put("primary", symbol.isPrimary())
				.put("global", symbol.isGlobal()));
		}
	}

	private void emitDefinedData() throws Exception {
		for (Data data : currentProgram.getListing().getDefinedData(true)) {
			monitor.checkCancelled();
			if (data.hasStringValue()) {
				Object value = data.getValue();
				write(new Json("string")
					.put("address", hex(data.getMinAddress()))
					.put("data_type", data.getDataType().getName())
					.put("length", data.getLength())
					.put("value", value == null ? null : value.toString()));
				continue;
			}
			Symbol label = currentProgram.getSymbolTable().getPrimarySymbol(data.getMinAddress());
			write(new Json("data")
				.put("address", hex(data.getMinAddress()))
				.put("data_type", data.getDataType().getPathName())
				.put("length", data.getLength())
				.put("label", label == null ? null : label.getName()));
		}
	}

	private void emitReferences() throws Exception {
		ReferenceManager references = currentProgram.getReferenceManager();
		for (Address from : references.getReferenceSourceIterator(
				currentProgram.getMemory(), true)) {
			monitor.checkCancelled();
			for (Reference reference : references.getReferencesFrom(from)) {
				// A stack or register reference targets a synthetic space, so its
				// target is an offset in that space rather than an image address.
				write(new Json("reference")
					.put("from", hex(reference.getFromAddress()))
					.put("to", hex(reference.getToAddress()))
					.put("to_space", reference.getToAddress().getAddressSpace().getName())
					.put("type", reference.getReferenceType().getName())
					.put("operand", reference.getOperandIndex())
					.put("source", reference.getSource().toString())
					.put("primary", reference.isPrimary()));
			}
		}
	}

	/**
	 * Recover the virtual function tables the RTTI analyzer labelled. A slot is
	 * taken while it holds a pointer into an executable block and carries no
	 * label of its own, which is where the next table would begin.
	 */
	private void emitVtables() throws Exception {
		SymbolIterator symbols = currentProgram.getSymbolTable().getAllSymbols(false);
		while (symbols.hasNext()) {
			monitor.checkCancelled();
			Symbol symbol = symbols.next();
			if (!symbol.getName().startsWith("vftable")) {
				continue;
			}
			List<String> slots = new ArrayList<>();
			Address at = symbol.getAddress();
			while (true) {
				if (!slots.isEmpty()) {
					Symbol here = currentProgram.getSymbolTable().getPrimarySymbol(at);
					if (here != null) {
						break;
					}
				}
				Address target;
				try {
					target = addressOf(currentProgram.getMemory().getInt(at) & 0xFFFFFFFFL);
				}
				catch (Exception error) {
					break;
				}
				MemoryBlock block = currentProgram.getMemory().getBlock(target);
				if (block == null || !block.isExecute()) {
					break;
				}
				slots.add(hex(target));
				at = at.add(4);
			}
			write(new Json("vtable")
				.put("address", hex(symbol.getAddress()))
				.put("symbol", symbol.getName(true))
				.put("class", symbol.getParentNamespace().getName(true))
				.raw("slots", Json.array(slots)));
		}
	}

	private void emitRtti() throws Exception {
		for (Data data : currentProgram.getListing().getDefinedData(true)) {
			monitor.checkCancelled();
			String kind = data.getDataType().getName();
			if (!RTTI_TYPE_NAMES.contains(kind)) {
				continue;
			}
			Symbol label = currentProgram.getSymbolTable().getPrimarySymbol(data.getMinAddress());
			write(new Json("rtti")
				.put("address", hex(data.getMinAddress()))
				.put("kind", kind)
				.put("type_name", typeDescriptorName(data))
				.put("demangled", label == null ? null
					: label.getParentNamespace().getName(true))
				.put("symbol", label == null ? null : label.getName(true)));
		}
	}

	/**
	 * Read the decorated name out of an MSVC type descriptor, if this is one.
	 * The name comes from the structure's own `name` field rather than a fixed
	 * offset, so a layout change cannot turn this into silent garbage.
	 */
	private String typeDescriptorName(Data data) {
		if (!TYPE_DESCRIPTOR.equals(data.getDataType().getName())) {
			return null;
		}
		for (int index = 0; index < data.getNumComponents(); index++) {
			Data component = data.getComponent(index);
			if ("name".equals(component.getFieldName()) && component.getValue() != null) {
				return component.getValue().toString();
			}
		}
		return null;
	}

	/** Render one call or thunk target with the address space it belongs to. */
	private static String target(Address address) {
		return new Json(null)
			.put("address", hex(address))
			.put("space", address.getAddressSpace().getName())
			.toString();
	}

	private Address addressOf(long value) {
		return currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(value);
	}

	private static String hex(Address address) {
		return String.format("0x%08x", address.getOffset());
	}

	private void write(Json record) {
		out.print(record.toString());
		out.print("\n");
		if (out.checkError()) {
			throw new RuntimeException("failed writing the analysis stream");
		}
	}

	/** Minimal JSON object builder; the stream must stay dependency-free. */
	private static final class Json {
		private final StringBuilder text = new StringBuilder("{");

		Json(String tag) {
			if (tag != null) {
				put("tag", tag);
			}
		}

		private Json append(String key, String value) {
			if (text.length() > 1) {
				text.append(",");
			}
			text.append(quote(key)).append(":").append(value);
			return this;
		}

		Json put(String key, String value) {
			return append(key, value == null ? "null" : quote(value));
		}

		Json put(String key, long value) {
			return append(key, Long.toString(value));
		}

		Json put(String key, boolean value) {
			return append(key, Boolean.toString(value));
		}

		Json raw(String key, String json) {
			return append(key, json);
		}

		/** Join values that are already JSON into an array. */
		static String raws(List<String> values) {
			return "[" + String.join(",", values) + "]";
		}

		static String array(List<String> values) {
			StringBuilder result = new StringBuilder("[");
			for (String value : values) {
				if (result.length() > 1) {
					result.append(",");
				}
				result.append(quote(value));
			}
			return result.append("]").toString();
		}

		/** Split a newline-separated option value into its non-empty lines. */
		static List<String> lines(String packed) {
			List<String> values = new ArrayList<>();
			for (String value : packed.split("\n")) {
				if (!value.trim().isEmpty()) {
					values.add(value.trim());
				}
			}
			return values;
		}

		/** Zip two newline-separated option values into an array of name/reason objects. */
		static String pairs(String packedNames, String packedReasons) {
			if (packedNames.isEmpty()) {
				return "[]";
			}
			String[] names = packedNames.split("\n", -1);
			String[] reasons = packedReasons.split("\n", -1);
			StringBuilder result = new StringBuilder("[");
			for (int index = 0; index < names.length; index++) {
				if (index > 0) {
					result.append(",");
				}
				result.append(new Json(null)
					.put("name", names[index])
					.put("reason", index < reasons.length ? reasons[index] : "")
					.toString());
			}
			return result.append("]").toString();
		}

		/** Split a comma-separated option value into a JSON array. */
		static String stringList(String packed) {
			List<String> values = new ArrayList<>();
			for (String value : packed.split(",")) {
				if (!value.trim().isEmpty()) {
					values.add(value.trim());
				}
			}
			return array(values);
		}

		static String quote(String value) {
			StringBuilder result = new StringBuilder("\"");
			for (int index = 0; index < value.length(); index++) {
				char character = value.charAt(index);
				switch (character) {
					case '"' -> result.append("\\\"");
					case '\\' -> result.append("\\\\");
					case '\n' -> result.append("\\n");
					case '\r' -> result.append("\\r");
					case '\t' -> result.append("\\t");
					default -> {
						if (character < 0x20 || character > 0x7E) {
							result.append(String.format("\\u%04x", (int) character));
						}
						else {
							result.append(character);
						}
					}
				}
			}
			return result.append("\"").toString();
		}

		@Override
		public String toString() {
			return text + "}";
		}
	}
}
