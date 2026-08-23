#!/usr/bin/env python3
"""Normalize Ghidra's raw analysis stream into deterministic oracle evidence.

The raw stream carries absolute addresses and Ghidra's whole metadata map, some
of which records the project path, the wall-clock time or the session state. This
tool converts addresses to RVAs, keeps only the metadata that survives a fresh
analysis, canonicalizes type strings and orders every table by address, so two
runs from clean Ghidra projects produce byte-identical output.
"""

import argparse
import json
import re
from pathlib import Path

import verify_toolchain


SCHEMA_VERSION = 1

# The pin sits beside the tools, so no flag is needed to find it.
PIN_FILENAME = "analysis_toolchain.json"

# The analyzer contract lives in verify_toolchain, so evidence cannot claim an
# analysis configuration the pin does not certify.
PINNED_ANALYZERS = verify_toolchain.DEFAULT_ANALYSIS_OPTIONS["analyzers"]

# Ghidra metadata worth keeping: the identity of the image and of the analysis,
# the totals a reader needs to detect a filtered table, and Ghidra's own verdicts.
# What is dropped is what a rerun would not reproduce - a project path, a
# wall-clock time, or a session state such as `Date Created`, `Executable
# Location`, `Program Name`, `FSRL`, `Analyzed` and `Should Ask To Analyze`.
RETAINED_METADATA_KEYS = (
    "# of Bytes",
    "# of Defined Data",
    "# of Functions",
    "# of Instructions",
    "# of Memory Blocks",
    "# of Symbols",
    "Address Size",
    "Compiler",
    "Compiler ID",
    "Created With Ghidra Version",
    "Endian",
    "Executable Format",
    "Executable MD5",
    "Executable SHA256",
    "Language ID",
    "Maximum Address",
    "Minimum Address",
    "PE Property[FileVersion]",
    "Processor",
    "Relocatable",
    # Ghidra's own verdict on whether the image carries MSVC RTTI. It decides
    # whether any vtable can be recovered by name, so it belongs in the evidence.
    "RTTI Found",
    # The section alignment is the whole explanation for the executable-extent
    # difference between this oracle and the PE oracle, so Task 5 needs it here.
    "SectionAlignment",
)

DECOMPILER_STATUSES = ("ok", "failed")

# Every field each record tag must carry, and no others. A whitelist means a new
# volatile field cannot reach the manifest by accident.
RECORD_FIELDS = {
    "program": ("name", "image_base", "language_id", "compiler_spec_id", "address_space",
                "ghidra_version", "decompiler_timeout_seconds",
                "decompiler_simplification_style", "decompiler_options", "symbol_table_total",
                "symbol_scope", "analysis_options", "metadata", "memory_blocks"),
    "type_coverage": ("header_sha256", "types_parsed", "applied_exports", "applied_vtables",
                      "typed_vtable_slots", "skipped_vtable_slots", "skipped_slot_reasons",
                      "unresolved", "exports_undeclared_in_headers",
                      "non_export_entry_points", "excluded_types"),
    "function": ("entry", "name", "namespace", "prototype", "calling_convention", "stack_purge",
                 "thunk", "thunk_target", "body", "called", "decompiler_status",
                 "decompiler_c", "decompiler_error"),
    "instruction_range": ("start", "end", "block"),
    "symbol": ("address", "space", "name", "namespace", "type", "source", "primary", "global"),
    "string": ("address", "data_type", "length", "value"),
    "reference": ("from", "to", "to_space", "type", "operand", "source", "primary"),
    "vtable": ("address", "symbol", "class", "slots"),
    "rtti": ("address", "kind", "type_name", "demangled", "symbol"),
    "data": ("address", "data_type", "length", "label"),
}


def normalize_type(text):
    """Canonicalize a Ghidra type or prototype string.

    Ghidra appends `.conflict` suffixes when two archives declare the same name,
    and spaces declarators inconsistently; neither carries meaning about layout.
    """
    if text is None:
        return None
    text = re.sub(r"\.conflict\d*\b", "", text)
    text = re.sub(r"\s+", " ", text).strip()
    text = re.sub(r"\[\s*", "[", text)
    return re.sub(r"\s*\]", "]", text)


def normalize_newlines(text):
    """Collapse the host line separator Ghidra's PrettyPrinter emits.

    Decompiler output ends every line with `System.getProperty("line.separator")`,
    so leaving it alone would make the manifest hash depend on the host OS rather
    than on the binary.
    """
    if text is None:
        return None
    return text.replace("\r\n", "\n").replace("\r", "\n")


