#!/usr/bin/env python3
"""Execution coverage of the matched vendored qhull/OPCODE groups, by cdb breakpoint trace.

The Phase 4 differentials are oracle differentials: the candidate side of every comparison is
the vendored code linked into the test executable itself (the NxQhull and NxOpcode static
libraries), not a staged NxPhysics.dll. So the binary that executes candidate code is the test
exe, and that is what is traced. Three steps, one subcommand each:

  identity  Show that each matched group's function in the exe is the same code as in the
            shipped candidate NxPhysics.dll. Both link the same NxQhull.lib/NxOpcode.lib
            objects; what a linker may still change is the addresses. The bodies are compared
            instruction by instruction: identical bytes, except that a relocated 32-bit value
            and a relative branch or call target are compared as the symbol (plus offset) they
            resolve to in each image's own linker map, and an import slot as the imported name.
  script    Write a cdb command file: one counting breakpoint per distinct exe address of a
            matched group (MATCH, SHAPE, REVIEW, DIFF), counters in a page the script allocates
            with .dvalloc at a fixed address, and boundary breakpoints that print the family
            the harness just reported (the first argument of `nxReport`) and dump and clear the
            counters. A counter that reaches --cap disables its breakpoint until the next
            boundary, so hot recursive traversals cost at most cap hits per family.
  parse     Read the cdb log back: per segment (family) and per group, the hit count.

`report` writes the committed trace excerpt for one library (evidence/vendored-trace-*.txt):
the binaries' sha256, the per-group counters per family, and the head of the raw cdb log.

`coverage` joins one or more parsed traces with the identity results into the per-group
coverage CSV (evidence/phase4-third-party-map/vendored_coverage.csv). `hits` sums the capped
per-family counts (a trailing `+` means some family reached the cap).

Classes. Each family the harness reports is classified from its own line (family_class):
`exact` (verdict exact: no word differs), `lastbit` (a divergent family in which no discrete
word differs, the tapes are the same length, and every differing float or double is at most
LASTBIT_ULP representable values from the oracle's), or `discrete` (anything else: a differing
count, index, verdict, tree link or quantized box, a length difference, a sign change, or a
float further apart than the bound). A `FAILED` family is `failed`.

Executions. A trace segment runs from one harness report to the next, so a segment's hits
are executions whose outputs the family reported at its end compares. A harness drive can fill
several families from the same executions and report them back to back (`opcode_ray`, then
`opcode_ray_x87` and `opcode_ray_boundary`, with nothing executed in between); the segments
of the later ones hold no hits. So an execution (a drive) is a segment with hits plus the
following hit-less segments whose family is its sibling (`<family>_<suffix>`), and its class is
the worst class of those families: the floats an execution produced are part of its outcome.

`differential` lists, per drive in which the group executed, the drive's families with their
classes. A breakpoint sees only the candidate's out-of-line copy of a group, so a group the
candidate compiler inlines into a caller is also credited with the caller's drives, marked
`<harness>[via-inline:<caller symbol>]:...` (inline_callers): the matcher's inlining note
"candidate inlines G" on the caller, or an oracle-only call to G in the caller's `diff_calls`,
means the oracle calls G's out-of-line row from that caller where the candidate runs an inlined
copy. Those entries list families; they do not change `outcome` or `family_best`, which stay
about executions of the out-of-line body that identity checks. `outcome` is the group's best drive (exact < lastbit < discrete < failed): `exact` if
some execution of it was compared and matched completely. `family_best` is the best class of
any family the group ran in, whatever else its execution fed -- the looser reading, kept so the
two can be told apart. `layout` means only the ThirdParty harness's candidate-only layout
assertions ran it (execution, not a compared outcome); `uncompared` that only segments no family
reports ran it (a `--mark` boundary such as `release`, the harness's model clean-up, or the
ThirdParty harness's `END` after its last report); `none` that nothing ran it.

main() returns 0 on success, 1 when identity finds a body that differs, and 2 on bad input.
"""

import argparse
import csv
import json
import re
import sys
from collections import OrderedDict, defaultdict
from pathlib import Path

import vendored_match as vm

