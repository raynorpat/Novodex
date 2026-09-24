#!/usr/bin/env python3
"""Reconcile the PE, Ghidra and Capstone oracles into one stable-ID inventory.

Three oracles were built independently and none of them is amended here. The
reconciliation paints every executable byte with the one owner the evidence
supports, in a fixed precedence, and whatever no rule reaches stays unexplained
rather than being handed to the nearest plausible function. The same painting
runs over the referenced bytes of the non-executable image, so that a data
object, a resource, an import structure, relocation metadata or proven padding
owns each one. Stable IDs come from sorted entry RVAs and survive label changes.
"""

import argparse
import bisect
import json
from pathlib import Path


SCHEMA_VERSION = 1

# Byte classes the executable census paints.
CODE, TABLE, DATA, PADDING, UNREFERENCED = 1, 2, 3, 4, 5
CODE_CLASSES = (CODE, UNREFERENCED)
DATA_ROW_CLASSES = (TABLE, DATA)

# int3 and the multi-byte NOP encodings MSVC emits for alignment. Capstone
# decodes the lea/mov forms as themselves, so a sweep looking for the `nop`
# mnemonic alone calls a gap holding them code; they are listed by encoding
# instead, longest first so the walk is greedy.
NOP_ENCODINGS = tuple(sorted((
    bytes.fromhex("90"), bytes.fromhex("cc"),
    bytes.fromhex("6690"), bytes.fromhex("8bff"),
    bytes.fromhex("0f1f00"), bytes.fromhex("8d4900"),
    bytes.fromhex("0f1f4000"), bytes.fromhex("8d642400"),
    bytes.fromhex("0f1f440000"),
    bytes.fromhex("660f1f440000"), bytes.fromhex("8d9b00000000"),
    bytes.fromhex("0f1f800000 0000".replace(" ", "")),
    bytes.fromhex("8da42400000000"),
    bytes.fromhex("0f1f840000000000"), bytes.fromhex("660f1f840000000000"),
), key=len, reverse=True))

# What a ghidra_ref resolves to where Ghidra maps the bytes and claims no more.
# The block is looked up by address: memory_blocks[0] is the PE headers, and a
# reference to it would name a block that does not contain the row it backs.
MEMORY_BLOCK = "#/memory_blocks/"

# The signature MSVC's C++ exception runtime parks in .text beside __SEH_epilog.
MSVC_EH_SIGNATURE = "VC20XC00"

# Ghidra reference types that are a control-flow edge rather than a data access.
CALL_REFERENCES = frozenset({"UNCONDITIONAL_CALL", "COMPUTED_CALL",
                             "COMPUTED_CALL_TERMINATOR"})
FLOW_REFERENCES = CALL_REFERENCES | frozenset({
    "UNCONDITIONAL_JUMP", "CONDITIONAL_JUMP", "COMPUTED_JUMP"})

# Names are the phase plan titles, so this table and program.json agree. Phase 8
# is the full semantic audit; owning the compiler and runtime artifacts is part
# of that audit rather than the whole of it, which is why ARTIFACT_PHASE is 8.
PHASES = (
    (1, "Oracle census"),
    (2, "SDK core"),
    (3, "Geometry and collision"),
    (4, "Mesh and spatial assets"),
    (5, "Objects"),
    (6, "Joints and effectors"),
    (7, "Scenes and simulation"),
    (8, "Full semantic audit"),
)
SHARED_PHASE = 2
ARTIFACT_PHASE = 8

# The umbrella plan locks the initial phase owner of every named export.
EXPORT_PHASES = {name: 2 for name in (
    "NxCreatePhysicsSDK", "NxFluidAssert", "NxFluidDebugAABB", "NxFluidDebugArrow",
    "NxFluidDebugLine", "NxFluidDebugPoint", "NxFluidDebugSphere",
    "NxFluidDebugTriangle", "NxFluidFree", "NxFluidPAlloc")}
EXPORT_PHASES.update({name: 3 for name in (
    "NxBoxBoxIntersect", "NxBuildSmoothNormals", "NxComputeBoxDensity",
    "NxComputeBoxInertiaTensor", "NxComputeBoxMass", "NxComputeConeDensity",
    "NxComputeConeMass", "NxComputeCylinderDensity", "NxComputeCylinderMass",
    "NxComputeEllipsoidDensity", "NxComputeEllipsoidMass", "NxComputeSphereDensity",
    "NxComputeSphereInertiaTensor", "NxComputeSphereMass", "NxRayAABBIntersect",
    "NxRayAABBIntersect2", "NxRayCapsuleIntersect", "NxRayOBBIntersect",
    "NxRayPlaneIntersect", "NxRaySphereIntersect", "NxRayTriIntersect",
    "NxSegmentAABBIntersect", "NxSegmentBoxIntersect", "NxSegmentOBBIntersect",
    "NxSegmentPlaneIntersect", "NxSeparatingAxis", "NxSweptSpheresIntersect")})
EXPORT_PHASES.update({"NxCreatePMap": 4, "NxReleasePMap": 4,
                      "NxJointDesc_SetGlobalAnchor": 6,
                      "NxJointDesc_SetGlobalAxis": 6})

# The tail of each NX_ASSERT __FILE__ string the image carries, and the phase
# that owns the component it names. A function that references one of these was
# compiled from that translation unit, which is the only direct component
# evidence the image holds; everything else is reached through the call graph.
SOURCE_PHASES = {
    "PhysicsSDK.cpp": 2, "NpPhysicsSDK.cpp": 2,
    "CapsuleShape.cpp": 3, "SphereShape.cpp": 3, "Shape.cpp": 3,
    "NpBoxShape.cpp": 3, "NpCapsuleShape.cpp": 3, "NpPlaneShape.cpp": 3,
    "NpSphereShape.cpp": 3, "NpTriangleMeshShape.cpp": 3,
    "ContactBoxMeshICE.cpp": 3, "ContactConvexHeightfield.cpp": 3,
    "ContactMeshHeightfield.cpp": 3, "ContactMeshMesh.cpp": 3,
    "ContactPlaneMesh.cpp": 3, "ConvexHull.cpp": 3,
    "EdgeList.cpp": 4, "IceAdjacencies.cpp": 4, "InternalTriangleMesh.cpp": 4,
    "NpTriangleMesh.cpp": 4, "TriangleMesh.cpp": 4, "PenetrationMap.cpp": 4,
    "opcode\\IcePrunable.cpp": 4, "opcode\\OPC_MeshInterface.cpp": 4,
    "opcode\\OPC_Model.cpp": 4,
    "Actor.cpp": 5, "NpActor.cpp": 5, "Controller.cpp": 5,
    "Joint.cpp": 6, "D6Joint.cpp": 6, "NpSpringAndDamperEffector.cpp": 6,
    "core\\Articulation.cpp": 6, "core\\CylindricalJoint.cpp": 6,
    "core\\DistanceJoint.cpp": 6, "core\\FixedJoint.cpp": 6,
    "core\\NpCylindricalJoint.cpp": 6, "core\\NpD6Joint.cpp": 6,
    "core\\NpDistanceJoint.cpp": 6, "core\\NpFixedJoint.cpp": 6,
    "core\\NpPointInPlaneJoint.cpp": 6, "core\\NpPointOnLineJoint.cpp": 6,
    "core\\NpPrismaticJoint.cpp": 6, "core\\NpPulleyJoint.cpp": 6,
    "core\\NpRevoluteJoint.cpp": 6, "core\\NpSphericalJoint.cpp": 6,
    "core\\PointInPlaneJoint.cpp": 6, "core\\PointOnLineJoint.cpp": 6,
    "core\\PrismaticJoint.cpp": 6, "core\\PulleyJoint.cpp": 6,
    "core\\RevoluteJoint.cpp": 6, "core\\SphericalJoint.cpp": 6,
    "Scene.cpp": 7, "NpScene.cpp": 7, "SceneRaycast.cpp": 7,
    "fluids\\Fluid.cpp": 7, "fluids\\FluidManager.cpp": 7,
    "fluids\\ImplicitMesh.cpp": 7, "fluids\\NpFluid.cpp": 7,
    "fluids\\NpFluidEmitter.cpp": 7, "fluids\\NpImplicitMesh.cpp": 7,
}
SOURCE_MARK = "SDKs" + chr(92) + "Physics" + chr(92) + "src" + chr(92)

# Owner classes for the non-executable image, highest precedence first. Every
# byte a reference names has to land in one of them.
# How a row's phase was decided, published on the row so a phase worker can
# tell evidence from propagation without re-deriving the assignment. The nine
# names `assign_phases` records, plus the three decisions taken over rows no
# layer reaches: padding, the PE structures pinned to shared runtime, and a data
# object phased from the code that reads it.
PHASE_PROVENANCE = ("runtime_artifact", "runtime_tail", "translation_unit",
                    "enclosed_by_one_phase", "callers", "layout_adjacency",
                    "shared_by_callers", "export_pin",
                    "padding", "pe_structure", "reading_sites", "slot_ruling")

# The field order every data row is written in.
DATA_ROW_KEYS = ("id", "rva", "size", "type", "owner", "references", "section",
                 "phase", "phase_provenance", "state", "source", "label",
                 "label_confidence", "structural_proof", "notes")

