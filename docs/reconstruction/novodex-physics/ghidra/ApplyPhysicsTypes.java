/* Import the generated ABI shim and apply it to the public surface of NxPhysics.dll.
 *
 * Runs after the first auto-analysis pass. It parses physics_x86_msvc.h into the
 * program's type manager, applies the derived prototypes to the exports, types
 * the `this` parameter of every slot of a recovered public vtable, reruns the
 * affected analyzers, and records what it managed to resolve.
 *
 * Every public type named by an export or a recovered public vtable must resolve;
 * anything left unresolved is recorded and makes normalize_ghidra.py reject the
 * stream rather than publish evidence that quietly lost a type.
 *
 * Usage: -postScript ApplyPhysicsTypes.java <physics_x86_msvc.h>
 */
//@category Novodex

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.TreeSet;

import ghidra.app.cmd.function.ApplyFunctionSignatureCmd;
import ghidra.app.script.GhidraScript;
import ghidra.app.util.cparser.C.CParserUtils;
import ghidra.framework.options.Options;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressIterator;
import ghidra.program.model.data.DataType;
import ghidra.program.model.data.DataTypeManager;
import ghidra.program.model.data.Enum;
import ghidra.program.model.data.FunctionDefinition;
import ghidra.program.model.data.Pointer;
import ghidra.program.model.data.Structure;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Parameter;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.SourceType;
import ghidra.program.model.symbol.Symbol;
import ghidra.program.model.symbol.SymbolIterator;

public class ApplyPhysicsTypes extends GhidraScript {

	/** Option names shared with ExportPhysicsAnalysis.java. */
	private static final String CATEGORY = "Novodex";
	private static final String HEADER_SHA256 = "type_coverage.header_sha256";
	private static final String TYPES_PARSED = "type_coverage.types_parsed";
	private static final String APPLIED_EXPORTS = "type_coverage.applied_exports";
	private static final String APPLIED_VTABLES = "type_coverage.applied_vtables";
	private static final String UNRESOLVED = "type_coverage.unresolved";
	private static final String EXPORTS_UNDECLARED =
		"type_coverage.exports_undeclared_in_headers";
	private static final String NON_EXPORT_ENTRY_POINTS =
		"type_coverage.non_export_entry_points";
	private static final String TYPED_VTABLE_SLOTS = "type_coverage.typed_vtable_slots";
	private static final String SKIPPED_VTABLE_SLOTS = "type_coverage.skipped_vtable_slots";
	private static final String SKIPPED_SLOT_REASONS = "type_coverage.skipped_slot_reasons";

	/** Marks a type the generator withheld; see generate_ghidra_types.py. */
	private static final String EXCLUDED_MARKER = "/* excluded type:";
	private static final String WITHHELD_NAMES = "type_coverage.excluded_type_names";
	private static final String WITHHELD_REASONS = "type_coverage.excluded_type_reasons";

	/** MSVC fixes every enum's underlying type at 4 bytes. */
	private static final int MSVC_ENUM_BYTES = 4;

	/** Ghidra's PE loader labels the image entry point `entry`; it is not an export. */
	private static final String IMAGE_ENTRY_LABEL = "entry";

	private final TreeSet<String> unresolved = new TreeSet<>();
	private final TreeSet<String> exportsUndeclared = new TreeSet<>();
	private final TreeSet<String> nonExportEntryPoints = new TreeSet<>();
	private final TreeSet<String> shimEnums = new TreeSet<>();
	private final TreeSet<String> skippedSlotReasons = new TreeSet<>();
	private final List<String> withheldNames = new ArrayList<>();
	private final List<String> withheldReasons = new ArrayList<>();
	private int typedSlots;
	private int skippedSlots;

	@Override
	public void run() throws Exception {
		String[] args = getScriptArgs();
		if (args.length != 1) {
			throw new IllegalArgumentException(
				"ApplyPhysicsTypes.java requires exactly one argument: the ABI shim header");
		}
		Path header = Paths.get(args[0]).toAbsolutePath();
		if (!Files.isRegularFile(header)) {
			throw new IllegalArgumentException("no ABI shim header at " + header);
		}

		readWithheldTypes(header);
		int parsed = parseHeader(header);
		int exports = applyExportPrototypes();
		int vtables = applyVtableThisTypes();

		// Rerun the reference, parameter and decompiler analyzers over what the
		// new types changed, so the exported stream reflects the typed view.
		analyzeChanges(currentProgram);

		Options options = currentProgram.getOptions(CATEGORY);
		options.setString(HEADER_SHA256, sha256(header));
		options.setInt(TYPES_PARSED, parsed);
		options.setInt(APPLIED_EXPORTS, exports);
		options.setInt(APPLIED_VTABLES, vtables);
		options.setString(UNRESOLVED, String.join(",", unresolved));
		options.setString(EXPORTS_UNDECLARED, String.join(",", exportsUndeclared));
		options.setString(NON_EXPORT_ENTRY_POINTS, String.join(",", nonExportEntryPoints));
		options.setInt(TYPED_VTABLE_SLOTS, typedSlots);
		options.setInt(SKIPPED_VTABLE_SLOTS, skippedSlots);
		options.setString(SKIPPED_SLOT_REASONS, String.join("\n", skippedSlotReasons));
		// Newline-separated: a type name cannot contain one, and each reason is a
		// single line by the time it is read back out of the shim.
		options.setString(WITHHELD_NAMES, String.join("\n", withheldNames));
		options.setString(WITHHELD_REASONS, String.join("\n", withheldReasons));

		println("types_parsed=" + parsed);
		println("applied_exports=" + exports);
		println("applied_vtables=" + vtables);
		println("typed_vtable_slots=" + typedSlots);
		println("skipped_vtable_slots=" + skippedSlots);
		println("exports_undeclared_in_headers=" + exportsUndeclared.size());
		println("non_export_entry_points=" + nonExportEntryPoints.size());
		println("excluded_types=" + withheldNames.size());
		if (!unresolved.isEmpty()) {
			throw new IllegalStateException(
				"public types left unresolved: " + String.join(",", unresolved));
		}
	}