def parse_records(text) -> list:
    """Read the JSON-lines stream, rejecting any record the schema does not define."""
    records = []
    for number, line in enumerate(text.splitlines(), start=1):
        if not line.strip():
            continue
        try:
            record = json.loads(line)
        except json.JSONDecodeError as error:
            raise ValueError(f"line {number} is not valid JSON: {error}") from error
        tag = record.get("tag")
        if tag is None:
            raise ValueError(f"line {number} has no tag")
        if tag not in RECORD_FIELDS:
            raise ValueError(f"line {number} has unknown tag {tag!r}")
        expected = set(RECORD_FIELDS[tag])
        present = set(record) - {"tag"}
        for field in sorted(expected - present):
            raise ValueError(f"line {number}: {tag} record is missing {field!r}")
        for field in sorted(present - expected):
            raise ValueError(f"line {number}: {tag} record has unexpected field {field!r}")
        records.append(record)
    return records


def _offset(value):
    if not isinstance(value, str) or not re.fullmatch(r"0[xX][0-9a-fA-F]+", value.strip()):
        raise ValueError(f"{value!r} is not a hexadecimal address")
    return int(value, 16)


def _rva_reader(image_base):
    def rva(value):
        offset = _offset(value)
        if offset < image_base:
            raise ValueError(
                f"address {value} is below image base 0x{image_base:08x}")
        return offset - image_base
    return rva


def _hex(value):
    return f"0x{value:08x}"


def _single(records, tag):
    found = [record for record in records if record["tag"] == tag]
    if len(found) != 1:
        raise ValueError(f"the stream has {len(found)} {tag} records")
    return found[0]


def load_pin(path=None) -> dict:
    """Read the recorded toolchain pin this evidence must have been produced under."""
    path = Path(path) if path else Path(__file__).resolve().parent.parent / PIN_FILENAME
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except OSError as error:
        raise ValueError(f"cannot read the toolchain pin: {error}") from error


def check_against_pin(program, pin):
    """Reject a stream the pinned toolchain and analysis contract did not produce.

    The pin is compared, not echoed: the recorded Ghidra version, language and
    compiler spec must match, every pinned analyzer must be present and hold its
    pinned value, and the canonical hash of the options actually in force must
    equal the one the pin records.
    """
    expected = pin.get("analysis_options") or {}
    for field, actual in (("ghidra.version", program["ghidra_version"]),):
        recorded = (pin.get("ghidra") or {}).get("version")
        if recorded != actual:
            raise ValueError(f"{field} is {actual!r} but the pin records {recorded!r}")
    for field, key in (("language_id", "language_id"), ("compiler_spec_id", "compiler_spec_id")):
        if expected.get(key) != program[field]:
            raise ValueError(
                f"{field} is {program[field]!r} but the pin records {expected.get(key)!r}")

    options = program["analysis_options"]
    for name, value in sorted(PINNED_ANALYZERS.items()):
        if name not in options:
            raise ValueError(f"analysis options do not name the pinned analyzer {name!r}")
        if options[name] != value:
            raise ValueError(
                f"analysis option {name!r} is {options[name]!r} but the pin requires {value!r}")

    effective = dict(expected, analyzers=options)
    digest = verify_toolchain.canonical_options_sha256(effective)
    if digest != pin.get("analysis_options_sha256"):
        raise ValueError(
            f"analysis-options hash is {digest} but the pin records "
            f"{pin.get('analysis_options_sha256')!r}")


def _blocks(program, rva):
    blocks = []
    for block in program["memory_blocks"]:
        start = rva(block["start"])
        # Ghidra reports an inclusive end; evidence records a half-open interval.
        end = rva(block["end"]) + 1
        blocks.append({
            "name": block["name"],
            "rva": _hex(start),
            "end_rva": _hex(end),
            "size": end - start,
            "executable": block["executable"],
            "initialized": block["initialized"],
        })
    return sorted(blocks, key=lambda block: block["rva"])


