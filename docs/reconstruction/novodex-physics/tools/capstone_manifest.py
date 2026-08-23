#!/usr/bin/env python3
"""Disassemble the executable extent of a PE32 image into an independent corpus.

Recursive descent starts from every entry the PE and Ghidra oracles can justify
and runs to a fixed point. Only what the descent never reaches is swept linearly,
and a sweep cannot prove that a run of bytes is alignment padding rather than an
artifact some other tool owns, so its verdicts are recorded as proposals. Every
byte of the executable extent leaves the run as an instruction byte, an
undecodable byte, or a not-yet-classified coverage gap; none is ever skipped.
"""

import argparse
import hashlib
import json
from pathlib import Path

import capstone
from capstone import x86

import verify_toolchain


SCHEMA_VERSION = 1

# The pin sits beside the tools, so no flag is needed to find it.
PIN_FILENAME = "analysis_toolchain.json"

# Capstone disassembles eagerly, so the descent asks for a bounded window and
# resumes from wherever it stopped instead of handing it a whole section.
WINDOW_INSTRUCTIONS = 32
WINDOW_BYTES = WINDOW_INSTRUCTIONS * 15

# Flow classes whose instruction may name a target, and the seed source each
# kind of direct edge contributes.
FLOW_TARGETS = frozenset({"call", "jump", "branch"})
EDGE_SOURCES = {"call": "direct_call", "jump": "direct_jump", "branch": "direct_branch"}

# Flow classes after which execution does not continue at the next address.
TERMINATING_FLOWS = frozenset({"return", "jump", "halt"})

HALTING_MNEMONICS = frozenset({"hlt", "ud2"})

# What a linear sweep may find and still call the run padding.
PADDING_MNEMONICS = frozenset({"int3", "nop"})

# Stack effects that are fixed by the opcode rather than by an operand.
FIXED_STACK_DELTAS = {"pushfd": -4, "popfd": 4, "pushal": -32, "popal": 32}

# What each use of a relocated code address inside an executable section seeds.
# Only an immediate seeds anything: see METHOD["relocation_use"] for why neither
# of the other two may, however much code an unresolved slot appears to name.
RELOCATION_SEED_SOURCES = {"immediate": "relocation_target"}

# Every seed source, in the order the census reports them.
SEED_SOURCES = ("entry_point", "export", "ghidra_function", "thunk_target",
                "relocation_target", "pointer_table", "switch_target",
                "direct_call", "direct_jump", "direct_branch", "tail_call")

# A chunk is a function when something names it from outside local control flow.
# The precedence matters: a jump table's slots are relocated pointers too, so
# switch evidence has to outrank the pointer that carries it.
STRONG_FUNCTION_SOURCES = frozenset({"entry_point", "export", "ghidra_function",
                                     "thunk_target", "direct_call", "tail_call"})
WEAK_FUNCTION_SOURCES = frozenset({"relocation_target", "pointer_table"})