# Structures the PE loader owns. Whichever component happens to read one says
# nothing about who owns it, so they are pinned to shared runtime rather than
# phased from their reading site.
PE_STRUCTURAL_CLASSES = frozenset({"relocation_metadata", "export_directory",
                                   "resource", "import_address_table"})

DATA_CLASSES = ("relocation_metadata", "export_directory", "resource",
                "import_address_table", "dispatch_table", "pointer_slot", "string",
                "ghidra_data", "code_addressed_global")


def hexa(value):
    return f"0x{value:08x}"


def rva(value):
    """Read an RVA the oracles write either as an int or as a hex string."""
    return value if isinstance(value, int) else int(value, 16)


def runs(state, lo, hi, wanted=None):
    """Yield maximal (start, end, value) runs of equal class over state[lo:hi]."""
    start = lo
    for index in range(lo + 1, hi + 1):
        if index == hi or state[index] != state[start]:
            if wanted is None or state[start] in wanted:
                yield start, index, state[start]
            start = index


def padding_runs(buffer, lo, hi):
    """Yield maximal (start, end) runs of int3 and canonical NOP encodings."""
    index = lo
    while index < hi:
        end = index
        while end < hi:
            for encoding in NOP_ENCODINGS:
                width = len(encoding)
                if end + width <= hi and buffer[end:end + width] == encoding:
                    end += width
                    break
            else:
                break
        if end > index:
            yield index, end
            index = end
        else:
            index += 1


class Locator:
    """Answer 'which record covers this address' without a linear scan per row."""

    def __init__(self, records):
        self.records = sorted(records)
        self.starts = [start for start, _, _ in self.records]

    def at(self, address):
        index = bisect.bisect_right(self.starts, address) - 1
        if index >= 0:
            start, end, value = self.records[index]
            if start <= address < end:
                return value
        return None


def check_schema(kind, document):
    if document.get("schema_version") != SCHEMA_VERSION:
        raise ValueError(f"the {kind} oracle records schema version "
                         f"{document.get('schema_version')} but this tool reads "
                         f"{SCHEMA_VERSION}")


def check_images(pe, capstone, inventory):
    expected = pe["image"]["sha256"]
    if capstone["image"]["sha256"] != expected:
        raise ValueError(f"the capstone oracle describes image "
                         f"{capstone['image']['sha256']} but the PE oracle "
                         f"describes {expected}")
    if inventory["pins"]["oracle"]["sha256"] != expected:
        raise ValueError(f"the inventory pins image "
                         f"{inventory['pins']['oracle']['sha256']} but the PE "
                         f"oracle describes {expected}")


def check_extent(pe, capstone):
    """The corpus must have disassembled the extent the PE oracle measured."""
    measured = sum(interval["size"] for interval in pe["executable_intervals"])
    covered = sum(interval["size"] for interval in capstone["executable_intervals"])
    if covered != measured:
        raise ValueError(f"the capstone oracle covers {covered} executable bytes "
                         f"but the PE oracle records {measured}")
    return measured


def check_corpus_tiling(chunks, gaps, measured):
    accounted = sum(end - start for start, end in chunks + gaps)
    if accounted != measured:
        raise ValueError(f"the capstone corpus accounts for {accounted} of "
                         f"{measured} executable bytes")


def check_gap_bytes(gap):
    published = len(gap["bytes"]) // 2
    extent = rva(gap["end_rva"]) - rva(gap["rva"])
    if published != extent:
        raise ValueError(f"coverage gap {gap['rva']} publishes {published} bytes "
                         f"for an extent of {extent}")


def check_function_bodies(functions):
    """Ghidra bodies are the strongest ownership evidence, so they must be disjoint."""
    owned = sorted((rva(extent["rva"]), rva(extent["end_rva"]), function["rva"])
                   for function in functions for extent in function["body"])
    for (_, end, earlier), (start, _, later) in zip(owned, owned[1:]):
        if end > start:
            raise ValueError(f"Ghidra function {later} body overlaps {earlier}")


def check_unexplained(state, base):
    unexplained = state.count(0)
    if unexplained:
        first = next(start for start, _, value in runs(state, 0, len(state))
                     if value == 0)
        raise ValueError(f"the census leaves {unexplained} executable bytes "
                         f"unexplained, the first run at {hexa(base + first)}")


def check_targets(unresolved):
    if unresolved:
        raise ValueError(f"{len(unresolved)} recovered executable targets are "
                         f"unresolved, the first being {hexa(min(unresolved))}")


def check_entries_are_owned(missing):
    if missing:
        raise ValueError(f"{len(missing)} function entries the evidence names are "
                         f"not owned rows, the first being {hexa(missing[0])}")


def check_stable_ids(rows):
    seen = set()
    for row in rows:
        if row["id"] in seen:
            raise ValueError(f"stable ID {row['id']!r} is assigned more than once")
        seen.add(row["id"])


def check_row_references(rows):
    for row in rows:
        for reference in ("ghidra_ref", "capstone_ref"):
            if not row.get(reference):
                raise ValueError(f"function {row['id']!r} records no {reference}")


def check_label_ledger(rows, ledger):
    for entry in ledger:
        for field in ("evidence", "reason"):
            if not entry.get(field):
                raise ValueError(f"label {entry['id']!r} records no {field}")
    carried = {entry["id"] for entry in ledger}
    for row in rows:
        if row["label_confidence"] != "stable-id" and row["id"] not in carried:
            raise ValueError(f"{row['id']!r} carries a semantic label without a "
                             f"ledger entry")


def measure_ownership(rows):
    """Count overlapping row pairs and bytes more than one row claims."""
    extents = sorted((int(row["rva"], 16), int(row["rva"], 16) + row["size"])
                     for row in rows)
    overlaps, duplicated, reach = 0, 0, 0
    for start, end in extents:
        if start < reach:
            overlaps += 1
            duplicated += min(end, reach) - start
        reach = max(reach, end)
    return overlaps, duplicated


def check_translation_unit_spans(spans):
    """The linker emits one object's code contiguously, and the seeds say so.

    The 57 files this image names occupy 57 address spans that neither overlap
    nor interleave, which is what makes a span evidence about every entry inside
    it. Two spans that interleave would mean the assumption had stopped holding
    and every layout rule below would be reading noise.
    """
    for (low, high, name), (next_low, _, next_name) in zip(spans, spans[1:]):
        if next_low <= high:
            raise ValueError(f"translation unit {name} occupies {hexa(low)} to "
                             f"{hexa(high)}, which {next_name} interleaves at "
                             f"{hexa(next_low)}")


def check_census(coverage):
    for name in ("unexplained_executable_bytes", "unresolved_executable_targets",
                 "unexplained_referenced_data_bytes", "overlaps",
                 "duplicate_ownership"):
        if coverage[name]:
            raise ValueError(f"census passes but {name} is {coverage[name]}")


class Extent:
    """The executable extent, painted exactly one owner deep."""

    def __init__(self, pe, capstone):
        self.base = min(interval["rva"] for interval in pe["executable_intervals"])
        self.end = max(interval["end_rva"] for interval in pe["executable_intervals"])
        self.size = check_extent(pe, capstone)
        self.state = bytearray(self.end - self.base)
        self.detail = {}

    def contains(self, address):
        return self.base <= address < self.end

    def free(self, address):
        return self.contains(address) and not self.state[address - self.base]

    def paint(self, start, end, value, detail=None):
        """Claim [start, end) for value and return the runs this call won.

        A byte another rule already owns is left alone, so the runs a call wins
        are exactly the evidence that rule contributes and nothing more.
        """
        start, end = max(start, self.base), min(end, self.end)
        claimed, run = [], None
        for index in range(start - self.base, end - self.base):
            if self.state[index]:
                run = None
                continue
            self.state[index] = value
            if run is None:
                run = [index, index + 1]
                claimed.append(run)
            else:
                run[1] = index + 1
        won = [(begin + self.base, stop + self.base) for begin, stop in claimed]
        if detail is not None:
            for begin, _ in won:
                self.detail[begin] = detail
        return won