def _function(record, rva, spans, image_base, image_space):
    entry = rva(record["entry"])
    body = []
    for chunk in record["body"]:
        start = rva(chunk["start"])
        end = rva(chunk["end"]) + 1
        if end <= start:
            raise ValueError(
                f"function {_hex(entry)} has body range {_hex(start)}-{_hex(end - 1)} "
                "ending before it starts")
        if not any(span[0] <= start and end <= span[1] for span in spans):
            raise ValueError(
                f"function {_hex(entry)} body range {_hex(start)}-{_hex(end)} is outside "
                "every memory block")
        body.append({"rva": _hex(start), "end_rva": _hex(end), "size": end - start})
    body.sort(key=lambda chunk: chunk["rva"])

    if record["decompiler_status"] not in DECOMPILER_STATUSES:
        raise ValueError(
            f"function {_hex(entry)} has unknown decompiler status "
            f"{record['decompiler_status']!r}")

    return {
        "rva": _hex(entry),
        "name": record["name"],
        "namespace": record["namespace"],
        "prototype": normalize_type(record["prototype"]),
        "calling_convention": record["calling_convention"],
        "stack_purge": record["stack_purge"],
        "thunk": record["thunk"],
        "thunk_target": None if record["thunk_target"] is None else _target(
            record["thunk_target"]["address"], record["thunk_target"]["space"],
            image_base, image_space),
        "body": body,
        "body_bytes": sum(chunk["size"] for chunk in body),
        "called": sorted(
            (_target(entry["address"], entry["space"], image_base, image_space)
             for entry in record["called"]), key=_target_key),
        "decompiler_status": record["decompiler_status"],
        # Ghidra's PrettyPrinter ends each line with the host's separator, so
        # leaving it in would make the manifest hash depend on the host OS.
        "decompiler_c": normalize_newlines(record["decompiler_c"]),
        # Decompiler error text carries embedded separators too.
        "decompiler_error": normalize_newlines(record["decompiler_error"]),
    }


def _target(address, space, image_base, image_space):
    """Describe a call, thunk or reference target.

    A target becomes an RVA only when it lies in the image's own address space at
    or above the image base. An import lives in Ghidra's EXTERNAL space and a
    stack slot in the stack space, so both keep a space-local offset instead of
    an address the image does not have.
    """
    offset = _offset(address)
    if space == image_space and offset >= image_base:
        return {"rva": _hex(offset - image_base), "space": space, "space_offset": None}
    return {"rva": None, "space": space, "space_offset": offset}


def _target_key(target):
    return (target["rva"] is None, target["rva"] or "", target["space"],
            target["space_offset"] or 0)


def _reference(record, rva, image_base, image_space):
    """Normalize one reference, keeping non-image targets as space-local offsets."""
    target = _target(record["to"], record["to_space"], image_base, image_space)
    return {
        "from_rva": _hex(rva(record["from"])),
        "to_rva": target["rva"],
        "to_space": target["space"],
        "to_space_offset": target["space_offset"],
        "type": record["type"],
        "operand": record["operand"],
        "source": record["source"],
        "primary": record["primary"],
    }