METHOD = {
    "seeding": (
        "entries are seeded from the PE entry point, every export, every Ghidra "
        "function entry and Ghidra thunk target, every relocated pointer that names "
        "an executable section from a slot rather than from an instruction operand, "
        "every relocated address a decoded instruction carries as an immediate, and "
        "every direct call, jump, branch and jump-table target the descent recovers. "
        "A pointer record no relocation fixes up is never seeded: the image is "
        "rebasable and its relocation table is complete, so an absolute address "
        "without a fixup cannot be one"
    ),
    "relocation_use": (
        "a relocated absolute address inside an executable section is read three "
        "ways. An immediate of a decoded instruction is the program using the "
        "address as a value, so it names code and is seeded. The displacement of a "
        "memory operand is the program reading that address as data, so it names a "
        "jump table, an index table or another artifact parked in the section and is "
        "never decoded. One no decoded instruction covers is unresolved and is not "
        "seeded either: it looks like a slot holding a code pointer, but a site is "
        "only unresolved until the instruction covering it happens to be decoded, so "
        "seeding one decodes jump-table bases that a later round then proves to be "
        "displacements. Such a slot is reached instead through the jump table that "
        "claims it, and a table this tool cannot walk is published with no entries"
    ),
    "descent": (
        "each entry is decoded forward one instruction at a time until a return, "
        "an unconditional jump, hlt, ud2, an already decoded address, a byte "
        "capstone refuses, or the end of the executable extent. A call is assumed "
        "to return. Newly found targets are decoded on the next iteration and the "
        "loop repeats until an iteration finds no new entry. That the loop stops is "
        "no evidence it stopped in the right place, so no manifest is written until "
        "a fresh pass re-derives every seed from the finished evidence alone - the "
        "oracles, the relocation classification, the recorded control flow and the "
        "walked jump tables - and finds each one already recorded as an entry"
    ),
    "switch_table_candidate": (
        "an unconditional jmp whose only operand is a memory operand with scale 4 "
        "and a non-zero displacement. The displacement is read as a virtual "
        "address and consecutive four-byte slots from the RVA it names are table "
        "entries for as long as the PE oracle records a relocated pointer there "
        "whose target lies inside the executable extent; the walk stops at the "
        "first slot that does not"
    ),
    "tail_call": (
        "an unconditional direct jmp whose target is independently a function "
        "entry - the entry point, an export, a Ghidra function, a Ghidra thunk "
        "target, a relocated code pointer or a direct call target. No "
        "interprocedural analysis is carried past that single test. This runs after "
        "the descent has finished, so tail_call labels entries rather than "
        "contributing them: every entry it names is already an entry through the "
        "unconditional jump that reaches it, and its seed_sources column counts "
        "entries a tail call names, not entries a tail call found"
    ),
    "stack_delta": (
        "the change the instruction alone makes to esp, counting nothing a called "
        "function does. It is null when the tool does not determine it statically, "
        "which is every other instruction that writes esp"
    ),
    "chunk_kind": (
        "an entry is a function when a strong source names it, then a block when "
        "a jump table names it, then a function when a relocated pointer names it, "
        "and otherwise an intra-function block proven by a decoded control-flow edge"
    ),
    "sweep": (
        "every byte the descent did not reach is a coverage gap and stays "
        "unclassified. The gap is disassembled linearly, resuming one byte on from "
        "anything capstone refuses, and the result is recorded as a proposal only: "
        "alignment when the whole gap decodes and every instruction is int3 or nop, "
        "code when the whole gap decodes and something else is in it, data otherwise. "
        "The sweep knows nothing of what the descent proved, so a gap holding a jump "
        "table listed in switch_tables is usually proposed code or data even though "
        "the table owns it; the proposal is the sweep's own verdict and is left that "
        "way, and switch_tables is what settles those bytes"
    ),
    "coverage": (
        "every byte of the executable extent is an instruction byte, an undecodable "
        "byte, or an unclassified byte inside a coverage gap, and the three totals "
        "are checked against the extent before the manifest is written. An "
        "undecodable byte is one the descent refused; a byte the sweep refused is "
        "counted by sweep_undecodable_bytes and is already inside the unclassified "
        "total, because a gap the sweep cannot read is still only unclassified"
    ),
}


def _hex(value) -> str:
    return f"0x{value:08x}"


def load_pin(path=None) -> dict:
    """Read the recorded toolchain pin this evidence must have been produced under."""
    path = Path(path) if path else Path(__file__).resolve().parent.parent / PIN_FILENAME
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except OSError as error:
        raise ValueError(f"cannot read the toolchain pin: {error}") from error


def check_against_pin(pin):
    """Reject a Capstone install, or a pin, the recorded contract does not certify."""
    if pin.get("schema_version") != verify_toolchain.SCHEMA_VERSION:
        raise ValueError(
            f"toolchain pin schema version is {pin.get('schema_version')!r} but this "
            f"tool reads {verify_toolchain.SCHEMA_VERSION}")
    recorded = pin.get("capstone") or {}
    package_root = Path(capstone.__file__).resolve().parent
    measured = (("version", capstone.__version__),
                ("package_sha256", verify_toolchain.tree_sha256(package_root)))
    for field, actual in measured:
        if recorded.get(field) != actual:
            raise ValueError(
                f"capstone.{field} is {actual!r} but the pin records {recorded.get(field)!r}")
    digest = verify_toolchain.canonical_options_sha256(pin.get("analysis_options") or {})
    if digest != pin.get("analysis_options_sha256"):
        raise ValueError(
            f"analysis-options hash is {digest} but the pin records "
            f"{pin.get('analysis_options_sha256')!r}")