def paint_executable(pe, ghidra, capstone, extent):
    """Paint the extent in evidence precedence and report what each layer owns."""
    chunks = [(rva(chunk["rva"]), rva(chunk["end_rva"])) for chunk in capstone["chunks"]]
    gaps = [(rva(gap["rva"]), rva(gap["end_rva"])) for gap in capstone["coverage_gaps"]]
    check_corpus_tiling(chunks, gaps, extent.size)

    layers = {}
    layers["capstone_instruction_bytes"] = sum(
        stop - begin for start, end in chunks
        for begin, stop in extent.paint(start, end, CODE))

    # A jump table the corpus walked is data the decoded jump itself names.
    tables = [(rva(table["table_rva"]), rva(table["table_rva"]) + 4 * table["entries"],
               table["jump_rva"]) for table in capstone["switch_tables"]
              if table["entries"]]
    layers["walked_switch_table_bytes"] = sum(
        stop - begin for start, end, jump in sorted(tables)
        for begin, stop in extent.paint(start, end, TABLE,
                                        {"type": "switch_table", "jump": jump}))

    # Ghidra decoded these and the descent refused them; both readings are kept.
    ghidra_only = []
    for entry in ghidra["instruction_ranges"]:
        start, end = rva(entry["rva"]), rva(entry["end_rva"])
        if extent.contains(start):
            ghidra_only += extent.paint(start, end, CODE)
    layers["ghidra_only_instruction_bytes"] = sum(stop - begin
                                                  for begin, stop in ghidra_only)

    # Slots the corpus published as unresolved, in stride-4 runs of more than
    # one. A jump table whose dead slot 0 carries no relocation stops the walk
    # at its first slot, so the slots that do carry one are all the corpus can
    # leave behind. A lone site is not a table: it is as likely to be the imm32
    # of an instruction neither oracle decoded, and claiming it would carve a
    # data object out of a live instruction under a proof false of it.
    sites = sorted(rva(use["rva"]) for use in capstone["relocation_uses"]
                   if use["use"] == "unresolved" and extent.free(rva(use["rva"])))
    derived, run, lone = [], [], []
    for site in sites:
        if run and site == run[-1] + 4:
            run.append(site)
        else:
            (derived if len(run) > 1 else lone).append(run)
            run = [site]
    (derived if len(run) > 1 else lone).append(run)
    derived = [(group[0], group[-1] + 4) for group in derived if group]
    lone = [group[0] for group in lone if group]
    layers["derived_switch_table_bytes"] = sum(
        stop - begin for start, end in derived
        for begin, stop in extent.paint(start, end, TABLE,
                                        {"type": "derived_switch_table"}))

    layers["ghidra_data_bytes"] = sum(
        stop - begin for entry in ghidra["data"]
        if extent.contains(rva(entry["rva"]))
        for begin, stop in extent.paint(
            rva(entry["rva"]), rva(entry["rva"]) + entry["length"], DATA,
            {"type": "ghidra_data", "data_type": entry["data_type"]}))
    return layers, ghidra_only, derived, lone


def classify_gaps(capstone, extent, incoming):
    """Resolve what is left of each coverage gap, or leave it unexplained."""
    report = {"ascii_blob_bytes": 0, "alignment_padding_bytes": 0,
              "alignment_jump_bytes": 0, "unreferenced_code_bytes": 0,
              "interior_padding_runs": 0, "padding_runs": 0,
              "padding_runs_ending_on_an_alignment_boundary": 0,
              "unreferenced_code_regions": [], "ascii_blobs": []}
    for gap in capstone["coverage_gaps"]:
        check_gap_bytes(gap)
        start, end = rva(gap["rva"]), rva(gap["end_rva"])
        buffer = bytes.fromhex(gap["bytes"])
        undecodable = {start + offset for offset in gap["undecodable_offsets"]}

        def unowned():
            return [(begin + extent.base, stop + extent.base) for begin, stop, _
                    in runs(extent.state, start - extent.base, end - extent.base, {0})]

        # A printable run is a blob the compiler parked in .text, not an instruction.
        for begin, stop in unowned():
            text = buffer[begin - start:stop - start]
            if len(text) >= 4 and all(0x20 <= byte <= 0x7E for byte in text):
                value = text.decode("ascii")
                extent.paint(begin, stop, DATA, {"type": "ascii_blob", "value": value})
                report["ascii_blob_bytes"] += stop - begin
                report["ascii_blobs"].append({
                    "rva": hexa(begin), "size": stop - begin, "value": value,
                    "note": "MSVC C++ exception runtime signature"
                            if value == MSVC_EH_SIGNATURE else ""})

        padded = []
        for begin, stop in unowned():
            for run_start, run_end in padding_runs(buffer, begin - start, stop - start):
                padded.append((start + run_start, start + run_end))
        for begin, stop in padded:
            report["alignment_padding_bytes"] += sum(
                end - start for start, end in extent.paint(
                    begin, stop, PADDING, {"type": "alignment_padding"}))
            # A run that ends on an alignment boundary is padding doing the job
            # its name claims; this is the check that makes the reclassification
            # of the multi-byte NOP encodings credible.
            report["padding_runs"] += 1
            report["padding_runs_ending_on_an_alignment_boundary"] += stop % 4 == 0

        # A `jmp` over the alignment that follows it: the branch lands on the
        # first byte after the run, so the pair is padding, not reachable code.
        for begin, stop in unowned():
            for address in range(begin, stop - 1):
                if buffer[address - start] != 0xEB:
                    continue
                landing = address + 2 + buffer[address + 1 - start]
                if (address + 2, landing) in padded:
                    report["alignment_jump_bytes"] += sum(
                        end - start for start, end in extent.paint(
                            address, address + 2, PADDING,
                            {"type": "alignment_jump"}))

        # What is left decodes completely and nothing in any oracle reaches it.
        for begin, stop in unowned():
            if any(begin <= address < stop for address in incoming):
                continue
            if any(begin <= address < stop for address in undecodable):
                continue
            report["unreferenced_code_bytes"] += sum(
                end - start for start, end in extent.paint(
                    begin, stop, UNREFERENCED, {"type": "unreferenced_code"}))
            report["unreferenced_code_regions"].append({
                "rva": hexa(begin), "size": stop - begin, "gap": gap["rva"],
                "swept_instructions": gap["swept_instructions"]})

        for begin, stop in padded:
            before, after = begin - 1 - extent.base, stop - extent.base
            if (before >= 0 and extent.state[before] == UNREFERENCED
                    and after < len(extent.state)
                    and extent.state[after] == UNREFERENCED):
                report["interior_padding_runs"] += 1
    return report


def build_rows(extent, starts):
    """Cut the painted extent into maximal same-class rows.

    A row breaks at every function entry and at every address a painting rule
    recorded evidence for, so no row carries a proof that holds for only part
    of it.
    """
    ordered = sorted(set(starts) | set(extent.detail))
    rows = []
    for start, end, value in runs(extent.state, 0, len(extent.state)):
        start, end = start + extent.base, end + extent.base
        cuts = sorted({start, end}.union(
            ordered[bisect.bisect_right(ordered, start):bisect.bisect_left(ordered, end)]))
        for begin, stop in zip(cuts, cuts[1:]):
            rows.append({"rva": begin, "size": stop - begin, "class": value,
                         "entry": begin in starts,
                         "detail": extent.detail.get(begin, {})})
    return rows


def call_edges(ghidra, capstone, extent):
    edges = set()
    for function in ghidra["functions"]:
        for target in function["called"]:
            if target["space"] == "ram" and target["rva"]:
                edges.add((rva(function["rva"]), rva(target["rva"])))
    for reference in ghidra["references"]:
        if reference["type"] in CALL_REFERENCES and reference["to_rva"]:
            edges.add((rva(reference["from_rva"]), rva(reference["to_rva"])))
    for instruction in capstone["instructions"]:
        if instruction["flow"] == "call" and instruction["target_rva"]:
            edges.add((rva(instruction["rva"]), rva(instruction["target_rva"])))
    for tail in capstone["tail_calls"]:
        edges.add((rva(tail["from_rva"]), rva(tail["to_rva"])))
    return {(source, target) for source, target in edges
            if extent.contains(source) and extent.contains(target)}


def translation_unit_spans(files):
    """The address span each translation unit occupies, from its __FILE__ seeds.

    A span is evidence about every entry inside it, not only about the entries
    that carry the string, because the linker laid each object out contiguously.
    """
    span = {}
    for address, name in files.items():
        low, high = span.get(name, (address, address))
        span[name] = (min(low, address), max(high, address))
    spans = sorted((low, high, name) for name, (low, high) in span.items())
    check_translation_unit_spans(spans)
    return spans