TRACED_CLASSES = ("MATCH", "SHAPE", "REVIEW", "DIFF")
COUNTER_BASE = 0x60000000
COVERAGE_CSV = vm.MAP_DIR / "vendored_coverage.csv"
BOUNDARY_SYMBOL = "?nxReport@@YAXPBD000_NI@Z"
# The last-bit bound, in representable values; the harness applies the same one (kLastBitUlp in
# tests/PhysicsThirdPartyTests.cpp) to count `beyond=`. Two roundings on each side of a
# reassociated or unrounded three-term sum; see evidence/vendored-correspondence.md, Task 5a.
LASTBIT_ULP = 4
CLASS_ORDER = ("exact", "lastbit", "discrete", "failed")


# --------------------------------------------------------------------------- groups

def load_groups(map_dir=vm.MAP_DIR):
    """Matched groups of both libraries, in match-CSV order: one per candidate function.

    A row tagged as a separate instantiation keeps its own group, keyed by the oracle rva, the
    way vendored_match.group_rows keys it; such a group shares its candidate symbol with the
    first instantiation.
    """
    groups = OrderedDict()
    for library in ("qhull", "opcode"):
        path = Path(map_dir) / f"{library}_match.csv"
        with path.open(encoding="utf-8", newline="") as handle:
            for r in csv.DictReader(handle):
                symbol = r["candidate_symbol"]
                if not symbol:
                    continue
                _, tags = vm.parse_source_function(r["source_function"])
                separate = any("instantiation" in t for t in tags)
                key = f"{symbol}#{r['rva'][2:]}" if separate else symbol
                g = groups.get((library, key))
                if g is None:
                    g = groups[(library, key)] = {
                        "library": library, "key": key, "symbol": symbol,
                        "class": r["class"], "rows": [], "separate": separate,
                        "source_function": r["source_function"],
                        "candidate_rva": int(r["candidate_rva"], 16),
                        "candidate_size": int(r["candidate_size"] or 0),
                    }
                g["rows"].append((int(r["rva"], 16), int(r["size"])))
    return list(groups.values())


INLINE_NOTE = re.compile(r"candidate inlines (.+)")


def _shape_inlined(shape):
    """Identities the matcher's inlining note says the candidate inlines ("inlining: candidate
    inlines A, oracle inlines B, candidate inlines C; ..."). Overload identities list their
    parameters without spaces, so ", " separates the entries."""
    out = []
    for part in shape.split("; "):
        if not part.startswith("inlining: "):
            continue
        for entry in part[len("inlining: "):].split(", "):
            m = INLINE_NOTE.fullmatch(entry.strip())
            if m:
                out.append(m.group(1))
    return out


def group_identities(groups):
    """{call identity: group key} for (library, key), as the matcher names call targets: the
    candidate symbol's short key, and for an overload the key with its parameter list."""
    by_short = defaultdict(list)
    exact = {}
    for g in groups:
        d = vm.demangle(g["symbol"])
        if d is None or not d.components:
            continue
        k = vm.short_key(d.components)
        by_short[(g["library"], k)].append(g)
        if d.params is not None:
            exact[(g["library"], f"{k}({','.join(d.params)})")] = g
    out = {}
    for (lib, k), gs in by_short.items():
        if len({g["key"] for g in gs}) == 1:
            out[(lib, k)] = gs[0]
    out.update(exact)
    return out


def inline_callers(groups, match_rows):
    """{(library, callee key): [caller group, ...]}: the groups the candidate inlines into a
    caller, from the caller's match rows (the inlining note, and oracle-only calls left in
    diff_calls). match_rows: [(library, row dict)]."""
    by_row = {}
    for g in groups:
        for rva, _ in g["rows"]:
            by_row[(g["library"], rva)] = g
    idents = group_identities(groups)
    out = defaultdict(list)
    for lib, r in match_rows:
        caller = by_row.get((lib, int(r["rva"], 16)))
        if caller is None:
            continue
        names = _shape_inlined(r.get("shape", ""))
        names += [t[1:] for t in (r.get("diff_calls") or "").split("; ") if t.startswith("-")]
        for name in names:
            callee = idents.get((lib, name))
            if callee is None or callee["key"] == caller["key"]:
                continue
            bucket = out[(lib, callee["key"])]
            if all(c["key"] != caller["key"] for c in bucket):
                bucket.append(caller)
    return out


def load_match_rows(map_dir=vm.MAP_DIR):
    rows = []
    for library in ("qhull", "opcode"):
        with (Path(map_dir) / f"{library}_match.csv").open(encoding="utf-8", newline="") as handle:
            rows.extend((library, r) for r in csv.DictReader(handle))
    return rows