def check_coverage(executable_bytes, instruction_bytes, undecodable_bytes, unclassified_bytes):
    """Refuse a run whose three byte states do not add up to the executable extent."""
    total = instruction_bytes + undecodable_bytes + unclassified_bytes
    if total != executable_bytes:
        raise ValueError(
            f"coverage accounts for {total} of {executable_bytes} executable bytes")


class _Text:
    """The executable extent of the image, addressable by RVA."""

    def __init__(self, data, intervals):
        self.data = data
        self.spans = []
        self.size = 0
        for interval in intervals:
            rva, size, offset = interval["rva"], interval["size"], interval["raw_offset"]
            if offset < 0 or offset + size > len(data):
                raise ValueError(
                    f"executable interval {_hex(rva)} is not backed by the file")
            self.spans.append({"rva": rva, "end_rva": rva + size,
                               "raw_offset": offset, "index": self.size})
            self.size += size

    def span_of(self, rva):
        for span in self.spans:
            if span["rva"] <= rva < span["end_rva"]:
                return span
        return None

    def contains(self, rva) -> bool:
        return self.span_of(rva) is not None

    def index(self, rva) -> int:
        span = self.span_of(rva)
        return span["index"] + rva - span["rva"]

    def view(self, rva, end):
        """Return the bytes from rva up to end, clipped to the containing span."""
        span = self.span_of(rva)
        stop = min(end, span["end_rva"])
        start = span["raw_offset"] + rva - span["rva"]
        return memoryview(self.data)[start:start + stop - rva]

    def byte(self, rva) -> int:
        span = self.span_of(rva)
        return self.data[span["raw_offset"] + rva - span["rva"]]


def _stack_delta(instruction, mnemonic):
    """Measure what the instruction alone does to esp, or None when it is not static."""
    operands = instruction.operands
    if mnemonic == "push":
        return -operands[0].size
    if mnemonic == "pop":
        return operands[0].size
    if mnemonic in FIXED_STACK_DELTAS:
        return FIXED_STACK_DELTAS[mnemonic]
    if mnemonic in ("ret", "retf"):
        width = 4 if mnemonic == "ret" else 8
        if not operands:
            return width
        return width + operands[0].imm if operands[0].type == x86.X86_OP_IMM else None
    if mnemonic == "call":
        return -4
    if (mnemonic in ("add", "sub") and len(operands) == 2
            and operands[0].type == x86.X86_OP_REG and operands[0].reg == x86.X86_REG_ESP
            and operands[1].type == x86.X86_OP_IMM):
        return operands[1].imm if mnemonic == "add" else -operands[1].imm
    # `enter` builds a frame and moves esp, but Capstone 5.0.6 does not report it
    # as writing esp, so the catch-all below would call it zero.
    if mnemonic == "enter":
        return None
    return None if x86.X86_REG_ESP in instruction.regs_access()[1] else 0


def _flow(instruction, mnemonic):
    groups = set(instruction.groups)
    if x86.X86_GRP_RET in groups or x86.X86_GRP_IRET in groups:
        return "return"
    if x86.X86_GRP_CALL in groups:
        return "call"
    if x86.X86_GRP_JUMP in groups or x86.X86_GRP_BRANCH_RELATIVE in groups:
        return "jump" if mnemonic == "jmp" else "branch"
    if x86.X86_GRP_INT in groups:
        return "interrupt"
    if mnemonic in HALTING_MNEMONICS:
        return "halt"
    return "sequential"