def assign_phases(nodes, edges, files, artifacts, pinned_exports, runtime_floor,
                  ruling=None):
    """Phase every entry from the strongest evidence that reaches it.

    In this order, and every entry records the rule that placed it:

    `runtime_artifact`      Ghidra's Function ID names it, or nothing reaches it.
    `runtime_tail`          above the last entry carrying a translation unit,
                            where the statically linked runtime lives and the
                            image holds no product evidence at all.
    `slot_ruling`           the shape-class vtable ruling names it: `ruling`
                            maps its address to the phase shape_slot_ruling.json
                            gives it. It outranks a span on purpose, because
                            the shape classes' units are split by slot rather
                            than owned whole, and it seeds the caller layer so
                            a helper only ruled rows call follows them.
    `translation_unit`      inside the address span of a named translation unit.
    `enclosed_by_one_phase` between two spans one phase owns, so the unnamed
                            translation units between them are bracketed by it.
    `callers`               every caller already placed agrees on one phase, at
                            the moment this entry is placed - see the override
                            below, which can move a caller afterwards.
    `layout_adjacency`      the nearer span by address. The weakest rule here,
                            named apart so its weakness stays visible.

    Two more are then applied over the result rather than in that sequence:

    `shared_by_callers`     no translation unit of its own and reached from
                            callers several phases own: shared runtime. It
                            overrides `callers` and `layout_adjacency`, neither
                            of which is translation-unit evidence, and it does
                            not override `translation_unit`,
                            `enclosed_by_one_phase` or `slot_ruling` -- a ruled
                            row is placed by name. An entry a span names, or
                            that two spans of one phase bracket, is physically
                            inside that unit: the phase owning the unit
                            reconstructs it along with everything else in it,
                            and being called from several subsystems is ordinary
                            C++ rather than evidence of a separate owner.
    `export_pin`            the umbrella plan locks this export's owner. Applied
                            last and to the export alone, because a pin is a plan
                            decision rather than evidence about the code, and
                            propagating it would launder it into the neighbours.

    An image naming no translation unit has a runtime floor of zero, so every
    entry is placed by `runtime_tail` and the layout rules are never reached;
    that is why they may read the span list without guarding it for emptiness.
    """
    spans = translation_unit_spans(files)
    lows = [low for low, _, _ in spans]
    callers = {}
    for source, target in edges:
        callers.setdefault(target, set()).add(source)

    def enclosing(address):
        index = bisect.bisect_right(lows, address) - 1
        if index >= 0 and spans[index][0] <= address <= spans[index][1]:
            return spans[index][2]
        return None

    def neighbours(address):
        """The spans below and above an address that no span encloses."""
        index = bisect.bisect_right(lows, address) - 1
        return (spans[index] if index >= 0 else None,
                spans[index + 1] if index + 1 < len(spans) else None)

    phase, provenance = {}, {}

    def place(node, value, why):
        phase[node], provenance[node] = value, why

    for node in sorted(nodes):
        if node in artifacts:
            place(node, ARTIFACT_PHASE, "runtime_artifact")
            continue
        if node > runtime_floor:
            place(node, ARTIFACT_PHASE, "runtime_tail")
            continue
        if ruling and node in ruling:
            place(node, ruling[node], "slot_ruling")
            continue
        unit = enclosing(node)
        if unit:
            place(node, SOURCE_PHASES[unit], "translation_unit")
            continue
        below, above = neighbours(node)
        if below and above and SOURCE_PHASES[below[2]] == SOURCE_PHASES[above[2]]:
            place(node, SOURCE_PHASES[below[2]], "enclosed_by_one_phase")

    changed = True
    while changed:
        changed = False
        for node in sorted(nodes - set(phase)):
            reaching = {phase[caller] for caller in callers.get(node, ())
                        if caller in phase} - {ARTIFACT_PHASE}
            if len(reaching) == 1:
                place(node, reaching.pop(), "callers")
                changed = True

    for node in sorted(nodes - set(phase)):
        below, above = neighbours(node)
        nearer = (below if above is None else above if below is None else
                  below if node - below[1] <= above[0] - node else above)
        place(node, SOURCE_PHASES[nearer[2]], "layout_adjacency")

    # Shared runtime is what has no translation unit of its own and serves
    # several phases. Read from the settled assignment, so which entry is
    # examined first cannot change the answer.
    #
    # This runs after the caller layer, so it can move a caller an entry has
    # already inherited from. That entry keeps the phase the evidence gave it,
    # which is right - the override is a statement about the caller, not about
    # everything it calls - but its `callers` provenance then names a caller
    # that no longer carries that phase. Entries in the shipped image are in
    # that position, and some have no caller left on their own phase, so they
    # have to be audited from the layout around them instead.
    settled = dict(phase)
    for node in sorted(nodes):
        if provenance[node] in ("translation_unit", "enclosed_by_one_phase",
                                "runtime_tail", "runtime_artifact", "slot_ruling"):
            continue
        reaching = {settled[caller] for caller in callers.get(node, ())
                    if caller in settled} - {ARTIFACT_PHASE}
        if len(reaching) > 1:
            place(node, SHARED_PHASE, "shared_by_callers")

    for name, address in pinned_exports.items():
        place(address, EXPORT_PHASES[name], "export_pin")
    return phase, provenance


def source_seeds(ghidra, owners):
    """Every function referencing an NX_ASSERT __FILE__ string names its own unit.

    The reference is resolved through the census's own owner map, not through
    Ghidra's function bodies alone: those cover 925,415 of 1,056,977 executable
    bytes, and a reference landing in the rest would otherwise be dropped in
    silence. Anything that still resolves to nothing is counted, not discarded.
    """
    files = {}
    for entry in ghidra["strings"]:
        index = entry["value"].find(SOURCE_MARK)
        if index >= 0:
            files[rva(entry["rva"])] = entry["value"][index + len(SOURCE_MARK):]
    seeds, unmapped, unowned, total = {}, set(), 0, 0
    for reference in ghidra["references"]:
        if not reference["to_rva"]:
            continue
        name = files.get(rva(reference["to_rva"]))
        if name is None:
            continue
        total += 1
        owner = owners.at(rva(reference["from_rva"]))
        if owner is None:
            unowned += 1
        elif name in SOURCE_PHASES:
            seeds[owner] = name
        else:
            unmapped.add(name)
    return seeds, sorted(unmapped), {"source_references": total,
                                     "source_references_with_no_owner": unowned}


def runtime_artifacts(ghidra, exports, edges, nodes):
    """Ghidra's Function ID names the CRT; what only the CRT reaches joins it."""
    artifacts = {}
    for function in ghidra["functions"]:
        name = function["name"]
        if name not in exports and not name.startswith(("FUN_", "thunk_FUN_")):
            artifacts[rva(function["rva"])] = f"Ghidra Function ID names it {name}"
    callers = {}
    for source, target in edges:
        callers.setdefault(target, set()).add(source)
    changed = True
    while changed:
        changed = False
        for node in nodes:
            if node in artifacts:
                continue
            reaching = callers.get(node, set()) & nodes
            if reaching and all(caller in artifacts for caller in reaching):
                artifacts[node] = "every caller in either oracle is runtime code"
                changed = True
    return artifacts


def dispatch_tables(pe, extent, entries):
    """Recover vtable-shaped dispatch from the PE pointer table.

    The image was built /GR-, so it carries no RTTI and Ghidra's vtable analyzer
    finds nothing. What it does carry is runs of consecutive relocated slots that
    every one name a function entry, which is what a vtable is once its type
    descriptor is gone. A run of one is just a callback and is not claimed here.
    """
    slots = sorted((slot["rva"], slot["target_rva"]) for slot in pe["pointers"]
                   if slot["relocated"] and slot["target_rva"] is not None
                   and not extent.contains(slot["rva"])
                   and extent.contains(slot["target_rva"]))
    tables, run = [], []
    for address, target in slots:
        if run and address == run[-1][0] + 4:
            run.append((address, target))
        else:
            if len(run) > 1:
                tables.append(run)
            run = [(address, target)]
    if len(run) > 1:
        tables.append(run)
    return [table for table in tables
            if all(target in entries for _, target in table)]


def data_census(pe, ghidra, extent, tables):
    """Paint the referenced bytes of the non-executable image, one owner deep."""
    sections = [(section["name"], section["rva"],
                 section["rva"] + max(section["virtual_size"], section["raw_size"]))
                for section in pe["sections"]]
    header_end = min(start for _, start, _ in sections)
    regions = [("headers", 0, header_end)] + sections
    limit = max(end for _, _, end in regions)
    region_end = Locator([(start, end, end) for _, start, end in regions])

    candidates = []

    def add(kind, start, size, detail=None):
        if size > 0 and 0 <= start < limit and not extent.contains(start):
            candidates.append((DATA_CLASSES.index(kind), start,
                               min(size, limit - start), detail or {}))

    for directory in pe["data_directories"]:
        if directory["name"] == "basereloc":
            add("relocation_metadata", directory["rva"], directory["size"])
        elif directory["name"] == "export":
            add("export_directory", directory["rva"], directory["size"])
    for resource in pe["resources"]:
        add("resource", resource["rva"], resource["size"])
    for slot in sorted({entry["iat_rva"] for entry in pe["imports"]}):
        add("import_address_table", slot, 4)
    for entry in pe["strings"]:
        add("string", entry["rva"], entry["size"], {"encoding": entry["encoding"]})
    for entry in ghidra["data"]:
        add("ghidra_data", rva(entry["rva"]), entry["length"],
            {"data_type": entry["data_type"]})
    for table in tables:
        add("dispatch_table", table[0][0], 4 * len(table),
            {"slots": len(table),
             "targets": [hexa(target) for _, target in table[:8]]})
    for slot in pe["pointers"]:
        if slot["relocated"]:
            add("pointer_slot", slot["rva"], 4, {"target": slot["target_rva_hex"]})

    referenced, sites, outside = {}, {}, []
    for reference in ghidra["references"]:
        if reference["to_space"] != "ram" or not reference["to_rva"]:
            continue
        target = rva(reference["to_rva"])
        if extent.contains(target):
            continue
        if target >= limit or region_end.at(target) is None:
            outside.append({"rva": hexa(target), "from": reference["from_rva"],
                            "reason": "no mapped section of the image covers it"})
        else:
            referenced.setdefault(target, set()).add("ghidra:" + reference["type"])
            if extent.contains(rva(reference["from_rva"])):
                sites.setdefault(target, set()).add(rva(reference["from_rva"]))
    for slot in pe["pointers"]:
        target = slot["target_rva"]
        if slot["relocated"] and target is not None and not extent.contains(target):
            referenced.setdefault(target, set()).add("pe:relocated_pointer")
            if extent.contains(slot["rva"]):
                sites.setdefault(target, set()).add(slot["rva"])
    for entry in pe["imports"]:
        referenced.setdefault(entry["iat_rva"], set()).add("pe:import")

    # A reference is evidence that an object starts there; the next anchor and
    # the end of the region are all that bound it, and the proof says so.
    anchors = sorted({start for _, start, _, _ in candidates} | set(referenced))
    for position, address in enumerate(anchors):
        if address not in referenced:
            continue
        stop = anchors[position + 1] if position + 1 < len(anchors) else limit
        add("code_addressed_global", address,
            min(stop, region_end.at(address) or limit) - address,
            {"bounded_by": "the next anchor in the same section"})

    owner = [-1] * limit
    objects = []
    for rank, start, size, detail in sorted(candidates,
                                            key=lambda item: (item[0], item[1])):
        index = len(objects)
        painted = False
        for position in range(start, start + size):
            if owner[position] < 0:
                owner[position] = index
                painted = True
        if painted:
            objects.append({"type": DATA_CLASSES[rank], "detail": detail})

    rows, explained, bounded, structural, per_type = [], 0, 0, 0, {}
    position = 0
    while position < limit:
        index = owner[position]
        stop = position
        while stop < limit and owner[stop] == index:
            stop += 1
        if index >= 0:
            kind = objects[index]["type"]
            uses = sorted({use for address in range(position, stop)
                           for use in referenced.get(address, ())})
            # A dispatch table is significant whether or not either oracle
            # recorded the instruction that installs it.
            if uses or kind == "dispatch_table":
                per_type[kind] = per_type.get(kind, 0) + stop - position
                if not uses:
                    structural += stop - position
                elif kind == "code_addressed_global":
                    bounded += stop - position
                else:
                    explained += stop - position
                rows.append({"rva": position, "size": stop - position, "type": kind,
                             "detail": objects[index]["detail"],
                             "referenced_by": uses,
                             "sites": sorted({site for address in range(position, stop)
                                              for site in sites.get(address, ())})})
        position = stop
    unexplained = sorted(address for address in referenced if owner[address] < 0)
    report = {
        "referenced_addresses": len(referenced),
        # Bytes owned by a class that proves what they are, kept apart from the
        # bytes the catch-all merely bounds.
        "explained_referenced_bytes": explained,
        "bounded_referenced_bytes": bounded,
        # One byte per referenced address no owner class covers. That total
        # cannot rise while `code_addressed_global` exists to bound whatever
        # nothing else claimed, so read the zero as a statement about the other
        # seven classes and read `bounded_referenced_bytes` on its own terms.
        "unexplained_referenced_bytes": len(unexplained),
        "unexplained_referenced_addresses": len(unexplained),
        "unexplained_addresses": [hexa(address) for address in unexplained[:32]],
        "bytes_by_type": dict(sorted(per_type.items())),
        "dispatch_table_bytes_without_a_recorded_reference": structural,
        "references_outside_the_image": outside,
    }
    return rows, report


