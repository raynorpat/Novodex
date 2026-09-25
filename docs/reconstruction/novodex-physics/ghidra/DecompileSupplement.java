/* Decompile census rows Ghidra never made functions, without changing the project.
 *
 * Run with -readOnly -noanalysis against the analysed PhysicsOracle project, so a
 * function created here exists only for this session. Decompiler settings match
 * ExportPhysicsAnalysis.java, so the output is comparable to the committed manifest.
 *
 * Usage: -postScript DecompileSupplement.java <output.json> <analysis_options_sha256> <rva>...
 */
//@category Novodex

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.framework.Application;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressRange;
import ghidra.program.model.listing.Function;

public class DecompileSupplement extends GhidraScript {

	private static final int DECOMPILE_TIMEOUT_SECONDS = 60;
	private static final String SIMPLIFICATION_STYLE = "decompile";

	@Override
	public void run() throws Exception {
		String[] args = getScriptArgs();
		if (args.length < 3) {
			throw new IllegalArgumentException(
				"usage: <output.json> <analysis_options_sha256> <rva>...");
		}
		Address base = currentProgram.getImageBase();
		DecompInterface decompiler = new DecompInterface();
		decompiler.setOptions(new DecompileOptions());
		decompiler.toggleCCode(true);
		decompiler.toggleSyntaxTree(false);
		decompiler.setSimplificationStyle(SIMPLIFICATION_STYLE);
		if (!decompiler.openProgram(currentProgram)) {
			throw new IllegalStateException(
				"the decompiler refused this program: " + decompiler.getLastMessage());
		}
		StringBuilder json = new StringBuilder();
		json.append("{\n \"schema_version\": 1,\n");
		json.append(" \"ghidra_version\": ").append(q(Application.getApplicationVersion())).append(",\n");
		json.append(" \"analysis_options_sha256\": ").append(q(args[1])).append(",\n");
		json.append(" \"decompiler_timeout_seconds\": ").append(DECOMPILE_TIMEOUT_SECONDS).append(",\n");
		json.append(" \"decompiler_simplification_style\": ").append(q(SIMPLIFICATION_STYLE)).append(",\n");
		json.append(" \"requested\": [");
		for (int i = 2; i < args.length; i++) {
			json.append(i > 2 ? ", " : "").append(q(hex(Long.decode(args[i]))));
		}
		json.append("],\n \"functions\": [\n");
		try {
			for (int i = 2; i < args.length; i++) {
				long rva = Long.decode(args[i]);
				json.append(record(rva, base.add(rva), decompiler));
				json.append(i + 1 < args.length ? ",\n" : "\n");
			}
		}
		finally {
			decompiler.dispose();
		}
		json.append(" ]\n}\n");
		Files.write(Paths.get(args[0]), json.toString().getBytes(StandardCharsets.UTF_8));
	}

	private String record(long rva, Address entry, DecompInterface decompiler) {
		Function function = getFunctionAt(entry);
		boolean created = false;
		if (function == null) {
			function = createFunction(entry, null);
			created = function != null;
		}
		StringBuilder r = new StringBuilder();
		r.append("  {\"rva\": ").append(q(hex(rva))).append(", \"created\": ").append(created);
		if (function == null) {
			r.append(", \"status\": \"create_failed\", \"prototype\": null, \"calling_convention\": null,")
				.append(" \"stack_purge\": null, \"body\": [], \"decompiler_c\": null,")
				.append(" \"error\": ").append(q("createFunction returned null")).append("}");
			return r.toString();
		}
		long imageBase = currentProgram.getImageBase().getOffset();
		r.append(", \"prototype\": ").append(q(function.getSignature().getPrototypeString()));
		r.append(", \"calling_convention\": ").append(q(function.getCallingConventionName()));
		r.append(", \"stack_purge\": ").append(function.getStackPurgeSize());
		r.append(", \"body\": [");
		boolean first = true;
		for (AddressRange range : function.getBody()) {
			r.append(first ? "" : ", ").append("[")
				.append(q(hex(range.getMinAddress().getOffset() - imageBase))).append(", ")
				.append(q(hex(range.getMaxAddress().getOffset() - imageBase + 1))).append("]");
			first = false;
		}
		r.append("]");
		DecompileResults results =
			decompiler.decompileFunction(function, DECOMPILE_TIMEOUT_SECONDS, monitor);
		if (results != null && results.decompileCompleted()
				&& results.getDecompiledFunction() != null) {
			r.append(", \"status\": \"ok\", \"decompiler_c\": ")
				.append(q(results.getDecompiledFunction().getC())).append(", \"error\": null}");
		}
		else {
			String error = results == null ? "no results" : results.getErrorMessage();
			r.append(", \"status\": \"decompile_failed\", \"decompiler_c\": null, \"error\": ")
				.append(q(error)).append("}");
		}
		return r.toString();
	}

	private static String hex(long value) {
		return String.format("0x%08x", value);
	}

	private static String q(String s) {
		if (s == null) {
			return "null";
		}
		StringBuilder out = new StringBuilder("\"");
		for (char c : s.toCharArray()) {
			switch (c) {
				case '"': out.append("\\\""); break;
				case '\\': out.append("\\\\"); break;
				case '\n': out.append("\\n"); break;
				case '\r': out.append("\\r"); break;
				case '\t': out.append("\\t"); break;
				default:
					if (c < 0x20) {
						out.append(String.format("\\u%04x", (int) c));
					}
					else {
						out.append(c);
					}
			}
		}
		return out.append('"').toString();
	}
}