class _Corpus:
    """The recursive-descent state: what is decoded, what is entered, what is refused."""

    def __init__(self, text, image_base, size_of_image, pointer_targets, iat):
        self.md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        self.md.detail = True
        # Intel syntax is Capstone's default, but the manifest publishes it as
        # part of the configuration the corpus was produced under, so it is set
        # rather than assumed.
        self.md.syntax = capstone.CS_OPT_SYNTAX_INTEL
        self.text = text
        self.image_base = image_base
        self.size_of_image = size_of_image
        self.pointer_targets = pointer_targets
        self.iat = iat
        self.instructions = {}
        self.undecodable = {}
        self.entries = {}
        self.switch_tables = []
        self.import_thunks = []
        self.pending = []
        # Where every four-byte absolute field of a decoded instruction sits, so a
        # relocation site can be told apart by the use the instruction makes of it.
        self.absolute = {}

    def note(self, source, rva) -> bool:
        """Record a source for an entry, reporting whether the entry is new."""
        sources = self.entries.get(rva)
        if sources is None:
            self.entries[rva] = {source}
            return True
        sources.add(source)
        return False

    def _record(self, instruction, rva):
        mnemonic = " ".join(instruction.mnemonic.split()).lower()
        flow = _flow(instruction, mnemonic)
        operand = instruction.operands[0] if instruction.operands else None
        direct = (flow in FLOW_TARGETS and operand is not None
                  and operand.type == x86.X86_OP_IMM)
        target = operand.imm - self.image_base if direct else None
        if target is not None and not 0 <= target < self.size_of_image:
            target = None
        encoding = instruction.encoding
        if encoding.imm_size == 4:
            self.absolute[rva + encoding.imm_offset] = "immediate"
        if encoding.disp_size == 4:
            self.absolute[rva + encoding.disp_offset] = "displacement"
        return {
            "rva": _hex(rva),
            "size": instruction.size,
            "bytes": instruction.bytes.hex(),
            "mnemonic": mnemonic,
            "operands": " ".join(instruction.op_str.split()).lower(),
            "flow": flow,
            "indirect": flow in FLOW_TARGETS and not direct,
            "target_rva": None if target is None else _hex(target),
            "stack_delta": _stack_delta(instruction, mnemonic),
        }

    def _edges(self, instruction, record, rva):
        """Follow whatever control flow the instruction names."""
        if record["flow"] not in FLOW_TARGETS:
            return
        if record["target_rva"] is not None:
            self.pending.append((EDGE_SOURCES[record["flow"]], int(record["target_rva"], 16)))
        elif record["flow"] == "jump":
            self._indirect_jump(instruction, rva)

    def _indirect_jump(self, instruction, rva):
        operand = instruction.operands[0]
        if operand.type != x86.X86_OP_MEM:
            return
        slot = (operand.mem.disp & 0xFFFFFFFF) - self.image_base
        # A displacement that names no part of the image is an offset into a
        # register, not the address of anything this tool can read.
        if not 0 <= slot < self.size_of_image:
            return
        if operand.mem.base == 0 and operand.mem.index == 0 and slot in self.iat:
            self.import_thunks.append(dict(rva=_hex(rva), iat_rva=_hex(slot), **self.iat[slot]))
            return
        if operand.mem.scale != 4 or operand.mem.disp == 0:
            return
        targets = []
        while True:
            target = self.pointer_targets.get(slot + 4 * len(targets))
            if target is None or not self.text.contains(target):
                break
            targets.append(target)
        self.switch_tables.append({
            "jump_rva": _hex(rva), "table_rva": _hex(slot), "entries": len(targets),
            "targets": [_hex(target) for target in targets]})
        for target in targets:
            self.pending.append(("switch_target", target))

    def explore(self, start):
        """Decode forward from an entry until control flow or the bytes run out."""
        span_end = self.text.span_of(start)["end_rva"]
        cursor = start
        while cursor < span_end:
            if cursor in self.instructions or cursor in self.undecodable:
                return
            window = self.text.view(cursor, min(cursor + WINDOW_BYTES, span_end))
            progress = False
            for instruction in self.md.disasm(
                    window, self.image_base + cursor, WINDOW_INSTRUCTIONS):
                rva = instruction.address - self.image_base
                if rva in self.instructions or rva in self.undecodable:
                    return
                record = self._record(instruction, rva)
                self.instructions[rva] = record
                self._edges(instruction, record, rva)
                cursor = rva + instruction.size
                progress = True
                if record["flow"] in TERMINATING_FLOWS:
                    return
            if not progress:
                self.undecodable[cursor] = {
                    "rva": _hex(cursor), "byte": f"0x{self.text.byte(cursor):02x}",
                    "chunk_rva": _hex(start)}
                return


def _discover(corpus, seeds, iterations, number):
    """Run the descent until an iteration finds no new entry, recording each one."""
    frontier = seeds
    while True:
        corpus.pending = []
        fresh = sorted({rva for source, rva in frontier if corpus.note(source, rva)})
        for rva in fresh:
            if corpus.text.contains(rva):
                corpus.explore(rva)
        iterations.append({"round": number, "iteration": len(iterations) + 1,
                           "seeds": len(frontier), "new_entries": len(fresh)})
        if not fresh:
            return
        frontier = corpus.pending