def oracle_independence(ghidra, capstone, extent):
    """Measure what the Capstone descent would lose if Ghidra contributed nothing.

    `seed_sources.ghidra_function: 2527` reads as heavy dependence and is not:
    the descent re-reaches nearly all of it from the PE entry point, the export
    table, the relocation table and its own recovered control flow. This drops
    every entry only Ghidra names, then walks the corpus's own recorded edges
    from what is left, and reports the entries that stay unreachable.
    """
    ghidra_sources = {"ghidra_function", "thunk_target"}
    entries = {rva(entry["rva"]): set(entry["sources"]) for entry in capstone["entries"]}
    seeds = {address for address, sources in entries.items()
             if sources - ghidra_sources}

    chunks = Locator([(rva(chunk["rva"]), rva(chunk["end_rva"]), rva(chunk["rva"]))
                      for chunk in capstone["chunks"]])
    successors = {}
    for chunk in capstone["chunks"]:
        if chunk["terminator"] == "fallthrough":
            successors.setdefault(rva(chunk["rva"]), set()).add(rva(chunk["end_rva"]))
    for instruction in capstone["instructions"]:
        if not instruction["target_rva"]:
            continue
        owner = chunks.at(rva(instruction["rva"]))
        if owner is not None:
            successors.setdefault(owner, set()).add(rva(instruction["target_rva"]))
    for table in capstone["switch_tables"]:
        owner = chunks.at(rva(table["jump_rva"]))
        if owner is not None:
            successors.setdefault(owner, set()).update(
                rva(target) for target in table["targets"])

    reached, stack = set(seeds), list(seeds)
    while stack:
        node = stack.pop()
        for target in successors.get(node, ()):
            if target not in reached:
                reached.add(target)
                stack.append(target)
    lost = sorted(address for address in entries if address not in reached)

    functions = {rva(function["rva"]) for function in ghidra["functions"]}
    return {
        "capstone_entries": len(entries),
        "entries_named_only_by_ghidra": len(entries) - len(seeds),
        "entries_unreachable_without_ghidra": len(lost),
        "unreachable_entries": [hexa(address) for address in lost[:16]],
        "ghidra_functions": len(functions),
        "ghidra_functions_that_are_capstone_entries":
            len(functions & set(entries)),
        "ghidra_instruction_bytes": ghidra["coverage"]["instruction_bytes"],
        "capstone_instruction_bytes": capstone["coverage"]["instruction_bytes"],
        "note": "the two disassemblers were run apart and agree on the extent of "
                "the code to within the difference these two totals show; "
                "dropping Ghidra costs the descent only the entries counted here.",
    }