def x87_count(image, rows):
    """x87 instructions in the oracle rows of a group (linear sweep of each row)."""
    count = 0
    cs = vm._disassembler()
    for rva, size in rows:
        raw = image.read(rva, size)
        if raw is None:
            continue
        for insn in cs.disasm(raw, rva):
            if vm._is_float_insn(insn) == "x87":
                count += 1
    return count


# --------------------------------------------------------------------------- identity

class Mapped:
    """An image with its linker map: symbol lookup by address."""

    def __init__(self, image, symbols, entry_rva=0):
        self.image = image
        self.entry_rva = entry_rva
        self.symbols = symbols
        self.index = vm.SymbolIndex(symbols)
        self.by_name = defaultdict(list)
        for s in symbols:
            self.by_name[s.name].append(s)

    @classmethod
    def load(cls, image_path, map_path):
        import pefile
        image = vm.Image.from_pe(image_path)
        entry = pefile.PE(str(image_path), fast_load=True).OPTIONAL_HEADER.AddressOfEntryPoint
        text = Path(map_path).read_text(encoding="utf-8", errors="replace")
        return cls(image, vm.parse_map(text, image.base, image), entry)

    def function(self, name):
        found = [s for s in self.by_name.get(name, []) if s.is_function]
        return found[0] if found else None

    def describe(self, rva):
        """Position-independent names for an address: {import} or {symbol+offset, ...}.

        A set, because identical-COMDAT folding gives one address several names (the test
        exes link with the default /OPT:ICF), and two addresses are the same target when
        their name sets meet.
        """
        if rva in self.image.imports:
            return {"imp:" + self.image.imports[rva]}
        i = vm.bisect.bisect_right(self.index.rvas, rva) - 1
        if i < 0:
            return {f"?{rva:x}"}
        start = self.index.sorted[i].rva
        names = set()
        for s in self.index.sorted[vm.bisect.bisect_left(self.index.rvas, start):i + 1]:
            name = s.name
            if name.startswith("__imp_"):
                name = "imp:" + name[6:].lstrip("_")
            names.add(f"{name}+{rva - start:x}")
        return names


def _branch_target(insn):
    if insn.group(vm.capstone.CS_GRP_JUMP) or insn.group(vm.capstone.CS_GRP_CALL):
        ops = insn.operands
        if len(ops) == 1 and ops[0].type == vm.x86.X86_OP_IMM:
            return ops[0].imm
    return None


def _same(x, y):
    return bool(x & y)


def _show(x):
    return "|".join(sorted(x))


def compare_bodies(dll, exe, dll_sym, exe_sym, size):
    """None when the two bodies are the same code, else a description of the first difference.

    The walk is linear from the entry and stops at the first ret or unconditional jmp past the
    furthest local branch target, so alignment padding after the body is not compared.
    """
    a = dll.image.read(dll_sym.rva, size)
    b = exe.image.read(exe_sym.rva, size)
    if a is None or b is None:
        return "body not readable"
    cs = vm._disassembler()
    pos, reach = 0, 0
    while pos < size:
        ia = next(cs.disasm(a[pos:pos + 16], dll_sym.rva + pos, 1), None)
        ib = next(cs.disasm(b[pos:pos + 16], exe_sym.rva + pos, 1), None)
        if ia is None or ib is None:
            if a[pos] != b[pos]:
                return f"+{pos:x}: undecodable byte differs"
            pos += 1
            continue
        if ia.size != ib.size or ia.mnemonic != ib.mnemonic:
            return f"+{pos:x}: {ia.mnemonic} {ia.op_str} / {ib.mnemonic} {ib.op_str}"
        ta, tb = _branch_target(ia), _branch_target(ib)
        masked = set()
        if ta is not None and tb is not None:
            ra, rb = ta - dll_sym.rva, tb - exe_sym.rva
            inside = 0 <= ra < size and 0 <= rb < size
            if inside and ra != rb:
                return f"+{pos:x}: local branch +{ra:x} / +{rb:x}"
            if inside:
                reach = max(reach, ra)
            elif not _same(dll.describe(ta), exe.describe(tb)):
                return f"+{pos:x}: {_show(dll.describe(ta))} / {_show(exe.describe(tb))}"
            masked = set(range(ia.size))
        for off in range(ia.size - 3):
            ra, rb = ia.address + off, ib.address + off
            if ra in dll.image.relocations or rb in exe.image.relocations:
                if (ra in dll.image.relocations) != (rb in exe.image.relocations):
                    return f"+{pos + off:x}: relocation on one side only"
                va = dll.image.u32(ra) - dll.image.base
                vb = exe.image.u32(rb) - exe.image.base
                if not _same(dll.describe(va), exe.describe(vb)):
                    return (f"+{pos + off:x}: {_show(dll.describe(va))} / "
                            f"{_show(exe.describe(vb))}")
                masked.update(range(off, off + 4))
        for k in range(ia.size):
            if k not in masked and a[pos + k] != b[pos + k]:
                return f"+{pos + k:x}: byte {a[pos + k]:02x} / {b[pos + k]:02x} in {ia.mnemonic}"
        pos += ia.size
        if pos > reach and (ia.mnemonic.startswith("ret") or ia.mnemonic == "jmp"):
            break
    return None