def _oracle_seeds(pe, ghidra, executable):
    """List the entries the two oracles justify before anything has been decoded."""
    seeds = [("entry_point", pe["image"]["entry_point"])]
    seeds += [("export", entry["rva"]) for entry in pe["exports"]["entries"]
              if entry["forwarder"] is None]
    for function in ghidra["functions"]:
        seeds.append(("ghidra_function", int(function["rva"], 16)))
        target = function["thunk_target"]
        if target is not None and target["rva"] is not None:
            seeds.append(("thunk_target", int(target["rva"], 16)))
    seeds += [("pointer_table", entry["target_rva"]) for entry in pe["pointers"]
              if entry["relocated"] and entry["target_section"] in executable
              and entry["section"] not in executable]
    return seeds


def _relocation_uses(pe, corpus, executable):
    """Say how the program uses each relocated code address embedded in code."""
    return [{"rva": _hex(entry["rva"]), "target_rva": _hex(entry["target_rva"]),
             "use": corpus.absolute.get(entry["rva"], "unresolved")}
            for entry in pe["pointers"]
            if entry["relocated"] and entry["target_section"] in executable
            and entry["section"] in executable]


def _run(corpus, pe, ghidra, executable):
    """Alternate descent and relocation classification until neither finds anything.

    Which relocated addresses name code cannot be settled before decoding, and
    decoding needs those addresses as seeds, so the two run in rounds until a
    round adds no seed the entries do not already carry.
    """
    iterations = []
    rounds = []
    seeds = _oracle_seeds(pe, ghidra, executable)
    while True:
        number = len(rounds) + 1
        _discover(corpus, seeds, iterations, number)
        uses = _relocation_uses(pe, corpus, executable)
        seeds = [(RELOCATION_SEED_SOURCES[use["use"]], int(use["target_rva"], 16))
                 for use in uses if use["use"] in RELOCATION_SEED_SOURCES]
        seeds = [(source, rva) for source, rva in seeds
                 if source not in corpus.entries.get(rva, ())]
        rounds.append({"round": number, "relocation_seeds": len(seeds)})
        if not seeds:
            return iterations, rounds, uses


def _fresh_seeds(corpus, pe, ghidra, executable):
    """Re-derive every seed claim from the finished evidence alone.

    Nothing here consults how the descent ran: the oracles, the relocation
    classification, the recorded control flow and the walked jump tables are read
    back exactly as an outside checker would read them.
    """
    seeds = _oracle_seeds(pe, ghidra, executable)
    seeds += [(RELOCATION_SEED_SOURCES[use["use"]], int(use["target_rva"], 16))
              for use in _relocation_uses(pe, corpus, executable)
              if use["use"] in RELOCATION_SEED_SOURCES]
    seeds += [(EDGE_SOURCES[record["flow"]], int(record["target_rva"], 16))
              for record in corpus.instructions.values()
              if record["flow"] in FLOW_TARGETS and record["target_rva"] is not None]
    seeds += [("switch_target", int(target, 16)) for table in corpus.switch_tables
              for target in table["targets"]]
    return seeds


def check_fixed_point(entries, seeds, published, decoded):
    """Refuse a descent that a fresh pass over its own evidence would still add to.

    Seed closure alone is not enough: an entry the loop recorded but never
    decoded would satisfy it while leaving no instruction behind, so every entry
    the manifest publishes has to start an instruction the manifest publishes
    too. A seed the executable extent does not contain is not published and is
    not expected to decode; it is reported by seeds_outside_executable instead.
    """
    missing = sorted((rva, source) for source, rva in seeds
                     if source not in entries.get(rva, ()))
    if missing:
        rva, source = missing[0]
        raise ValueError(
            f"the descent is not at a fixed point: {len(missing)} seeds are unrecorded, "
            f"the first being {source} at {_hex(rva)}")
    undecoded = sorted(set(published) - set(decoded))
    if undecoded:
        raise ValueError(
            f"the descent recorded {len(undecoded)} entries it never decoded, "
            f"the first being {_hex(undecoded[0])}")


def _tail_calls(corpus):
    """Label every unconditional direct jump that lands on a known function entry."""
    tail_calls = []
    for rva in sorted(corpus.instructions):
        record = corpus.instructions[rva]
        if record["flow"] != "jump" or record["target_rva"] is None:
            continue
        target = int(record["target_rva"], 16)
        sources = corpus.entries.get(target, set())
        if sources & (STRONG_FUNCTION_SOURCES | WEAK_FUNCTION_SOURCES):
            tail_calls.append({"from_rva": record["rva"], "to_rva": record["target_rva"]})
    for tail_call in tail_calls:
        corpus.entries[int(tail_call["to_rva"], 16)].add("tail_call")
    return tail_calls