def reconcile(pe, ghidra, capstone, inventory, ruling=None):
    """Return the inventory, ledger, coverage reports and dependency graph.

    `ruling` is shape_slot_ruling.json, read through the same resolver the
    validator uses, so the rows it places are placed identically by both.
    """
    for kind, document in (("pe", pe), ("ghidra", ghidra), ("capstone", capstone)):
        check_schema(kind, document)
    check_images(pe, capstone, inventory)
    check_function_bodies(ghidra["functions"])

    extent = Extent(pe, capstone)
    exports = {entry["name"]: entry["rva"] for entry in pe["exports"]["entries"]}
    layers, ghidra_only, derived, lone_sites = paint_executable(
        pe, ghidra, capstone, extent)

    # Every address either oracle records something as arriving at.
    incoming = set()
    for instruction in capstone["instructions"]:
        if instruction["target_rva"]:
            incoming.add(rva(instruction["target_rva"]))
    for reference in ghidra["references"]:
        if reference["to_rva"] and reference["to_space"] == "ram":
            incoming.add(rva(reference["to_rva"]))
    for slot in pe["pointers"]:
        if slot["relocated"] and slot["target_rva"] is not None:
            incoming.add(slot["target_rva"])
    incoming = {address for address in incoming if extent.contains(address)}
    gaps = classify_gaps(capstone, extent, incoming)
    check_unexplained(extent.state, extent.base)

    starts = {rva(entry["rva"]) for entry in capstone["entries"]
              if entry["kind"] == "function"}
    starts |= {rva(function["rva"]) for function in ghidra["functions"]}
    starts |= set(exports.values())
    starts.add(pe["image"]["entry_point"])
    starts |= {start for start, _ in ghidra_only}
    starts |= {int(region["rva"], 16)
               for region in gaps["unreferenced_code_regions"]}
    decoded = {rva(instruction["rva"]) for instruction in capstone["instructions"]}
    decoded |= starts
    ghidra_decoded = Locator([(rva(entry["rva"]), rva(entry["end_rva"]), True)
                              for entry in ghidra["instruction_ranges"]])

    # Only control flow contributes an executable target. A relocated pointer
    # the corpus classified as the displacement of a memory operand is the
    # program reading an address, not branching to one.
    displacements = {rva(use["rva"]) for use in capstone["relocation_uses"]
                     if use["use"] == "displacement"}
    targets, entries = set(), set(exports.values()) | {pe["image"]["entry_point"]}
    for instruction in capstone["instructions"]:
        if instruction["target_rva"] and instruction["flow"] in ("call", "jump",
                                                                 "branch"):
            targets.add(rva(instruction["target_rva"]))
            if instruction["flow"] == "call":
                entries.add(rva(instruction["target_rva"]))
    for tail in capstone["tail_calls"]:
        entries.add(rva(tail["to_rva"]))
    for table in capstone["switch_tables"]:
        targets |= {rva(target) for target in table["targets"]}
    for use in capstone["relocation_uses"]:
        if use["use"] == "unresolved":
            targets.add(rva(use["target_rva"]))
    for slot in pe["pointers"]:
        if (slot["relocated"] and slot["target_rva"] is not None
                and slot["rva"] not in displacements):
            targets.add(slot["target_rva"])
            # A slot inside a jump table names a block; any other relocated slot
            # holding a code address has had that address taken.
            if not extent.free(slot["rva"]) and (
                    not extent.contains(slot["rva"])
                    or extent.state[slot["rva"] - extent.base] != TABLE):
                entries.add(slot["target_rva"])
    for reference in ghidra["references"]:
        if reference["to_rva"] and reference["type"] in FLOW_REFERENCES:
            targets.add(rva(reference["to_rva"]))
            if reference["type"] in CALL_REFERENCES:
                entries.add(rva(reference["to_rva"]))
    targets = {target for target in targets | entries if extent.contains(target)}
    entries &= targets

    # A target landing on a jump table this reconciliation owns is the table's
    # base, which the jump names through a memory operand. The corpus could not
    # say so because the instruction carrying it is one only Ghidra decoded.
    bases = sorted(target for target in targets
                   if extent.state[target - extent.base] == TABLE)
    targets -= set(bases)

    resolved = {target for target in targets
                if target in decoded or ghidra_decoded.at(target)}
    unresolved = sorted(targets - resolved)
    check_targets(unresolved)

    # A function-like target the evidence resolves is an entry, and an entry
    # starts a row. One pass, not a loop: every input to this line is fixed
    # before it, so a second pass could only ever add the empty set - which is
    # why the counter that used to report "additions on the final iteration" is
    # gone rather than published as a zero that could not have been anything
    # else. What replaces it is the check below, against the finished rows.
    starts |= {target for target in entries & resolved
               if extent.state[target - extent.base] in CODE_CLASSES}
    independence = oracle_independence(ghidra, capstone, extent)
    discovery = {
        "recovered_targets": len(targets),
        "unresolved_targets": len(unresolved),
        "relocation_sites_too_lone_to_be_a_table": [hexa(site) for site in lone_sites],
        "function_like_targets": len(entries),
        "resolved_by_ghidra_decode_only":
            len(resolved - decoded),
        "targets_naming_a_jump_table_base": [hexa(base) for base in bases],
    }

    rows = build_rows(extent, starts)

    # Every function-like target the evidence names has to be the start of a row
    # this census owns. The two sides are derived apart - the targets from
    # decoded control flow, tail calls, relocated pointers and Ghidra's call
    # references, the starts from entry kinds, Ghidra functions, exports and the
    # runs each painting rule claimed - so agreement between them is a result.
    owned = {row["rva"] for row in rows}
    check_entries_are_owned(sorted((entries & resolved) - owned))

    bodies = Locator([(rva(body["rva"]), rva(body["end_rva"]), rva(function["rva"]))
                      for function in ghidra["functions"] for body in function["body"]])

    # Owner of a row: the Ghidra body that claims it, else the entry it follows.
    current = None
    for row in rows:
        owner = None
        if row["class"] == UNREFERENCED:
            # Nothing reaches these bytes, so no neighbour may claim them - and
            # the run may not claim a neighbour either: it is left out of
            # `current` so the next decoded chunk inherits the last real code
            # owner instead of a proof that holds only for the run.
            owner = row["rva"]
        elif row["class"] == CODE:
            owner = bodies.at(row["rva"]) or (row["rva"] if row["entry"] else current)
            current = owner or current
        row["owner"] = owner if owner is not None else current

    nodes = {row["owner"] for row in rows if row["owner"] is not None}
    owners = Locator([(row["rva"], row["rva"] + row["size"], row["owner"])
                      for row in rows if row["owner"] is not None])
    raw_edges = call_edges(ghidra, capstone, extent)

    # A dispatch table binds its installer to every method it holds. Without
    # that edge the methods look unreachable and every one of them would fall
    # to the shared-runtime default instead of joining the class that owns them.
    tables = dispatch_tables(pe, extent, starts)
    installers = {}
    for table in tables:
        base = table[0][0]
        for reference in ghidra["references"]:
            if reference["to_rva"] and rva(reference["to_rva"]) == base:
                site = rva(reference["from_rva"])
                if extent.contains(site):
                    installers.setdefault(base, site)
        for slot in pe["pointers"]:
            if slot["target_rva"] == base and extent.contains(slot["rva"]):
                installers.setdefault(base, slot["rva"])
        site = installers.get(base)
        if site is not None:
            raw_edges |= {(site, target) for _, target in table}

    edges = {(owners.at(source), owners.at(target)) for source, target in raw_edges}
    edges = {(source, target) for source, target in edges
             if source in nodes and target in nodes and source != target}

    artifacts = runtime_artifacts(ghidra, exports, edges, nodes)
    # A run nothing reaches owns itself and is a Phase 8 artifact on that
    # ground alone, so the graph has to phase it the same way its row is written.
    for region in gaps["unreferenced_code_regions"]:
        artifacts.setdefault(int(region["rva"], 16),
                             "nothing in any oracle reaches it")
    files, unmapped, seed_census = source_seeds(ghidra, owners)
    seeds = {address: SOURCE_PHASES[name] for address, name in files.items()}
    seeds.update({address: EXPORT_PHASES[name] for name, address in exports.items()
                  if name in EXPORT_PHASES})
    pinned_exports = {name: address for name, address in exports.items()
                      if name in EXPORT_PHASES}
    # Above the last entry carrying a translation unit the image holds no
    # product evidence at all, and all but one Function ID-named runtime
    # function lives there: that region is the statically linked runtime.
    runtime_floor = max(files, default=0)
    ruled = None
    if ruling is not None:
        pointers = {slot["rva"]: slot["target_rva"] for slot in pe["pointers"]
                    if slot["relocated"] and slot["target_rva"] is not None}
        resolved, _, problems = validate_inventory.resolve_shape_ruling(ruling, pointers)
        if problems:
            raise ValueError(f"the shape ruling does not resolve against the PE oracle: "
                             f"{problems[0]}")
        ruled = {address: phase for address, phase in resolved.items() if phase is not None}
    phases, provenance = assign_phases(nodes, edges, files, artifacts,
                                       pinned_exports, runtime_floor, ruled)
    for node, why in provenance.items():
        if why == "runtime_tail":
            artifacts[node] = ("no product translation unit reaches these bytes "
                               "and none is named above the last one that does, "
                               "at " + hexa(runtime_floor))

    return emit(pe, ghidra, capstone, inventory, extent, rows, edges, phases,
                artifacts, files, seeds, exports, layers, gaps,
                unmapped, discovery, derived, ghidra_only, tables,
                provenance, owners, seed_census, lone_sites, independence)


def references(capstone, ghidra, extent):
    """Locators that name the record backing any address in the extent."""
    chunks = Locator([(rva(chunk["rva"]), rva(chunk["end_rva"]),
                       f"oracle/capstone/manifest.json#/chunks/{number}")
                      for number, chunk in enumerate(capstone["chunks"])])
    gaps = Locator([(rva(gap["rva"]), rva(gap["end_rva"]),
                     f"oracle/capstone/manifest.json#/coverage_gaps/{number}")
                    for number, gap in enumerate(capstone["coverage_gaps"])])
    entries = {rva(entry["rva"]): f"oracle/capstone/manifest.json#/entries/{number}"
               for number, entry in enumerate(capstone["entries"])}
    bodies = Locator([(rva(body["rva"]), rva(body["end_rva"]),
                       f"oracle/ghidra/manifest.json#/functions/{number}")
                      for number, function in enumerate(ghidra["functions"])
                      for body in function["body"]])
    ranges = Locator([(rva(entry["rva"]), rva(entry["end_rva"]),
                       f"oracle/ghidra/manifest.json#/instruction_ranges/{number}")
                      for number, entry in enumerate(ghidra["instruction_ranges"])])
    data = Locator([(rva(entry["rva"]), rva(entry["rva"]) + entry["length"],
                     f"oracle/ghidra/manifest.json#/data/{number}")
                    for number, entry in enumerate(ghidra["data"])
                    if extent.contains(rva(entry["rva"]))])
    blocks = Locator([(rva(block["rva"]), rva(block["end_rva"]),
                       f"oracle/ghidra/manifest.json{MEMORY_BLOCK}{number}")
                      for number, block in enumerate(ghidra["memory_blocks"])])

    def capstone_ref(address):
        return (entries.get(address) or chunks.at(address) or gaps.at(address)
                or "oracle/capstone/manifest.json#/executable_intervals/0")

    def ghidra_ref(address):
        # Ghidra maps every one of these bytes; where it claims nothing more
        # specific, the memory block is the honest reference.
        return (bodies.at(address) or ranges.at(address) or data.at(address)
                or blocks.at(address))

    return capstone_ref, ghidra_ref