def identity(groups, dll, exes):
    """Per group and exe: (exe rva or None, verdict)."""
    out = {}
    for g in groups:
        dsym = dll.function(g["symbol"])
        for name, exe in exes.items():
            esym = exe.function(g["symbol"])
            if dsym is None:
                out[(g["key"], name)] = (None, "not in dll map")
            elif esym is None:
                out[(g["key"], name)] = (None, "absent")
            else:
                size = g["candidate_size"] or (dsym.end - dsym.rva)
                diff = compare_bodies(dll, exe, dsym, esym, size)
                origin = "" if esym.obj == dsym.obj else f" (exe copy from {esym.obj})"
                out[(g["key"], name)] = (esym.rva, ("same" if diff is None else "DIFFERS " + diff)
                                         + origin)
    return out


# --------------------------------------------------------------------------- cdb script

def breakpoints(groups, exe):
    """Distinct exe addresses of the traced groups, with the group keys at each."""
    at = OrderedDict()
    for g in groups:
        if g["class"] not in TRACED_CLASSES:
            continue
        s = exe.function(g["symbol"])
        if s is None:
            continue
        at.setdefault(s.rva, []).append(g["key"])
    return at


def write_script(at, entry_rva, boundary_rva, cap, path, marks=()):
    """The cdb command file for one run. Counter i lives at COUNTER_BASE + 4*i.

    A boundary prints `SEG <label>` and then dumps and clears the counters, so the counts after
    a SEG line are the hits since the previous boundary. `boundary_rva` takes its label from the
    string at [esp+4]; each (rva, label) in `marks` prints its fixed label.

    The exe has no PDB, so cdb names it image<base> and a module-relative expression cannot
    use its name. The base is taken from the entry point instead: $t19 = $exentry - entry_rva.
    """
    n = len(at)
    module = "@$t19"
    lines = [f"r $t19 = @$exentry - 0x{entry_rva:x}",
             f".dvalloc /b 0x{COUNTER_BASE:08x} 0x{max(0x1000, 4 * n):x}"]
    dump = (f"dd 0x{COUNTER_BASE:08x} L0n{n}; "
            f"f 0x{COUNTER_BASE:08x} L0n{4 * n} 0; be *")
    first = 1 + len(marks)
    for k, (rva, label) in enumerate(marks):
        lines.append(f'bp{1 + k} {module}+0x{rva:x} ".printf \\"SEG {label}\\\\n\\"; '
                     f'{dump}; gc"')
    if boundary_rva is not None:
        lines.append(f'bp{first} {module}+0x{boundary_rva:x} ".printf \\"SEG %ma\\\\n\\", '
                     f'poi(@esp+4); {dump}; gc"')
    for i, rva in enumerate(at):
        bp = first + 1 + i
        cell = f"0x{COUNTER_BASE + 4 * i:08x}"
        lines.append(f'bp{bp} {module}+0x{rva:x} "ed {cell} (dwo({cell})+1); '
                     f'.if (dwo({cell}) >= 0n{cap}) {{bd {bp}}}; gc"')
    lines.append("g")
    lines.append(f'.printf "SEG END\\n"; dd 0x{COUNTER_BASE:08x} L0n{n}')
    lines.append("q")
    Path(path).write_text("\n".join(lines) + "\n", encoding="ascii")