	/**
	 * Read the exclusion markers and enum names out of the shim this pass applies.
	 *
	 * Taking them from the header rather than from physics_type_inputs.json keeps
	 * them tied to the exact file this pass applied, whose SHA-256 is recorded
	 * alongside them, and carries the reason into the manifest that Task 5 reads.
	 */
	private void readWithheldTypes(Path header) throws Exception {
		String name = null;
		StringBuilder reason = new StringBuilder();
		for (String line : Files.readAllLines(header)) {
			String text = line.trim();
			if (text.startsWith("enum ") && text.endsWith("{")) {
				shimEnums.add(text.substring("enum ".length(), text.length() - 1).trim());
			}
			if (text.startsWith(EXCLUDED_MARKER)) {
				name = text.substring(EXCLUDED_MARKER.length()).trim();
				reason.setLength(0);
				continue;
			}
			if (name == null) {
				continue;
			}
			if (text.equals("*/")) {
				withheldNames.add(name);
				withheldReasons.add(reason.toString().trim());
				name = null;
				continue;
			}
			reason.append(text).append(" ");
		}
		if (name != null) {
			throw new IllegalStateException(
				"the ABI shim has an unclosed '" + EXCLUDED_MARKER + " " + name
					+ "' marker, so a withheld type would go unrecorded");
		}
	}

	private int parseHeader(Path header) throws Exception {
		DataTypeManager manager = currentProgram.getDataTypeManager();
		int before = manager.getDataTypeCount(true);
		CParserUtils.CParseResults results = CParserUtils.parseHeaderFiles(
			new DataTypeManager[0], new String[] { header.toString() }, new String[0],
			manager, monitor);
		if (!results.successful()) {
			throw new IllegalStateException("the ABI shim did not parse: "
				+ results.getFormattedParseMessage("ApplyPhysicsTypes"));
		}
		verifyEnumWidths(manager);
		return manager.getDataTypeCount(true) - before;
	}

	/**
	 * MSVC gives every enum a 4-byte underlying type, and the generator pads each
	 * one to match. Reading the widths back turns that from an intention into a
	 * checked fact: a narrower enum would move every member after it.
	 *
	 * Only the shim's own enums are checked. The program also holds PE and DOS
	 * types the loader applied, and a narrow built-in among those would abort the
	 * typed pass with a message blaming the shim.
	 */
	private void verifyEnumWidths(DataTypeManager manager) {
		// The shim's enums are selected by matching its text, so an emitted form
		// this parser does not recognise would empty the set and let the check
		// pass having verified nothing. The shim always declares some.
		if (shimEnums.isEmpty()) {
			throw new IllegalStateException(
				"no enum was recovered from the ABI shim, so their widths cannot be "
					+ "verified: the shim's enum spelling has moved away from what "
					+ "this pass parses");
		}
		List<String> wrong = new ArrayList<>();
		Iterator<DataType> types = manager.getAllDataTypes();
		while (types.hasNext()) {
			DataType type = types.next();
			if (type instanceof Enum && shimEnums.contains(type.getName())
					&& type.getLength() != MSVC_ENUM_BYTES) {
				wrong.add(type.getName() + " is " + type.getLength() + " bytes");
			}
		}
		if (!wrong.isEmpty()) {
			throw new IllegalStateException(
				"the ABI shim declares enums that are not " + MSVC_ENUM_BYTES + " bytes: "
					+ String.join(", ", wrong));
		}
	}