def emit(pe, ghidra, capstone, inventory, extent, rows, edges, phases,
         artifacts, files, seeds, exports, layers, gaps, unmapped,
         discovery, derived, ghidra_only, tables, provenance, owners, seed_census,
         lone_sites, independence):
    capstone_ref, ghidra_ref = references(capstone, ghidra, extent)
    names = {rva(function["rva"]): function["name"] for function in ghidra["functions"]}
    export_rvas = set(exports.values())
    functions, data_objects, ledger = [], [], []
    identifiers = {}

    for row in rows:
        owner, detail = row["owner"], row["detail"]
        phase = phases.get(owner, SHARED_PHASE)
        if row["class"] in DATA_ROW_CLASSES:
            continue
        identifier = f"phys_fn_{len(functions) + 1:06d}"
        identifiers[row["rva"]] = identifier
        if row["class"] == PADDING:
            functions.append(_row(
                identifier, row, "compiler_artifact", ARTIFACT_PHASE, "padding",
                None, ghidra_ref(row["rva"]), capstone_ref(row["rva"]),
                "int3 and canonical MSVC NOP encodings only"
                if detail.get("type") == "alignment_padding" else
                "a jmp whose target is the first byte after the alignment run "
                "it skips", ""))
            continue
        artifact = owner in artifacts or row["class"] == UNREFERENCED
        proof = None
        reference = ghidra_ref(row["rva"])
        if row["class"] == UNREFERENCED:
            proof = ("the capstone sweep decodes this run completely and no call, "
                     "jump, relocation or pointer in any oracle reaches it")
        elif artifact and not row["entry"] and MEMORY_BLOCK in reference:
            # The owner's proof is evidence about the owner. Where Ghidra claims
            # nothing at these bytes, the row says the proof was carried to it
            # rather than restating a relation its own reference denies.
            proof = (f"carried forward from the entry at {hexa(owner)} "
                     f"({artifacts[owner]}); Ghidra maps these bytes but claims "
                     f"nothing more specific at them")
        elif artifact:
            proof = artifacts[owner]
        record = _row(identifier, row, "compiler_artifact" if artifact else "code",
                      ARTIFACT_PHASE if artifact else phase,
                      provenance.get(owner, "padding"),
                      None if artifact else _source_of(owner, files),
                      reference, capstone_ref(row["rva"]), proof,
                      "" if row["entry"] else
                      f"continuation of the entry at {hexa(owner)}")
        name = names.get(row["rva"])
        if row["entry"] and name and not name.startswith(("FUN_", "thunk_FUN_")):
            record["label"], record["label_confidence"] = name, "semantic"
            ledger.append({
                "id": identifier, "label": name, "confidence": "semantic",
                "evidence": ghidra_ref(row["rva"]),
                "reason": "the PE export directory names this entry"
                          if row["rva"] in export_rvas else
                          "Ghidra Function ID matched a runtime signature"})
        functions.append(record)
    check_stable_ids(functions)
    check_row_references(functions)

    # An unreferenced run is owned as a compiler artifact on the strength of
    # nothing reaching it. Record the nearest runtime function Ghidra named
    # below each one so the Phase 8 claim can be audited, not just believed.
    named = sorted(address for address in artifacts
                   if artifacts[address].startswith("Ghidra Function ID"))
    for region in gaps["unreferenced_code_regions"]:
        position = bisect.bisect_left(named, int(region["rva"], 16)) - 1
        region["nearest_named_runtime_function"] = (
            {"rva": hexa(named[position]), "label": names[named[position]],
             "distance": int(region["rva"], 16) - named[position]}
            if position >= 0 else None)

    data_rows, data_report = data_census(pe, ghidra, extent, tables)
    for row in rows:
        if row["class"] not in DATA_ROW_CLASSES:
            continue
        detail = row["detail"]
        data_objects.append({
            "rva": row["rva"], "size": row["size"],
            "type": detail.get("type", "text_data"),
            "owner": identifiers.get(row["owner"]),
            "references": [f"jump at {detail['jump']}"] if detail.get("jump") else [],
            "section": ".text", "phase": phases.get(row["owner"], ARTIFACT_PHASE),
            "phase_provenance": provenance.get(row["owner"], "padding"),
            "state": "classified", "source": None,
            "structural_proof": _data_proof(detail), "notes": detail.get("value", "")})
    for row in data_rows:
        data_objects.append({
            "rva": row["rva"], "size": row["size"], "type": row["type"],
            "owner": None, "references": row["referenced_by"][:8],
            "section": _section_of(pe, row["rva"]),
            "phase": SHARED_PHASE
                     if row["type"] in PE_STRUCTURAL_CLASSES
                     or _section_of(pe, row["rva"]) == "headers"
                     else _phase_of_sites(row["sites"], owners, phases),
            "phase_provenance": "pe_structure"
                     if row["type"] in PE_STRUCTURAL_CLASSES
                     or _section_of(pe, row["rva"]) == "headers"
                     else "reading_sites",
            "state": "classified", "source": None,
            "structural_proof": _data_structural_proof(row),
            "notes": json.dumps(row["detail"], separators=(",", ":"))
                     if row["detail"] else ""})

    # One RVA order over both halves, so the stable IDs the later phases quote
    # run in the order the brief asks for rather than in emission order.
    data_objects.sort(key=lambda row: (row["rva"], row["size"]))
    for number, row in enumerate(data_objects, start=1):
        identifier = f"phys_data_{number:06d}"
        row["id"], row["rva"] = identifier, hexa(row["rva"])
        row["label"], row["label_confidence"] = identifier, "stable-id"
    data_objects = [{key: row[key] for key in DATA_ROW_KEYS} for row in data_objects]
    check_stable_ids(functions + data_objects)

    explained = sum(row["size"] for row in functions)
    explained += sum(row["size"] for row in data_objects if row["section"] == ".text")
    overlaps, duplicated = measure_ownership(functions + data_objects)
    coverage = {
        "census": dict(inventory["coverage"]["census"], status="pass"),
        "executable_bytes": extent.size,
        "explained_executable_bytes": explained,
        "unexplained_executable_bytes": extent.size - explained,
        "unresolved_executable_targets": discovery["unresolved_targets"],
        "referenced_data_bytes": data_report["explained_referenced_bytes"]
                                 + data_report["bounded_referenced_bytes"]
                                 + data_report["unexplained_referenced_bytes"],
        "unexplained_referenced_data_bytes":
            data_report["unexplained_referenced_bytes"],
        "overlaps": overlaps,
        "duplicate_ownership": duplicated,
    }
    check_census(coverage)
    check_label_ledger(functions + data_objects, ledger)

    document = dict(inventory)
    document["sections"] = [{
        "name": section["name"], "rva": hexa(section["rva"]),
        "virtual_size": section["virtual_size"],
        "raw_offset": hexa(section["raw_offset"]), "raw_size": section["raw_size"],
        "characteristics": section["characteristics_hex"],
        "executable": section["executable"]} for section in pe["sections"]]
    document["functions"] = functions
    document["data_objects"] = data_objects
    document["exports"] = [{
        "ordinal": entry["ordinal"], "name": entry["name"], "rva": hexa(entry["rva"]),
        "function_id": identifiers.get(entry["rva"])}
        for entry in pe["exports"]["entries"]]
    document["imports"] = [{"dll": entry["dll"], "name": entry["name"],
                            "ordinal": entry["ordinal"],
                            "iat_rva": hexa(entry["iat_rva"])}
                           for entry in pe["imports"]]
    document["coverage"] = coverage
    document["phases"] = [{"phase": number, "name": name,
                           "status": "closed" if number == 1 else "pending"}
                          for number, name in PHASES]
    document["gates"] = _gates(inventory, ghidra, capstone)

    return {
        "inventory": document,
        "labels": {"schema_version": SCHEMA_VERSION, "labels": ledger},
        "coverage": _code_report(extent, layers, gaps, functions, data_objects,
                                 discovery, derived, ghidra_only,
                                 unmapped, seeds, artifacts, provenance,
                                 seed_census, phases, independence),
        "data_coverage": _data_report(data_report, data_objects, ghidra, tables,
                                      overlaps),
        "dependencies": _dot(identifiers, edges, phases),
    }


# The data-object vocabulary lives in the validator, which is what has to accept this tool's
# output, and is imported here rather than written a second time. Two write-ups of one vocabulary
# drifted: this map carried a `resource` type the census never uses and was missing `switch_table`
# and `ascii_blob` entirely, so it disagreed with the validator in both directions.
import validate_inventory

_DATA_PROOFS = dict(validate_inventory.DATA_PROOF_BY_TYPE)
_DATA_PROOFS["ghidra_data"] = "Ghidra typed these bytes as data"
# `resource` is a PE structure class this tool can emit; its entry is in the validator's
# vocabulary rather than grafted on here, because a type the tool can emit has to be a type the
# validator accepts.


def _data_structural_proof(row):
    """A data object's structural proof: the literal its type determines, or the template.

    A switch table's proof names the jump that decoded it, so it is built here rather than looked up.
    """
    kind = row["type"]
    if kind == "switch_table":
        return ("a decoded jmp at %s names this table and the PE oracle relocates "
                "every slot it walks" % hexa(row["jump"]))
    return _DATA_PROOFS[kind]


def _gates(inventory, ghidra, capstone):
    """Pass only the gates this run has evidence for, and carry the rest forward.

    Two of them are evidenced transitively rather than re-verified here: a
    Ghidra manifest exists only if `normalize_ghidra.py` matched its stream
    against the toolchain pin, and it records the shim hash only if
    `generate_ghidra_types.py` re-verified every pinned public header first.
    """
    gates = dict(inventory["gates"])
    gates["pe_manifest"] = "pass"
    gates["ghidra_semantics"] = "pass"
    gates["capstone_corpus"] = "pass"
    gates["phase_1_oracle_census"] = "pass"
    if ghidra["ghidra"].get("version") and capstone["capstone"].get("version"):
        gates["toolchain_pinned"] = "pass"
    if ghidra["type_coverage"].get("header_sha256"):
        gates["public_headers_pinned"] = "pass"
    return gates


def _phase_of_sites(sites, owners, phases):
    """A data object belongs to the phase that reads it, or to shared runtime."""
    owning = {phases.get(owners.at(site)) for site in sites}
    owning.discard(None)
    owning.discard(ARTIFACT_PHASE)
    return owning.pop() if len(owning) == 1 else SHARED_PHASE