DD_LINE = re.compile(r"^([0-9a-f]{8})\s+((?:[0-9a-f]{8}\s*)+)$")


def parse_log(text, n):
    """[(segment label, [count per breakpoint index])] in the order the log prints them."""
    segments = []
    label, cells = None, {}
    for line in text.splitlines():
        line = line.strip()
        if line.startswith("SEG "):
            if label is not None:
                segments.append((label, [cells.get(i, 0) for i in range(n)]))
            label, cells = line[4:].strip(), {}
            continue
        m = DD_LINE.match(line)
        if m and label is not None:
            address = int(m.group(1), 16)
            if not COUNTER_BASE <= address < COUNTER_BASE + 4 * n + 16:
                continue
            for k, word in enumerate(m.group(2).split()):
                i = (address - COUNTER_BASE) // 4 + k
                if i < n:
                    cells[i] = int(word, 16)
    if label is not None:
        segments.append((label, [cells.get(i, 0) for i in range(n)]))
    return segments


# --------------------------------------------------------------------------- main

VERDICT_LINE = re.compile(r"^thirdparty name=(\S+) .* verdict=(\w+)$")
FIELD = re.compile(r" (mismatches|discrete|float_ulp|double_ulp|beyond|length_delta)=(\S+)")
ASSET_LINE = re.compile(r"^asset oracle digest=\S+ expect_mismatches=(\d+)")


def parse_families(text):
    """{family: {"verdict": ..., and the divergence fields the line carries}} from the harness's
    own lines in the cdb log.

    The ThirdParty harness prints `verdict=exact|divergent|FAILED` per family, and a divergent
    family's line also carries mismatches=, discrete=, float_ulp=, double_ulp=, beyond= and
    length_delta= (an ulp field may read `inf`). The asset harness has one family, `asset`,
    printed as exact when its expect_mismatches count is 0.
    """
    families = {}
    for line in text.splitlines():
        line = line.strip()
        m = VERDICT_LINE.match(line)
        if m:
            entry = {"verdict": m.group(2)}
            for key, value in FIELD.findall(line):
                entry[key] = value
            families[m.group(1)] = entry
        m = ASSET_LINE.match(line)
        if m:
            families["asset"] = {"verdict": "exact" if m.group(1) == "0" else "FAILED"}
    return families


def parse_verdicts(text):
    """{family: verdict}: parse_families without the divergence fields."""
    return {name: entry["verdict"] for name, entry in parse_families(text).items()}


def _ulp(value):
    return float("inf") if value == "inf" else int(value)


def family_class(entry):
    """exact | lastbit | discrete | failed | ? for one family's parsed line (see the module
    docstring). A divergent line without its distance fields cannot be called last-bit."""
    verdict = entry.get("verdict")
    if verdict == "exact":
        return "exact"
    if verdict == "FAILED":
        return "failed"
    if verdict != "divergent":
        return "?"
    try:
        discrete = int(entry["discrete"])
        delta = int(entry["length_delta"])
        worst = max(_ulp(entry["float_ulp"]), _ulp(entry["double_ulp"]))
    except (KeyError, ValueError):
        return "discrete"
    if discrete == 0 and delta == 0 and worst <= LASTBIT_ULP:
        return "lastbit"
    return "discrete"


def drives(segments):
    """[(segment index, [family, ...])]: one entry per segment with hits, carrying the sibling
    families reported right after it from hit-less segments (see the module docstring)."""
    out = []
    for i, seg in enumerate(segments):
        label = seg["label"]
        if seg["hits"] or not out:
            out.append((i, [label]))
            continue
        head = out[-1][1][0]
        if label.startswith(head + "_"):
            out[-1][1].append(label)
        else:
            out.append((i, [label]))
    return out


def worst(classes):
    ranked = [c for c in classes if c in CLASS_ORDER]
    if len(ranked) != len(classes) or not ranked:
        return "?"
    return max(ranked, key=CLASS_ORDER.index)


def best(classes):
    ranked = [c for c in classes if c in CLASS_ORDER]
    if not ranked:
        return None
    return min(ranked, key=CLASS_ORDER.index)


UNCOMPARED = ("layout", "uncompared")