	/** Give every export the prototype the shim derived for it, where one exists. */
	private int applyExportPrototypes() throws Exception {
		int applied = 0;
		AddressIterator entries =
			currentProgram.getSymbolTable().getExternalEntryPointIterator();
		for (Address entry : entries) {
			monitor.checkCancelled();
			Function function = currentProgram.getFunctionManager().getFunctionAt(entry);
			if (function == null) {
				// An entry point with no function is still an entry point; record
				// it rather than dropping it.
				nonExportEntryPoints.add(entry.toString());
				continue;
			}
			FunctionDefinition definition = findFunctionDefinition(function.getName());
			if (definition == null) {
				// The DLL exports symbols the public headers never declare; that
				// is evidence for the census, not a failure to resolve a type.
				// The image entry point is an entry point but not an export, so
				// it is recorded separately rather than counted as undeclared.
				if (IMAGE_ENTRY_LABEL.equals(function.getName())) {
					nonExportEntryPoints.add(function.getName());
				}
				else {
					exportsUndeclared.add(function.getName());
				}
				continue;
			}
			ApplyFunctionSignatureCmd command = new ApplyFunctionSignatureCmd(
				entry, definition, SourceType.IMPORTED);
			if (command.applyTo(currentProgram, monitor)) {
				applied++;
			}
			else {
				unresolved.add(function.getName() + " (prototype would not apply)");
			}
		}
		return applied;
	}

	/**
	 * Type the `this` parameter of every function a recovered public vtable
	 * points at. A vtable whose class the shim does not declare is recorded as
	 * unresolved rather than skipped.
	 */
	private int applyVtableThisTypes() throws Exception {
		int applied = 0;
		SymbolIterator symbols = currentProgram.getSymbolTable().getAllSymbols(false);
		List<Symbol> tables = new ArrayList<>();
		while (symbols.hasNext()) {
			Symbol symbol = symbols.next();
			if (symbol.getName().startsWith("vftable")) {
				tables.add(symbol);
			}
		}
		for (Symbol table : tables) {
			monitor.checkCancelled();
			String className = table.getParentNamespace().getName(false);
			Structure structure = findStructure(className);
			if (structure == null) {
				// Only classes the public headers declare are in scope; an
				// internal class has no public type to resolve against.
				if (isPublicClassName(className)) {
					unresolved.add(className);
				}
				continue;
			}
			DataType pointer = currentProgram.getDataTypeManager()
				.getPointer(structure);
			// Count what was actually typed. Counting tables instead would make a
			// run that typed no slot at all look identical to one that typed
			// every slot.
			for (Address slot : vtableSlots(table.getAddress())) {
				Function function = currentProgram.getFunctionManager().getFunctionAt(slot);
				Parameter self = function == null ? null : function.getParameter(0);
				if (self == null) {
					skip(slot, function == null ? "no function at slot"
						: "function declares no first parameter");
					continue;
				}
				try {
					self.setDataType(pointer, SourceType.IMPORTED);
					typedSlots++;
				}
				catch (Exception error) {
					skip(slot, error.getMessage());
				}
			}
			applied++;
		}
		return applied;
	}

	/** Record why one vtable slot went untyped, rather than only that it did. */
	private void skip(Address slot, String why) {
		skippedSlots++;
		skippedSlotReasons.add(slot + ": " + (why == null ? "unknown" : why));
	}

	/** Read a vtable's slots the same way ExportPhysicsAnalysis.java does. */
	private List<Address> vtableSlots(Address start) {
		List<Address> slots = new ArrayList<>();
		Address at = start;
		while (true) {
			if (!slots.isEmpty()
					&& currentProgram.getSymbolTable().getPrimarySymbol(at) != null) {
				break;
			}
			Address target;
			try {
				target = currentProgram.getAddressFactory().getDefaultAddressSpace()
					.getAddress(currentProgram.getMemory().getInt(at) & 0xFFFFFFFFL);
			}
			catch (Exception error) {
				break;
			}
			MemoryBlock block = currentProgram.getMemory().getBlock(target);
			if (block == null || !block.isExecute()) {
				break;
			}
			slots.add(target);
			at = at.add(4);
		}
		return slots;
	}

	/** The public SDK spells every published class with an Nx prefix. */
	private static boolean isPublicClassName(String name) {
		return name.startsWith("Nx");
	}

	private FunctionDefinition findFunctionDefinition(String name) {
		List<DataType> found = new ArrayList<>();
		currentProgram.getDataTypeManager().findDataTypes(name, found);
		for (DataType candidate : found) {
			if (candidate instanceof FunctionDefinition definition) {
				return definition;
			}
		}
		return null;
	}

	private Structure findStructure(String name) {
		List<DataType> found = new ArrayList<>();
		currentProgram.getDataTypeManager().findDataTypes(name, found);
		for (DataType candidate : found) {
			DataType resolved = candidate instanceof Pointer pointer
				? pointer.getDataType() : candidate;
			if (resolved instanceof Structure structure) {
				return structure;
			}
		}
		return null;
	}

	private static String sha256(Path path) throws Exception {
		MessageDigest digest = MessageDigest.getInstance("SHA-256");
		StringBuilder text = new StringBuilder();
		for (byte value : digest.digest(Files.readAllBytes(path))) {
			text.append(String.format("%02x", value));
		}
		return text.toString();
	}
}