def _kind(sources):
    if sources & STRONG_FUNCTION_SOURCES:
        return "function"
    if "switch_target" in sources:
        return "block"
    return "function" if sources & WEAK_FUNCTION_SOURCES else "block"


def _chunks(corpus):
    """Cut the decoded instructions into chunks at every entry and every flow break."""
    order = sorted(corpus.instructions)
    starts = set()
    reach = None
    previous = None
    for rva in order:
        if reach != rva or previous in TERMINATING_FLOWS or rva in corpus.entries:
            starts.add(rva)
        reach = rva + corpus.instructions[rva]["size"]
        previous = corpus.instructions[rva]["flow"]

    chunks = []
    current = None
    for rva in order:
        record = corpus.instructions[rva]
        if rva in starts:
            current = {"rva": _hex(rva), "end_rva": None, "size": 0,
                       "instructions": 0, "terminator": None, "_start": rva}
            chunks.append(current)
        current["instructions"] += 1
        current["_end"] = rva + record["size"]
        current["_flow"] = record["flow"]

    for chunk in chunks:
        end = chunk.pop("_end")
        flow = chunk.pop("_flow")
        chunk["end_rva"] = _hex(end)
        chunk["size"] = end - chunk.pop("_start")
        if flow in TERMINATING_FLOWS:
            chunk["terminator"] = flow
        elif not corpus.text.contains(end):
            chunk["terminator"] = "interval_end"
        elif end in corpus.undecodable:
            chunk["terminator"] = "undecodable"
        elif end in corpus.instructions:
            chunk["terminator"] = "fallthrough"
        else:
            chunk["terminator"] = "gap"
    return chunks


def _unresolved_indirect_jumps(corpus):
    """Count indirect jumps no jump table and no import thunk accounts for."""
    resolved = ({table["jump_rva"] for table in corpus.switch_tables}
                | {thunk["rva"] for thunk in corpus.import_thunks})
    return sum(1 for record in corpus.instructions.values()
               if record["flow"] == "jump" and record["indirect"]
               and record["rva"] not in resolved)


def _overlaps(corpus):
    """Report every instruction that starts inside the one before it."""
    overlaps = []
    reach = 0
    previous = None
    for rva in sorted(corpus.instructions):
        if rva < reach:
            overlaps.append({"rva": _hex(rva), "previous_rva": _hex(previous)})
        reach = max(reach, rva + corpus.instructions[rva]["size"])
        previous = rva
    return overlaps


def _sweep(corpus, rva, end):
    """Disassemble one gap linearly and propose, but do not conclude, what it holds."""
    undecodable_offsets = []
    swept = 0
    padding = True
    cursor = rva
    while cursor < end:
        progress = False
        for instruction in corpus.md.disasm(
                corpus.text.view(cursor, end), corpus.image_base + cursor):
            cursor = instruction.address - corpus.image_base + instruction.size
            swept += 1
            progress = True
            if instruction.mnemonic not in PADDING_MNEMONICS:
                padding = False
        if not progress:
            undecodable_offsets.append(cursor - rva)
            cursor += 1
    if undecodable_offsets:
        proposal = "data"
    else:
        proposal = "alignment" if padding else "code"
    return {"rva": _hex(rva), "end_rva": _hex(end), "size": end - rva,
            "bytes": bytes(corpus.text.view(rva, end)).hex(), "proposal": proposal,
            "swept_instructions": swept, "undecodable_offsets": undecodable_offsets}


def _coverage_gaps(corpus, state):
    """Find every maximal run of bytes the descent left unclassified."""
    gaps = []
    for span in corpus.text.spans:
        start = None
        for offset in range(span["index"], span["index"] + span["end_rva"] - span["rva"]):
            if state[offset] == 0:
                start = offset if start is None else start
                continue
            if start is not None:
                gaps.append(_sweep(corpus, span["rva"] + start - span["index"],
                                   span["rva"] + offset - span["index"]))
                start = None
        if start is not None:
            gaps.append(_sweep(corpus, span["rva"] + start - span["index"],
                               span["end_rva"]))
    return gaps