def outcome_of(drive_classes):
    """The group's outcome over the classes of the drives it ran in ("layout" for the layout
    assertions, "uncompared" for a segment no family reports): its best compared drive, else
    layout, else uncompared, else none."""
    compared = [c for c in drive_classes if c not in UNCOMPARED]
    if compared:
        return best(compared) or "+".join(sorted(set(compared)))
    for label in UNCOMPARED:
        if label in drive_classes:
            return label
    return "none"


def cmd_identity(args, groups):
    dll = Mapped.load(args.dll, args.dll_map)
    exes = {name: Mapped.load(path, mp) for name, path, mp in
            (spec.split(",") for spec in args.exe)}
    result = identity(groups, dll, exes)
    bad = 0
    with open(args.out, "w", encoding="utf-8", newline="") as handle:
        w = csv.writer(handle, lineterminator="\n")
        w.writerow(["library", "group", "exe", "exe_rva", "verdict"])
        for g in groups:
            for name in exes:
                rva, verdict = result[(g["key"], name)]
                w.writerow([g["library"], g["key"], name,
                            "" if rva is None else f"0x{rva:08x}", verdict])
                bad += verdict.startswith("DIFFERS")
    print(f"identity groups={len(groups)} differing={bad}")
    return 1 if bad else 0


def cmd_script(args, groups):
    exe = Mapped.load(args.exe, args.exe_map)
    at = breakpoints(groups, exe)
    boundary = exe.function(BOUNDARY_SYMBOL)
    marks = []
    for spec in args.mark:
        symbol, label = spec.rsplit("=", 1)
        s = exe.function(symbol)
        if s is None:
            raise ValueError(f"mark symbol {symbol} not in the exe map")
        marks.append((s.rva, label))
    entry = exe.entry_rva
    write_script(at, entry, boundary.rva if boundary else None, args.cap, args.out, marks)
    index = [{"rva": f"0x{rva:08x}", "groups": keys} for rva, keys in at.items()]
    Path(args.index).write_text(json.dumps(index, indent=1) + "\n", encoding="utf-8")
    print(f"script breakpoints={len(at)} boundary={'yes' if boundary else 'no'}")
    return 0


def cmd_parse(args, groups):
    index = json.loads(Path(args.index).read_text(encoding="utf-8"))
    text = Path(args.log).read_text(encoding="utf-8", errors="replace")
    segments = parse_log(text, len(index))
    families = parse_families(text)
    out = {"segments": [], "verdicts": {k: v["verdict"] for k, v in families.items()},
           "families": families}
    for label, counts in segments:
        hits = {}
        for entry, count in zip(index, counts):
            if count:
                for key in entry["groups"]:
                    hits[key] = count
        out["segments"].append({"label": label, "hits": hits})
    Path(args.out).write_text(json.dumps(out, indent=1) + "\n", encoding="utf-8")
    print(f"parse segments={len(segments)}")
    return 0