def _row(identifier, row, kind, phase, why, source, ghidra_ref, capstone_ref,
         proof, notes):
    # An artifact has no behaviour to mutate, so it never passes through the code rows' ladder
    # and never reaches `closed`: it is `classified` from the moment its classification proof is
    # recorded, which is here. Emitting it as `discovered` would produce an inventory its own
    # validator rejects.
    state = "classified" if kind == "compiler_artifact" else "discovered"
    return {"id": identifier, "rva": hexa(row["rva"]), "size": row["size"],
            "kind": kind, "label": identifier, "label_confidence": "stable-id",
            "section": ".text", "phase": phase, "phase_provenance": why,
            "state": state,
            "source": source, "ghidra_ref": ghidra_ref, "capstone_ref": capstone_ref,
            "static_proof": proof, "dynamic_proof": None, "notes": notes}


def _source_of(owner, files):
    name = files.get(owner)
    return "Physics/src/" + name.replace(chr(92), "/") if name else None


def _data_proof(detail):
    kind = detail.get("type")
    if kind == "switch_table":
        return (f"a decoded jmp at {detail['jump']} names this table and the PE "
                f"oracle relocates every slot it walks")
    if kind == "derived_switch_table":
        return ("consecutive relocated slots the corpus published as unresolved: "
                "its walk stopped at a dead slot 0 that carries no relocation")
    if kind == "ascii_blob":
        return "a printable ASCII run inside the executable extent"
    return f"Ghidra typed these bytes as {detail.get('data_type', 'data')}"


def _section_of(pe, address):
    for section in pe["sections"]:
        size = max(section["virtual_size"], section["raw_size"])
        if section["rva"] <= address < section["rva"] + size:
            return section["name"]
    return "headers"


def _tally(values):
    counts = {}
    for value in values:
        counts[value] = counts.get(value, 0) + 1
    return dict(sorted(counts.items()))


def _code_report(extent, layers, gaps, functions, data_objects,
                 discovery, derived, ghidra_only, unmapped, seeds, artifacts,
                 provenance, seed_census, phases, independence):
    per_phase, per_kind = {}, {}
    for row in functions:
        per_phase[row["phase"]] = per_phase.get(row["phase"], 0) + 1
        per_kind[row["kind"]] = per_kind.get(row["kind"], 0) + 1
    return {
        "schema_version": SCHEMA_VERSION,
        "executable_bytes": extent.size,
        "layers": dict(layers, **{name: value for name, value in gaps.items()
                                  if name.endswith("_bytes")}),
        "rows": dict(sorted(per_kind.items()),
                     data_objects_in_text=sum(1 for row in data_objects
                                              if row["section"] == ".text")),
        "target_discovery": discovery,
        "derived_switch_tables": [{"rva": hexa(start), "slots": (end - start) // 4}
                                  for start, end in derived],
        "ghidra_only_instruction_ranges": [{"rva": hexa(start), "size": end - start}
                                           for start, end in ghidra_only],
        "unreferenced_code_regions": gaps["unreferenced_code_regions"],
        "ascii_blobs": gaps["ascii_blobs"],
        "interior_padding_runs": gaps["interior_padding_runs"],
        "padding_runs": gaps["padding_runs"],
        "padding_runs_ending_on_an_alignment_boundary":
            gaps["padding_runs_ending_on_an_alignment_boundary"],
        "rows_per_phase": dict(sorted(per_phase.items())),
        "phase_seeds": dict(seed_census, seeded_entries=len(seeds),
                            runtime_artifacts=len(artifacts),
                            entries_phased_by=_tally(provenance.values())),
        # How much of each phase rests on evidence and how much on propagation.
        "entries_per_phase_by_provenance": {
            str(phase): _tally(why for node, why in provenance.items()
                               if phases[node] == phase)
            for phase in sorted({phases[node] for node in provenance})},
        "label_sources": {
            "pe_export_directory": sum(1 for row in functions
                                       if row["label_confidence"] == "semantic"
                                       and row["kind"] == "code"),
            "ghidra_function_id": sum(1 for row in functions
                                      if row["label_confidence"] == "semantic"
                                      and row["kind"] == "compiler_artifact"),
            "not_attempted": "recovering a product name from calls, arguments, "
                             "constants, switches or floating-point behaviour. "
                             "Phase 1 censuses bytes; a name inferred from "
                             "behaviour is an inference, and the ledger admits "
                             "only evidence an oracle records.",
        },
        "unmapped_source_files": unmapped,
        "oracle_independence": independence,
    }


def _data_report(report, data_objects, ghidra, tables, overlaps):
    per_type = {}
    for row in data_objects:
        per_type[row["type"]] = per_type.get(row["type"], 0) + 1
    return {
        "schema_version": SCHEMA_VERSION,
        "classes": list(DATA_CLASSES),
        "referenced_addresses": report["referenced_addresses"],
        "explained_referenced_bytes": report["explained_referenced_bytes"],
        "bounded_referenced_bytes": report["bounded_referenced_bytes"],
        "unexplained_referenced_bytes": report["unexplained_referenced_bytes"],
        "unexplained_referenced_addresses": report["unexplained_referenced_addresses"],
        "unexplained_addresses": report["unexplained_addresses"],
        "overlaps": overlaps,
        "objects_by_type": dict(sorted(per_type.items())),
        "bytes_by_type": report["bytes_by_type"],
        "dispatch_table_bytes_without_a_recorded_reference":
            report["dispatch_table_bytes_without_a_recorded_reference"],
        "references_outside_the_image": report["references_outside_the_image"],
        "vtables": ghidra["vtables"],
        "rtti": ghidra["rtti"],
        "dispatch_structures": {
            "vtables_recovered": len(ghidra["vtables"]),
            "rtti_records_recovered": len(ghidra["rtti"]),
            "pointer_tables_recovered": len(tables),
            "pointer_table_slots": sum(len(table) for table in tables),
            "pointer_tables": [{"rva": hexa(table[0][0]), "slots": len(table)}
                               for table in tables],
            "note": "the image was built /GR-: it carries no RTTI record and "
                    "Ghidra's vtable analyzer recovers nothing, which is why "
                    "vtables and rtti are both empty. The dispatch a C++ image "
                    "still needs survives as runs of consecutive relocated "
                    "slots that every one name a function entry, and those are "
                    "recovered here from the PE pointer table instead.",
        },
    }


def _dot(identifiers, edges, phases):
    lines = ["digraph novodex_physics {", "  rankdir=LR;",
             "  node [shape=box,fontsize=8];"]
    for address in sorted(identifiers):
        if address in phases:
            lines.append(f'  "{identifiers[address]}" [rva="{hexa(address)}",'
                         f'phase={phases[address]}];')
    for source, target in sorted(edges):
        if source in identifiers and target in identifiers:
            lines.append(f'  "{identifiers[source]}" -> "{identifiers[target]}";')
    lines.append("}")
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pe", required=True, help="PE oracle manifest")
    parser.add_argument("--ghidra", required=True, help="Ghidra oracle manifest")
    parser.add_argument("--capstone", required=True, help="Capstone corpus manifest")
    parser.add_argument("--inventory", required=True, help="census inventory to rewrite")
    parser.add_argument("--labels", required=True, help="label ledger to rewrite")
    parser.add_argument("--ruling", help="shape-class vtable ruling; defaults to the "
                                         "shape_slot_ruling.json beside the inventory, if any")
    args = parser.parse_args()

    inventory_path = Path(args.inventory)
    ruling_path = (Path(args.ruling) if args.ruling else
                   inventory_path.parent / validate_inventory.SHAPE_RULING_NAME)
    try:
        inventory = json.loads(inventory_path.read_text(encoding="utf-8"))
        ruling = (json.loads(ruling_path.read_text(encoding="utf-8"))
                  if args.ruling or ruling_path.is_file() else None)
        result = reconcile(
            json.loads(Path(args.pe).read_text(encoding="utf-8")),
            json.loads(Path(args.ghidra).read_text(encoding="utf-8")),
            json.loads(Path(args.capstone).read_text(encoding="utf-8")),
            inventory, ruling)
        census = result["inventory"]["coverage"]["census"]
        reports = inventory_path.parent
        (reports / census["code_report"]).parent.mkdir(parents=True, exist_ok=True)
        for path, payload in ((inventory_path, result["inventory"]),
                              (Path(args.labels), result["labels"]),
                              (reports / census["code_report"], result["coverage"]),
                              (reports / census["data_report"], result["data_coverage"])):
            path.write_text(json.dumps(payload, indent=2) + "\n",
                            encoding="utf-8", newline="\n")
        graph = (reports / census["code_report"]).parent / "dependencies.dot"
        graph.write_text(result["dependencies"], encoding="utf-8", newline="\n")
    except (OSError, ValueError) as error:
        parser.exit(2, f"error: {error}\n")

    coverage = result["inventory"]["coverage"]
    print(f"census={coverage['census']['status']}")
    print(f"functions={len(result['inventory']['functions'])}")
    print(f"data_objects={len(result['inventory']['data_objects'])}")
    for name in ("executable_bytes", "explained_executable_bytes",
                 "unexplained_executable_bytes", "unresolved_executable_targets",
                 "referenced_data_bytes",
                 "unexplained_referenced_data_bytes", "overlaps",
                 "duplicate_ownership"):
        print(f"{name}={coverage[name]}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