def _byte_states(corpus):
    """Mark every executable byte as instruction, undecodable or unclassified."""
    state = bytearray(corpus.text.size)
    for rva, record in corpus.instructions.items():
        index = corpus.text.index(rva)
        state[index:index + record["size"]] = b"\x01" * record["size"]
    for rva in corpus.undecodable:
        state[corpus.text.index(rva)] = 2
    return state


def _check_inputs(data, pe, ghidra, pin):
    for oracle, name in ((pe, "PE"), (ghidra, "Ghidra")):
        if oracle.get("schema_version") != SCHEMA_VERSION:
            raise ValueError(
                f"the {name} oracle records schema version "
                f"{oracle.get('schema_version')!r} but this tool reads {SCHEMA_VERSION}")
    digest = hashlib.sha256(data).hexdigest()
    for recorded, name in ((pe["image"]["sha256"], "PE"),
                           (ghidra["ghidra"]["metadata"]["Executable SHA256"], "Ghidra")):
        if recorded.lower() != digest:
            raise ValueError(
                f"the {name} oracle describes image {recorded.lower()} but the binary "
                f"is {digest}")
    recorded = pin["ghidra"]["version"]
    if ghidra["ghidra"]["version"] != recorded:
        raise ValueError(
            f"ghidra.version is {ghidra['ghidra']['version']!r} but the pin records "
            f"{recorded!r}")
    if not pe["executable_intervals"]:
        raise ValueError("the PE oracle records no executable interval")


def build_manifest(data, pe, ghidra, pin=None) -> dict:
    """Disassemble the executable extent and account for every byte of it."""
    pin = load_pin() if pin is None else pin
    check_against_pin(pin)
    _check_inputs(data, pe, ghidra, pin)

    text = _Text(data, pe["executable_intervals"])
    executable = {section["name"] for section in pe["sections"] if section["executable"]}
    pointer_targets = {entry["rva"]: entry["target_rva"] for entry in pe["pointers"]
                       if entry["relocated"] and entry["target_section"] in executable}
    iat = {entry["iat_rva"]: {"dll": entry["dll"], "name": entry["name"],
                              "ordinal": entry["ordinal"]} for entry in pe["imports"]}
    corpus = _Corpus(text, pe["image"]["image_base"], pe["image"]["size_of_image"],
                     pointer_targets, iat)

    iterations, rounds, relocation_uses = _run(corpus, pe, ghidra, executable)
    rechecked = _fresh_seeds(corpus, pe, ghidra, executable)
    check_fixed_point(corpus.entries, rechecked,
                      [rva for rva in corpus.entries if text.contains(rva)],
                      corpus.instructions)
    tail_calls = _tail_calls(corpus)
    chunks = _chunks(corpus)
    state = _byte_states(corpus)
    gaps = _coverage_gaps(corpus, state)

    overlaps = _overlaps(corpus)
    entries = [{"rva": _hex(rva), "sources": sorted(corpus.entries[rva]),
                "kind": _kind(corpus.entries[rva])}
               for rva in sorted(corpus.entries) if text.contains(rva)]
    outside = [{"rva": _hex(rva), "sources": sorted(corpus.entries[rva])}
               for rva in sorted(corpus.entries) if not text.contains(rva)]
    proposed = {name: sum(gap["size"] for gap in gaps if gap["proposal"] == name)
                for name in ("alignment", "code", "data")}
    swept_undecodable = sum(len(gap["undecodable_offsets"]) for gap in gaps)
    check_coverage(text.size, state.count(1), state.count(2), state.count(0))

    return {
        "schema_version": SCHEMA_VERSION,
        "capstone": {"version": capstone.__version__, "arch": "x86", "mode": "32",
                     "syntax": "intel", "detail": True},
        "image": {"name": pe["image"]["name"], "size": len(data),
                  "sha256": hashlib.sha256(data).hexdigest(),
                  "image_base": _hex(pe["image"]["image_base"]),
                  "entry_point": _hex(pe["image"]["entry_point"])},
        "method": METHOD,
        "executable_intervals": [
            {"section": interval["section"], "rva": _hex(interval["rva"]),
             "end_rva": _hex(interval["end_rva"]), "size": interval["size"]}
            for interval in pe["executable_intervals"]],
        # Counted over the same entries `counts.entries` counts, so the two
        # reconcile whether or not a seed lands outside the executable extent.
        "seed_sources": {
            source: sum(1 for entry in entries if source in entry["sources"])
            for source in SEED_SOURCES},
        # No `fixed_point` flag: both loops exit only when they find nothing, so
        # such a flag would be true for every input, including a broken run. What
        # is published instead is how many seed claims the gate re-derived and
        # confirmed, and the run fails outright if any one of them is unrecorded.
        "discovery": {"rounds": rounds, "iterations": iterations,
                      "rechecked_seeds": len(rechecked)},
        "entries": entries,
        "seeds_outside_executable": outside,
        "relocation_uses": relocation_uses,
        "chunks": chunks,
        "instructions": [corpus.instructions[rva] for rva in sorted(corpus.instructions)],
        "undecodable": [corpus.undecodable[rva] for rva in sorted(corpus.undecodable)],
        "switch_tables": sorted(corpus.switch_tables, key=lambda table: table["jump_rva"]),
        "import_thunks": sorted(corpus.import_thunks, key=lambda thunk: thunk["rva"]),
        "tail_calls": tail_calls,
        "overlaps": overlaps,
        "coverage_gaps": gaps,
        "counts": {
            "entries": len(entries), "chunks": len(chunks),
            "instructions": len(corpus.instructions),
            # Named for which pass refused the byte: the descent's refusals are a
            # byte state of their own, the sweep's sit inside a coverage gap.
            "descent_undecodable": len(corpus.undecodable),
            "sweep_undecodable": swept_undecodable,
            "switch_tables": len(corpus.switch_tables),
            "import_thunks": len(corpus.import_thunks), "tail_calls": len(tail_calls),
            "unresolved_indirect_jumps": _unresolved_indirect_jumps(corpus),
            "coverage_gaps": len(gaps), "overlaps": len(overlaps),
            "seeds_outside_executable": len(outside),
            "relocation_uses": len(relocation_uses),
            "chunks_without_entry": sum(1 for chunk in chunks
                                        if int(chunk["rva"], 16) not in corpus.entries),
        },
        "coverage": {
            "executable_bytes": text.size,
            "instruction_bytes": state.count(1),
            "undecodable_bytes": state.count(2),
            "unclassified_bytes": state.count(0),
            "proposed_alignment_bytes": proposed["alignment"],
            "proposed_code_bytes": proposed["code"],
            "proposed_data_bytes": proposed["data"],
            # Already inside unclassified_bytes, not a fourth state: these are
            # positions the sweep could not decode while proposing what a gap holds.
            "sweep_undecodable_bytes": swept_undecodable,
        },
    }