def cmd_coverage(args, groups):
    oracle = vm.Image.from_pe(args.oracle)
    identities = {}
    with open(args.identity, encoding="utf-8", newline="") as handle:
        for r in csv.DictReader(handle):
            identities[(r["group"], r["exe"])] = r["verdict"]
    traces = []
    for spec in args.trace:
        harness, path = spec.split(",", 1)
        traces.append((harness, json.loads(Path(path).read_text(encoding="utf-8"))))
    cap = args.cap
    with open(args.out, "w", encoding="utf-8", newline="") as handle:
        w = csv.writer(handle, lineterminator="\n")
        w.writerow(["library", "rva", "class", "candidate_symbol", "x87", "identity", "hits",
                    "outcome", "family_best", "differential", "source_function"])
        prepared = []
        for harness, trace in traces:
            families = trace.get("families") or {k: {"verdict": v} for k, v in trace.get("verdicts", {}).items()}
            classes = {name: family_class(entry) for name, entry in families.items()}
            segments = trace["segments"]
            plan = []
            for index, names in drives(segments):
                names = ["asset" if n == "END" and harness == "asset" else n for n in names]
                plan.append((index, names))
            prepared.append((harness, segments, plan, classes))
        callers = inline_callers(groups, load_match_rows(args.map_dir))

        def executed(g):
            """[(harness, names, member classes)] for the drives in which g's body ran."""
            out = []
            for harness, segments, plan, classes in prepared:
                for index, names in plan:
                    if segments[index]["hits"].get(g["key"], 0) and names != ["layout"]                             and any(n in classes for n in names):
                        out.append((harness, names, [classes.get(n, "?") for n in names]))
            return out

        for g in groups:
            hits, saturated, listed, idents, drive_classes, family_classes = 0, False, [], [], [], []
            for harness, segments, plan, classes in prepared:
                idents.append(f"{harness}:{identities.get((g['key'], harness), 'n/a')}")
                for index, names in plan:
                    count = segments[index]["hits"].get(g["key"], 0)
                    if not count:
                        continue
                    hits += count
                    saturated |= count >= cap
                    if names == ["layout"]:
                        listed.append(f"{harness}:layout")
                        drive_classes.append("layout")
                        continue
                    if not any(n in classes for n in names):
                        listed.append(f"{harness}:" + "+".join(names) + "=uncompared")
                        drive_classes.append("uncompared")
                        continue
                    member = [classes.get(n, "?") for n in names]
                    listed.append(f"{harness}:" + "+".join(f"{n}={c}" for n, c in zip(names, member)))
                    drive_classes.append(worst(member))
                    family_classes.extend(member)
            if g["class"] not in TRACED_CLASSES:
                hits_text, outcome, loose = "untraced", "untraced", ""
            elif g["separate"]:
                hits_text, outcome, loose = "no candidate body of its own", "none", "none"
                listed = []
            else:
                hits_text = f"{hits}+" if saturated else str(hits)
                outcome = outcome_of(drive_classes)
                loose = best(family_classes) or outcome
                seen = set()
                for caller in callers.get((g["library"], g["key"]), []):
                    for harness, names, member in executed(caller):
                        entry = (f"{harness}[via-inline:{caller['key']}]:" +
                                 "+".join(f"{n}={c}" for n, c in zip(names, member)))
                        if entry not in seen:
                            seen.add(entry)
                            listed.append(entry)
            w.writerow([g["library"], f"0x{g['rows'][0][0]:08x}", g["class"], g["symbol"],
                        x87_count(oracle, g["rows"]), ";".join(idents), hits_text, outcome, loose,
                        ";".join(listed), g["source_function"]])
    print(f"coverage groups={len(groups)}")
    return 0


def _sha256(path):
    import hashlib
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def _rel(path):
    try:
        return str(Path(path).resolve().relative_to(vm.REPO_DIR.resolve()))
    except ValueError:
        return str(path)