def build_manifest(records, pin=None) -> dict:
    """Turn the raw stream into the normalized, order-stable oracle manifest."""
    program = _single(records, "program")
    coverage = _single(records, "type_coverage")
    check_against_pin(program, pin if pin is not None else load_pin())
    if coverage["unresolved"]:
        raise ValueError(
            "type coverage left public types unresolved: "
            + ", ".join(sorted(coverage["unresolved"])))

    image_base = _offset(program["image_base"])
    image_space = program["address_space"]
    rva = _rva_reader(image_base)
    blocks = _blocks(program, rva)
    spans = [(_offset(block["rva"]), _offset(block["end_rva"])) for block in blocks]

    functions = sorted(
        (_function(record, rva, spans, image_base, image_space)
         for record in records if record["tag"] == "function"),
        key=lambda function: function["rva"])
    seen = set()
    for function in functions:
        if function["rva"] in seen:
            raise ValueError(f"two functions share entry {function['rva']}")
        seen.add(function["rva"])

    ranges = sorted(
        ({"rva": _hex(rva(record["start"])),
          "end_rva": _hex(rva(record["end"]) + 1),
          "size": rva(record["end"]) + 1 - rva(record["start"]),
          "block": record["block"]}
         for record in records if record["tag"] == "instruction_range"),
        key=lambda entry: entry["rva"])

    # An import lives in Ghidra's EXTERNAL space, where the offset is a slot
    # index. Such a symbol keeps its place in the census with a null RVA rather
    # than being dropped or given an address the image does not have.
    symbols = sorted(
        ({"rva": _hex(rva(record["address"])) if record["space"] == image_space else None,
          "space": record["space"], "name": record["name"],
          "namespace": record["namespace"], "type": record["type"],
          "source": record["source"], "primary": record["primary"], "global": record["global"]}
         for record in records if record["tag"] == "symbol"),
        key=lambda entry: (entry["rva"] is None, entry["rva"] or entry["space"],
                           entry["name"], entry["namespace"]))

    strings = sorted(
        ({"rva": _hex(rva(record["address"])), "data_type": normalize_type(record["data_type"]),
          "length": record["length"], "value": record["value"]}
         for record in records if record["tag"] == "string"),
        key=lambda entry: entry["rva"])

    # A stack or register reference targets a synthetic space; its target is an
    # offset there, so it carries the space instead of an RVA.
    references = sorted(
        (_reference(record, rva, image_base, image_space)
         for record in records if record["tag"] == "reference"),
        key=lambda entry: (entry["from_rva"], entry["to_rva"] is None, entry["to_rva"] or "",
                           entry["to_space"], entry["to_space_offset"] or 0,
                           entry["type"], entry["operand"]))

    vtables = sorted(
        ({"rva": _hex(rva(record["address"])), "symbol": record["symbol"],
          "class": record["class"],
          "slots": [_hex(rva(slot)) for slot in record["slots"]]}
         for record in records if record["tag"] == "vtable"),
        key=lambda entry: entry["rva"])

    rtti = sorted(
        ({"rva": _hex(rva(record["address"])), "kind": record["kind"],
          "type_name": record["type_name"], "demangled": record["demangled"],
          "symbol": record["symbol"]}
         for record in records if record["tag"] == "rtti"),
        key=lambda entry: entry["rva"])

    data = sorted(
        ({"rva": _hex(rva(record["address"])), "data_type": normalize_type(record["data_type"]),
          "length": record["length"], "label": record["label"]}
         for record in records if record["tag"] == "data"),
        key=lambda entry: entry["rva"])

    executable_bytes = sum(block["size"] for block in blocks if block["executable"])
    return {
        "schema_version": SCHEMA_VERSION,
        "ghidra": {
            "version": program["ghidra_version"],
            "language_id": program["language_id"],
            "compiler_spec_id": program["compiler_spec_id"],
            "analysis_options": {name: program["analysis_options"][name]
                                 for name in sorted(PINNED_ANALYZERS)},
            "metadata": {key: program["metadata"][key] for key in RETAINED_METADATA_KEYS
                         if key in program["metadata"]},
        },
        "decompiler": {
            "timeout_seconds": program["decompiler_timeout_seconds"],
            "simplification_style": program["decompiler_simplification_style"],
            "options": program["decompiler_options"],
        },
        "image": {"name": program["name"], "image_base": program["image_base"]},
        "symbol_scope": {"description": program["symbol_scope"],
                         "symbol_table_total": program["symbol_table_total"]},
        "memory_blocks": blocks,
        "type_coverage": {key: coverage[key] for key in RECORD_FIELDS["type_coverage"]},
        "functions": functions,
        "instruction_ranges": ranges,
        "symbols": symbols,
        "strings": strings,
        "references": references,
        "vtables": vtables,
        "rtti": rtti,
        "data": data,
        "counts": {
            "functions": len(functions), "instruction_ranges": len(ranges),
            "symbols": len(symbols), "strings": len(strings), "references": len(references),
            "vtables": len(vtables), "rtti": len(rtti), "data": len(data),
            # Made explicit so a reader can tell how much of the census sits
            # outside the image rather than inferring it from null RVAs.
            "external_symbols": sum(1 for entry in symbols if entry["rva"] is None),
            "non_image_references": sum(1 for entry in references
                                        if entry["to_rva"] is None),
        },
        "coverage": {
            "executable_bytes": executable_bytes,
            "function_body_bytes": sum(function["body_bytes"] for function in functions),
            "instruction_bytes": sum(entry["size"] for entry in ranges),
            "decompiled_functions": sum(1 for function in functions
                                        if function["decompiler_status"] == "ok"),
            "failed_functions": sum(1 for function in functions
                                    if function["decompiler_status"] == "failed"),
        },
    }


def serialize(manifest) -> str:
    return json.dumps(manifest, separators=(",", ":")) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", required=True, help="raw Ghidra JSON-lines stream")
    parser.add_argument("--output", required=True, help="normalized manifest JSON to write")
    args = parser.parse_args()

    try:
        manifest = build_manifest(
            parse_records(Path(args.input).read_text(encoding="utf-8")))
        output = Path(args.output)
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(serialize(manifest), encoding="utf-8", newline="\n")
    except (OSError, ValueError) as error:
        parser.exit(2, f"error: {error}\n")

    counts = manifest["counts"]
    coverage = manifest["coverage"]
    print("ghidra=written")
    print(f"version={manifest['ghidra']['version']}")
    for name in ("functions", "instruction_ranges", "symbols", "strings",
                 "references", "vtables", "rtti", "data"):
        print(f"{name}={counts[name]}")
    print(f"decompiled={coverage['decompiled_functions']}")
    print(f"failed={coverage['failed_functions']}")
    print(f"function_body_bytes={coverage['function_body_bytes']}")
    print(f"instruction_bytes={coverage['instruction_bytes']}")
    print(f"executable_bytes={coverage['executable_bytes']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