def serialize(manifest) -> str:
    return json.dumps(manifest, separators=(",", ":")) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", required=True, help="PE32 image to disassemble")
    parser.add_argument("--pe", required=True, help="PE oracle manifest")
    parser.add_argument("--ghidra", required=True, help="normalized Ghidra oracle manifest")
    parser.add_argument("--output", required=True, help="Capstone corpus JSON to write")
    args = parser.parse_args()

    try:
        manifest = build_manifest(
            Path(args.binary).read_bytes(),
            json.loads(Path(args.pe).read_text(encoding="utf-8")),
            json.loads(Path(args.ghidra).read_text(encoding="utf-8")))
        output = Path(args.output)
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(serialize(manifest), encoding="utf-8", newline="\n")
    except (OSError, ValueError) as error:
        parser.exit(2, f"error: {error}\n")

    counts = manifest["counts"]
    coverage = manifest["coverage"]
    print("capstone=written")
    print(f"version={manifest['capstone']['version']}")
    print(f"rounds={len(manifest['discovery']['rounds'])}")
    print(f"iterations={len(manifest['discovery']['iterations'])}")
    print(f"rechecked_seeds={manifest['discovery']['rechecked_seeds']}")
    for name in ("entries", "chunks", "instructions", "descent_undecodable",
                 "sweep_undecodable", "switch_tables", "import_thunks", "tail_calls",
                 "unresolved_indirect_jumps", "relocation_uses", "coverage_gaps",
                 "overlaps", "seeds_outside_executable"):
        print(f"{name}={counts[name]}")
    for name in ("instruction_bytes", "undecodable_bytes", "unclassified_bytes",
                 "sweep_undecodable_bytes", "proposed_alignment_bytes",
                 "proposed_code_bytes", "proposed_data_bytes", "executable_bytes"):
        print(f"{name}={coverage[name]}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