def cmd_report(args, groups):
    """The committed trace excerpt for one library: header, per-group counts, raw log head."""
    identities = {}
    with open(args.identity, encoding="utf-8", newline="") as handle:
        for r in csv.DictReader(handle):
            identities[(r["group"], r["exe"])] = (r["exe_rva"], r["verdict"])
    traces = []
    for spec in args.trace:
        harness, parsed, log, exe = spec.split(",")
        traces.append((harness, json.loads(Path(parsed).read_text(encoding="utf-8")),
                       Path(log).read_text(encoding="utf-8", errors="replace"), exe))
    first_log = traces[0][2]
    debugger = next((l.strip() for l in first_log.splitlines() if "Windows Debugger Version" in l), "?")
    oracle = next((l.strip() for l in first_log.splitlines() if l.startswith("oracle module path=")), "?")
    out = [args.title, "=" * len(args.title), ""]
    out += [line.rstrip() for line in Path(args.preamble).read_text(encoding="utf-8").splitlines()]
    out += ["", f"Debugger:  {debugger}", f"Oracle:    {oracle}",
            f"Candidate: {_rel(args.dll)} sha256={_sha256(args.dll)}"]
    for harness, parsed, log, exe in traces:
        out.append(f"Traced:    {harness}: {_rel(exe)} sha256={_sha256(exe)}")
        fams = parsed.get("families") or {k: {"verdict": v} for k, v in parsed.get("verdicts", {}).items()}
        out.append(f"           families and classes: " + ", ".join(
            f"{k}={family_class(v)}" for k, v in fams.items()))
    out += ["", "Per-group counts. One line per matched group of this library in match-CSV order:",
            "oracle rva, class, candidate symbol, and for each harness the exe address traced and",
            "the counter per segment (the family the harness reported at the segment's end; `layout`",
            f"is the ThirdParty harness's candidate-only layout assertions). A count of {args.cap} is",
            "the cap: the breakpoint was disabled for the rest of that segment. `-` = not in the exe.",
            ""]
    executed = 0
    listed = [g for g in groups if g["library"] == args.library and g["class"] in TRACED_CLASSES]
    for g in listed:
        parts = []
        any_hit = False
        for harness, parsed, log, exe in traces:
            rva, verdict = identities.get((g["key"], harness), ("", "n/a"))
            if not rva:
                parts.append(f"{harness}:-")
                continue
            counts = [f"{seg['label']}={seg['hits'][g['key']]}" for seg in parsed["segments"]
                      if seg["hits"].get(g["key"])]
            any_hit |= bool(counts)
            note = "" if verdict.startswith("same") else f" [{verdict}]"
            parts.append(f"{harness}@{rva}:" + (" ".join(counts) if counts else "0") + note)
        executed += any_hit
        tag = " (separate instantiation: no candidate body of its own)" if g["separate"] else ""
        out.append(f"0x{g['rows'][0][0]:08x} {g['class']:6} {g['symbol']}{tag}")
        out.append("    " + " | ".join(parts))
    out += ["", f"groups={len(listed)} executed={executed}", "",
            "Raw log head (the ThirdParty run): the script's first commands and the first three",
            "boundary dumps. Edited only by omission: counting-breakpoint commands after the first",
            "dozen, and counter lines that are all zero, are left out; lines are cut at 160 columns.",
            ""]
    raw = traces[0][2].splitlines()
    start = next((i for i, l in enumerate(raw) if "r $t19" in l), 0)
    kept, segs = [], 0
    for line in raw[start:]:
        if line.startswith("SEG "):
            segs += 1
            if segs > 3:
                break
        if " bp" in line[:12] and "ed 0x6" in line and len(kept) > 12:
            continue
        m = DD_LINE.match(line.strip())
        if m and all(int(w, 16) == 0 for w in m.group(2).split()):
            continue
        kept.append(line.rstrip()[:160])
    out += kept
    Path(args.out).write_text("\n".join(out) + "\n", encoding="utf-8")
    print(f"report library={args.library} groups={len(listed)} executed={executed}")
    return 0


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--map-dir", default=str(vm.MAP_DIR))
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("identity")
    p.add_argument("--dll", default=str(vm.REPO_DIR / "build" / "Release" / "NxPhysics.dll"))
    p.add_argument("--dll-map", default=str(vm.REPO_DIR / "build" / "Release" / "NxPhysics.map"))
    p.add_argument("--exe", action="append", required=True, help="name,exe path,map path")
    p.add_argument("--out", required=True)
    p = sub.add_parser("script")
    p.add_argument("--exe", required=True)
    p.add_argument("--exe-map", required=True)
    p.add_argument("--cap", type=int, default=100000)
    p.add_argument("--mark", action="append", default=[],
                   help="symbol=label: a boundary at the symbol's entry with a fixed label")
    p.add_argument("--out", required=True)
    p.add_argument("--index", required=True)
    p = sub.add_parser("parse")
    p.add_argument("--log", required=True)
    p.add_argument("--index", required=True)
    p.add_argument("--out", required=True)
    p = sub.add_parser("coverage")
    p.add_argument("--oracle", default=str(vm.DEFAULT_ORACLE))
    p.add_argument("--identity", required=True)
    p.add_argument("--trace", action="append", required=True,
                   help="harness,parsed json")
    p.add_argument("--cap", type=int, default=100000)
    p.add_argument("--out", default=str(COVERAGE_CSV))
    p = sub.add_parser("report")
    p.add_argument("--library", required=True, choices=("qhull", "opcode"))
    p.add_argument("--title", required=True)
    p.add_argument("--preamble", required=True, help="text file with the method paragraph")
    p.add_argument("--identity", required=True)
    p.add_argument("--trace", action="append", required=True,
                   help="harness,parsed json,cdb log,traced exe")
    p.add_argument("--dll", default=str(vm.REPO_DIR / "build" / "Release" / "NxPhysics.dll"))
    p.add_argument("--cap", type=int, default=5)
    p.add_argument("--out", required=True)
    args = parser.parse_args(argv)
    try:
        groups = load_groups(args.map_dir)
        handler = {"identity": cmd_identity, "script": cmd_script, "parse": cmd_parse,
                   "coverage": cmd_coverage, "report": cmd_report}[args.command]
        return handler(args, groups)
    except (OSError, ValueError, KeyError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
