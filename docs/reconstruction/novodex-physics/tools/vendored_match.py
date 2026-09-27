#!/usr/bin/env python3
"""Match each vendored qhull/OPCODE row of the oracle against its counterpart in the candidate.

The Phase 4 maps (`evidence/phase4-third-party-map/{qhull,opcode,opcode_outside_span}_map.csv`)
pair every oracle row with an upstream source function. This tool finds that function in the
candidate's linker map, disassembles both bodies, reduces each to a handful of features and
classifies the row. It routes human attention; it does not prove equivalence on its own.

Locating the candidate
    `source_function` is parsed into qualified-name alternatives ("A::f / `scalar deleting
    destructor'" gives two), bracketed tags ("[body]", "[outlined block]") are dropped, and the
    parameter list, when there is one, is normalised to a list of types. Map symbols are demangled
    by a small MSVC demangler (C names lose one leading underscore; `?name@Class@Ns@@...` gives
    Ns::Class::name; ??0/??1/??_G/??_E give ctor/dtor/scalar/vector deleting destructor; a
    local-scope `?N?` qualifier is unwrapped). A symbol matches when its qualified name ends with
    the source's components. Overloads are cut down, in order, by an exact parameter-type match,
    then by the candidate object file's stem equalling the source file's stem; if more than one
    is left the row is AMBIGUOUS. A "scalar deleting destructor" that is absent falls back to the
    vector deleting destructor, and a row whose alternatives include a deleting destructor is
    looked up by that alternative first (the other one names the body inlined into it). The
    candidate extent runs from the symbol to the next map symbol in the same section.

Grouping
    Oracle rows that resolve to the same candidate symbol are one group (the census splits some
    functions into several rows: switch arms, outlined blocks, "[body]" tails). The oracle side
    of a group is the union of its rows; every row of the group gets the group's class. Rows
    tagged "instantiation" are a separate copy of the code and are never grouped. The group's
    primary row is its "[entry]" row, else its first untagged row: a call to it is recursion
    ("self"), a call to any other row of the group is an outlined block and is not a call.

Disassembly
    Recursive descent from the entry (from every row start, for a group) inside the extent:
    conditional branches enqueue both edges, direct jumps inside the extent are followed and a
    jump out of it is a tail call, `jmp [reg*4+table]` enqueues the relocated in-extent entries
    of the table, and ret/int3/hlt/ud2 stop a path. Running off the extent stops a path quietly.
    Both images are read with the same code, so the two sides are measured alike.

Features (every relocated operand is resolved through the image's base relocations)
    calls      direct call and tail-call targets in address order, resolved to identities.
               Oracle: map row -> the candidate symbol's key when the row resolved, else the
               key of its source_function; else ORACLE_KNOWN; else the inventory label (CRT
               names) when it is not a stable id; else "census:<id>". Candidate: the map
               symbol's key. Keys keep the last two name components ("AABBTree::Build",
               "qh_setappend"); when several class or namespace members share a key (overloads)
               each gets its demangled parameter list ("T::Collide(const A*,const A*,int)"), so
               oracle rows, which take the key of the symbol they resolved to, stay apart too.
               Imports are their normalised name, so a UCRT import meets a static-CRT row.
               Memory-indirect calls are "icall[<base>]+0x<disp>", <base> being where the
               called pointer came from (see _icall_tag: host, gobj, getter, obj, fptr, ret, ?);
               register calls only count (shape `icall_reg`, usually a cached import).
               `rep stos`/`rep movs`/`repe cmps` count as memset/memcpy/memcmp. CRT helpers are
               folded (CRT_FOLDS); DROPPED_CALLS (/GS, the UCRT stream accessor) are dropped. A
               candidate call to a NovodeX host wrapper (HOST_SEAMS) counts as the oracle's
               inline form of it.
    strings    relocated immediates that point into a read-only section at a NUL-terminated
               printable string, compared by content; a __FILE__ path by its file name only.
    floats     memory operands of x87 or SSE instructions that land in a read-only section, read
               at the operand's width and compared by value; fldz/fld1/fldpi... count as their
               value. The width multiset and the fchs count are shape features.
    data       every other relocated reference into a non-executable section. Candidate: map
               symbol + offset (/GS's ___security_cookie dropped). Oracle: raw RVA, translated
               through a learned correspondence (below); an oracle address with no
               correspondence stays "oracle:0x...". The qhull host global is not compared
               (ORACLE_HOST_GLOBALS). References to code by a non-call instruction are
               "fnptr:<identity>"; memory operands into code (switch tables, index bytes) only
               count as `switch`.
    fields     byte coverage of structure accesses: for every memory operand of a non-call,
               non-lea, non-string instruction whose base is a register other than esp (and
               other than ebp when the function sets up an ebp frame), with no index register,
               a non-negative displacement and no relocation, the bytes disp..disp+size-1, each
               tagged with the base register's class (_base_class, traced back through register
               copies and ebp spills): "this" (ecx as it came in), "derived" (a pointer loaded
               from memory or computed from another register) or "other". A test/and/or/xor of
               such an operand with an immediate covers only the bytes of the bits it touches.
               Negative displacements and indexed operands are left out: they are
               strength-reduced pointer walks and array elements, whose offsets are the
               compiler's choice. The accesses themselves are kept for the narrowing rule.
    imms       (classifying) the logic/compare immediates: the multiset of immediates of
               cmp/test/and/or/xor/shl/shr/sar/sal, read as signed at the operand size, leaving
               out 0, the x87 status masks (first operand ah/ax) and stack alignment (esp). A
               bit operation on a field becomes a base-classed bit-address token
               "this:bit@N..M" (N = disp*8 + bit; "clear@"/"set@"/"flip@" for and/or/xor that
               write): the memory form, a test/and on a register just loaded from a field
               (within 6 instructions), and a register and/or/xor whose result is stored back to
               the field it was loaded from (read-modify-write). Byte-narrowing so cancels.
               These are where flags, masks, thresholds and enumerators live.
    x87ops     (classifying) the set of x87 operation classes executed: add, sub, mul, div,
               sqrt, abs. Operand order (fsub against fsubr) is not compared.
    other imms (shape only) every other non-relocated immediate, less those of instructions
               whose first operand is esp or ebp. They are reported and do not classify: they
               are mostly store values (float bit patterns, zero-fills, counters), argument byte
               counts and element sizes, which two compilers choose differently for the same
               source, and a store of a different constant also moves a field or a float.
    report     (report only, never classifying) report_jcc: (cmp immediate, following jcc)
               pairs; report_stores: (base class, field, value) for mov [r+d],imm and for
               fldz/fld1 followed by fstp [r+d], the value normalised (0 whatever its type, a
               float bit pattern as the float).
    shape      instruction count, conditional-branch count, x87 and SSE instruction counts,
               fchs count, switch-table references, register calls, float widths, whether an
               ebp frame is set up (the oracle uses ebp as a general register, the candidate
               keeps frames in some functions: a build difference), call order.

Global-data correspondence
    Learned from all resolved groups. Phase A: an oracle address and a candidate symbol+offset
    that co-occur in every group where either appears (Jaccard 1) in at least two groups, or
    Jaccard >= 0.6 in at least three, and are each other's unique best, are paired. Phase B:
    a candidate symbol whose Phase A pairs share one oracle-minus-candidate delta (at least 3
    pairs, 75%) is rigid, and every other oracle address the delta places inside it is paired.
    Phase C: in each group, the still-unpaired oracle addresses and candidate tokens, when equal
    in number, are paired by address rank; a pair needs at least two votes and a strict
    majority. The result is written to vendored_data_map.csv so it can be audited.

Comparison
    calls, strings, floats, data, fields, imms and x87ops are compared as sets after inlining is
    accounted for: a callee that one side calls and the other does not is replaced by that
    callee's own features (oracle: its matched group or inventory row; candidate: its map
    symbol) whenever that shrinks the difference, for up to three rounds. When nothing was
    inlined, imms are compared as a multiset. These differences are shape, not DIFF: occurrence
    counts; memset/memcpy/memmove; the trivial float constants 0.0, -0.0 and 1.0; an oracle-only
    and a candidate-only float whose product is 1 (reciprocal) or 2 (halving), or that are
    negations of each other when the side holding the positive value has more fchs; one-sided
    field bytes that each lie in a wider access of their own side which also covers bytes the
    other side touches (a narrower or wider access to the same field; two adjacent one-byte
    fields are not); field and bit tokens that differ only between the "derived" and "other"
    classes, which classification merges into one pointer class ("this" stays apart).

Classes (a row takes the first that applies)
    MISSING    no candidate symbol. The oracle callers of the row are resolved to candidate
               symbols and named as the likely inliners.
    AMBIGUOUS  more than one candidate symbol survives the overload filters.
    MAPCHECK   the group unites several untagged map rows none of which the map calls a
               "continuation" block: the map may name two functions alike. `compare_class`
               keeps the comparison's class; the map must be checked first.
    DIFF       calls, strings, floats or data differ (listed, "-" oracle only, "+" candidate
               only; `diff_causes` tags them: host-seam, allocator, sqrt-intrinsic,
               vector-iterator, calls, strings, floats, data).
    REVIEW     fields (`diff_fields`), logic/compare immediates (`diff_imms`) or x87
               operation classes (`diff_x87ops`) differ. A
               mandatory hand review: the two bodies call and reference the same things but
               read, write or test different parts of the objects.
    SHAPE      everything above equal; something in shape differs (listed in `shape`, with
               any inlining that was accounted for).
    MATCH      every feature equal.

Limitations
    x87 compare predicates are not canonicalised. A compare is fcom/fcomp/fucomp (or fcomi)
    followed by fnstsw/test ah/jcc (or sahf/jcc); turning that into a relation needs the operand
    order, which two compilers pick independently for two loaded values, and the branch sense,
    which follows block layout. Without data flow either flip is invisible, so the only
    canonical residue (ordered versus equality compare) would add noise, not signal. The status
    masks are therefore excluded from imms and the compare counts sit in the instruction counts.
    SHAPE and MATCH do not prove equivalence: arithmetic order and precision are not compared.
"""

import argparse
import bisect
import csv
import json
import re
import struct
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass, field
from pathlib import Path

import capstone
from capstone import x86

SCRIPT_DIR = Path(__file__).resolve().parent
EV_DIR = SCRIPT_DIR.parent
REPO_DIR = EV_DIR.parents[2]
MAP_DIR = EV_DIR / "evidence" / "phase4-third-party-map"
DEFAULT_ORACLE = Path(r"D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll")

LIBRARIES = {
    "qhull": ["qhull_map.csv"],
    "opcode": ["opcode_map.csv", "opcode_outside_span_map.csv"],
}
MATCHED_GRADES = frozenset({"mapped", "probable"})

CRT_FOLDS = {
    "_ftol": "ftol", "_ftol2": "ftol", "_ftol2_sse": "ftol", "__ftol": "ftol",
    "__ftol2": "ftol", "__ftol2_sse": "ftol", "ftol2": "ftol", "ftol2_sse": "ftol",
    "_chkstk": "chkstk", "__chkstk": "chkstk", "_alloca_probe": "chkstk",
    "__alloca_probe": "chkstk", "_alloca_probe_16": "chkstk", "__alloca_probe_16": "chkstk",
    "alloca_probe": "chkstk", "alloca_probe_16": "chkstk",
    "operator_new": "operator new", "operator_delete": "operator delete",
    "_time32": "time", "_time64": "time", "time32": "time", "time64": "time",
    "_localtime64": "localtime", "localtime64": "localtime", "_localtime32": "localtime",
    "localtime32": "localtime",
}
# Build instrumentation the candidate has and the oracle does not (/GS; the UCRT's stdio-stream
# accessor, where the 2003 static CRT addresses _iob directly). Dropped from both sides.
DROPPED_CALLS = frozenset({"_security_check_cookie", "_acrt_iob_func"})
DROPPED_CANDIDATE_DATA = frozenset({"___security_cookie"})

# Oracle rows with a known identity that neither map nor inventory label carries.
ORACLE_KNOWN = {
    0x00001000: "`vector constructor iterator'",   # phase3-narrow-phase.md, phys_fn_000001
    0x00001030: "`vector destructor iterator'",    # phase3-narrow-phase.md, phys_fn_000002
    0x000f7a3c: "ftol",                             # phys_fn_005825: the fistp/fild _ftol2 body
    0x000f4dc0: "ftol",                             # phys_fn_005719: _ftol2_sse (stmxcsr/fnstcw
                                                    # check, then 0x000fc7f1); qh_nextfurthest
    0x000f50a0: "strstr",                           # phys_fn_005732: qh_init_qhull_command's
                                                    # strstr(qhull_command, ".EXE"/".exe")
}

# The NovodeX host seams. The oracle reaches the host inline (a virtual call through a global
# or through an allocator getter); the candidate's overlays call one named wrapper. Each wrapper
# call counts as the oracle call tokens it stands for ("@rva" = the oracle row at that RVA).
# Sources: External/qhull/novodex/QhullNovodeXHost.h, External/opcode/novodex/OpcodeNovodeXHost.h.
HOST_SEAMS = {
    "qhNovodeXOffBegin": ["icall[host]+0x0"],
    "qhNovodeXPoint3": ["icall[host]+0x4"],
    "qhNovodeXFacet3Vertex": ["icall[host]+0x8"],
    "qhNovodeXSize": ["icall[host]+0xc"],
    "qhNovodeXFprintf": ["icall[host]+0x10"],
    "qhNovodeXNarrowHull": ["icall[host]+0x1c"],
    "qhNovodeXMalloc": ["icall[host]+0x14"],
    "qhNovodeXFree": ["icall[host]+0x18"],
    "qhNovodeXErrexit": ["icall[host]+0x20"],
    "opcNovodeXAlloc": ["@0x000b4000", "icall[getter]+0x0"],
    "opcNovodeXFree": ["@0x000b4000", "icall[getter]+0xc"],
    "opcNovodeXSetIceError": ["@0x000539b0"],
}
# The qhull host-object global (.data:0x00125080). Every inline seam loads it, and the load
# count does not track the call count (one load can feed two calls), so it is not compared.
ORACLE_HOST_GLOBALS = frozenset({0x00125080})

# Float constants whose presence depends on how a compiler materialises a comparison or a
# store (fldz/fld1, an integer store of the bit pattern, a memory constant); shape only.
TRIVIAL_FLOATS = frozenset({"0.0", "-0.0", "1.0"})
# Block moves and fills, which either compiler may emit as a call, a `rep` string instruction
# (recorded as the call it stands for) or unrolled moves; a difference is shape only.
MEMORY_CALLS = frozenset({"memset", "memcpy", "memmove"})

# x87 instructions that materialise a constant without touching memory.
X87_CONSTANTS = {"fldz": 0.0, "fld1": 1.0, "fldpi": 3.141592653589793,
                 "fldl2e": 1.4426950408889634, "fldl2t": 3.321928094887362,
                 "fldlg2": 0.3010299956639812, "fldln2": 0.6931471805599453}

STOP_MNEMONICS = frozenset({"ret", "retf", "int3", "hlt", "ud2"})
FRAME_REGISTERS = frozenset({"esp", "ebp"})
SCALAR_DTOR = "`scalar deleting destructor'"
VECTOR_DTOR = "`vector deleting destructor'"

CLASSES = ("MATCH", "SHAPE", "REVIEW", "DIFF", "MAPCHECK", "MISSING", "AMBIGUOUS")


# --------------------------------------------------------------------------- images

@dataclass
class Section:
    name: str
    rva: int
    data: bytes
    executable: bool
    writable: bool

    @property
    def end(self):
        return self.rva + len(self.data)


class Image:
    """What the matcher needs of a PE: bytes by RVA, relocations and the import address table."""

    def __init__(self, base, sections, relocations, imports):
        self.base = base
        self.sections = sorted(sections, key=lambda s: s.rva)
        self.relocations = set(relocations)
        self.imports = dict(imports)   # IAT slot rva -> imported name
        self.host_globals = frozenset()

    @classmethod
    def from_pe(cls, path):
        import pefile
        pe = pefile.PE(str(path), fast_load=True)
        pe.parse_data_directories(directories=[
            pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_IMPORT"],
            pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_BASERELOC"]])
        sections = []
        for s in pe.sections:
            data = s.get_data()
            size = max(s.Misc_VirtualSize, len(data))
            data = data.ljust(size, b"\0")
            sections.append(Section(s.Name.rstrip(b"\0").decode("ascii", "replace"),
                                    s.VirtualAddress, bytes(data),
                                    bool(s.Characteristics & 0x20000000),
                                    bool(s.Characteristics & 0x80000000)))
        relocations = []
        for block in getattr(pe, "DIRECTORY_ENTRY_BASERELOC", []):
            relocations.extend(e.rva for e in block.entries if e.type == 3)
        imports = {}
        for entry in getattr(pe, "DIRECTORY_ENTRY_IMPORT", []):
            for imp in entry.imports:
                name = imp.name.decode("ascii") if imp.name else f"ord{imp.ordinal}"
                imports[imp.address - pe.OPTIONAL_HEADER.ImageBase] = name
        return cls(pe.OPTIONAL_HEADER.ImageBase, sections, relocations, imports)

    def section_of(self, rva):
        for s in self.sections:
            if s.rva <= rva < s.end:
                return s
        return None

    def read(self, rva, size):
        s = self.section_of(rva)
        if s is None or rva + size > s.end:
            return None
        return s.data[rva - s.rva:rva - s.rva + size]

    def u32(self, rva):
        raw = self.read(rva, 4)
        return None if raw is None else struct.unpack("<I", raw)[0]

    def string_at(self, rva, limit=512):
        """The NUL-terminated printable string at rva, or None."""
        s = self.section_of(rva)
        if s is None or s.executable or s.writable:
            return None
        raw = s.data[rva - s.rva:rva - s.rva + limit]
        end = raw.find(b"\0")
        if end <= 0:
            return None
        text = raw[:end]
        if all(0x20 <= b < 0x7f or b in (9, 10, 13) for b in text):
            return text.decode("ascii")
        return None


# --------------------------------------------------------------------------- demangling

BUILTIN_CODES = {
    "C": "signed char", "D": "char", "E": "unsigned char", "F": "short",
    "G": "unsigned short", "H": "int", "I": "unsigned int", "J": "long",
    "K": "unsigned long", "M": "float", "N": "double", "O": "long double", "X": "void",
}
BUILTIN_EXTENDED = {"_N": "bool", "_J": "__int64", "_K": "unsigned __int64", "_W": "wchar_t"}
SOURCE_TYPEDEFS = {
    "udword": "unsigned int", "dword": "unsigned int", "sdword": "int", "uword": "unsigned short",
    "sword": "short", "ubyte": "unsigned char", "sbyte": "signed char", "BOOL": "int",
    "unsigned": "unsigned int", "size_t": "unsigned int", "uint": "unsigned int",
}
SPECIAL_NAMES = {
    "0": None, "1": None, "2": "operator new", "3": "operator delete", "4": "operator=",
    "5": "operator>>", "6": "operator<<", "7": "operator!", "8": "operator==",
    "9": "operator!=", "A": "operator[]", "B": "operator cast", "C": "operator->",
    "D": "operator*", "E": "operator++", "F": "operator--", "G": "operator-",
    "H": "operator+", "I": "operator&", "J": "operator->*", "K": "operator/",
    "L": "operator%", "M": "operator<", "N": "operator<=", "O": "operator>",
    "P": "operator>=", "Q": "operator,", "R": "operator()", "S": "operator~",
    "T": "operator^", "U": "operator|", "V": "operator&&", "W": "operator||",
    "X": "operator*=", "Y": "operator+=", "Z": "operator-=", "_0": "operator/=",
    "_1": "operator%=", "_2": "operator>>=", "_3": "operator<<=", "_4": "operator&=",
    "_5": "operator|=", "_6": "operator^=",
    "_G": SCALAR_DTOR, "_E": VECTOR_DTOR, "_7": "`vftable'", "_U": "operator new[]",
    "_V": "operator delete[]", "_H": "`vector constructor iterator'",
    "_L": "`vector constructor iterator'", "_I": "`vector destructor iterator'",
    "_M": "`vector destructor iterator'",
}


class _Unsupported(Exception):
    pass


class _Reader:
    def __init__(self, text, pos=0):
        self.text = text
        self.pos = pos
        self.names = []   # name back-reference table
        self.args = []    # argument back-reference table

    def peek(self, n=1):
        return self.text[self.pos:self.pos + n]

    def take(self, n=1):
        out = self.text[self.pos:self.pos + n]
        if len(out) < n:
            raise _Unsupported("truncated")
        self.pos += n
        return out

    def fragment(self):
        """One identifier terminated by '@', registering it for back-references."""
        end = self.text.find("@", self.pos)
        if end < 0:
            raise _Unsupported("unterminated name")
        name = self.text[self.pos:end]
        self.pos = end + 1
        if len(self.names) < 10:
            self.names.append(name)
        return name

    def scope(self):
        """Scope fragments up to the terminating '@'; innermost first."""
        parts = []
        while True:
            c = self.peek()
            if c == "":
                raise _Unsupported("unterminated scope")
            if c == "@":
                self.pos += 1
                return parts
            if c.isdigit():
                self.pos += 1
                index = int(c)
                if index >= len(self.names):
                    raise _Unsupported("bad name back-reference")
                parts.append(self.names[index])
            elif c == "?":
                if self.peek(2) == "?$":
                    raise _Unsupported("template")
                # ?N?  -> a local scope inside the nested mangled function name that follows.
                m = re.match(r"\?(\d+|[A-P]+@)\?", self.text[self.pos:])
                if not m:
                    raise _Unsupported("unknown scope form")
                self.pos += m.end()
                if self.peek() == "?":
                    self.pos += 1      # the nested function's own mangled name
                inner = _Reader(self.text, self.pos)
                name = inner.special_or_fragment()
                outer = inner.scope()
                parts.extend([name] + outer)
                return parts   # the rest is the nested function's type: nothing more to name
            else:
                parts.append(self.fragment())

    def special_or_fragment(self):
        if self.peek() == "?":
            self.pos += 1
            code = self.take(1)
            if code == "_":
                code += self.take(1)
            return ("special", code)
        return self.fragment()

    def type_(self):
        c = self.take(1)
        if c in BUILTIN_CODES:
            return BUILTIN_CODES[c]
        if c == "_":
            code = "_" + self.take(1)
            if code in BUILTIN_EXTENDED:
                return BUILTIN_EXTENDED[code]
            raise _Unsupported(code)
        if c in "PQAB":
            cv = self.take(1)
            if cv == "6":
                self.take(1)                       # calling convention
                self.type_()                       # return type
                self.arg_list()                    # arguments, '@'-terminated
                if self.peek() == "Z":
                    self.take(1)
                return "fnptr"
            if cv not in "ABCD":
                raise _Unsupported("cv " + cv)
            inner = self.type_()
            const = "const " if cv in "BD" else ""
            return f"{const}{inner}{'*' if c in 'PQ' else '&'}"
        if c == "?":
            cv = self.take(1)
            inner = self.type_()
            return f"const {inner}" if cv in "BD" else inner
        if c in "VUT":
            parts = self.scope()
            return parts[0] if parts else "?"
        if c == "W":
            self.take(1)
            parts = self.scope()
            return parts[0] if parts else "?"
        raise _Unsupported("type " + c)

    def arg_list(self):
        args = []
        while True:
            c = self.peek()
            if c in ("@", "Z", ""):
                if c == "@":
                    self.pos += 1
                return args
            if c.isdigit():
                self.pos += 1
                index = int(c)
                if index >= len(self.args):
                    raise _Unsupported("bad arg back-reference")
                args.append(self.args[index])
                continue
            if c == "X" and not args:
                self.pos += 1
                return []
            start = self.pos
            t = self.type_()
            if self.pos - start > 1 and len(self.args) < 10:
                self.args.append(t)
            args.append(t)


@dataclass
class Demangled:
    components: list          # outermost scope first
    params: list = None       # normalised parameter types, or None when unknown


def demangle(symbol):
    """Qualified name and parameter types of a map symbol (C or MSVC C++)."""
    if not symbol.startswith("?"):
        name = symbol
        if name.startswith("@"):                  # fastcall
            name = name[1:]
        name = re.sub(r"@\d+$", "", name)          # stdcall / fastcall suffix
        if name.startswith("_"):
            name = name[1:]
        return Demangled([name], None)
    r = _Reader(symbol, 1)
    try:
        head = r.special_or_fragment()
        scope = r.scope()
    except _Unsupported:
        return Demangled([symbol], None)
    components = list(reversed(scope))
    if isinstance(head, tuple):
        code = head[1]
        if code == "0":
            name = components[-1] if components else "?"
        elif code == "1":
            name = "~" + (components[-1] if components else "?")
        elif code in SPECIAL_NAMES and SPECIAL_NAMES[code]:
            name = SPECIAL_NAMES[code]
        else:
            name = f"operator?{code}"
    else:
        name = head
    components.append(name)
    params = None
    try:
        kind = r.take(1)
        if kind not in "ABCDEFGHIJKLMNOPQRSTUVWXYZ":
            raise _Unsupported("not a function")
        if kind not in "CDKLSTYZ":
            r.take(1)              # cv-qualifier of `this`
        r.take(1)                  # calling convention
        if r.peek() == "@":
            r.take(1)              # constructor/destructor: no return type
        else:
            r.type_()
        params = r.arg_list()
    except _Unsupported:
        params = None
    return Demangled(components, params)


def normalise_type(text):
    """A source parameter declaration reduced to the demangler's spelling."""
    text = text.strip()
    if not text or text == "...":
        return None
    const = bool(re.search(r"\bconst\b", text))
    text = re.sub(r"\bconst\b", " ", text)
    suffix = ""
    m = re.search(r"([*&]+)\s*\w*\s*$", text)
    if m:
        suffix = m.group(1)
        text = text[:m.start()]
    words = text.split()
    if not words:
        return None
    if len(words) >= 2 and words[0] in ("unsigned", "signed"):
        base = " ".join(words[:2])
        if base == "unsigned":
            base = "unsigned int"
    else:
        base = words[0]
    base = SOURCE_TYPEDEFS.get(base, base)
    base = base.split("::")[-1]
    if base.endswith("Callback") and not suffix:
        return "fnptr"
    if not suffix:
        return base
    return f"{'const ' if const else ''}{base}{suffix}"


def split_params(text):
    depth, parts, cur = 0, [], ""
    for ch in text:
        if ch == "," and depth == 0:
            parts.append(cur)
            cur = ""
            continue
        depth += ch in "(<"
        depth -= ch in ")>"
        cur += ch
    if cur.strip():
        parts.append(cur)
    return parts


@dataclass
class SourceName:
    components: list
    params: list = None          # None when the source gives none or they are unusable
    partial: bool = False        # the source elides trailing parameters with "..."


def parse_source_function(text):
    """Alternatives (SourceName) and bracketed tags of a map `source_function`."""
    tags = re.findall(r"\[([^\]]*)\]", text)
    body = re.sub(r"\[[^\]]*\]", "", text)
    body = body.replace("(file-static)", "")
    names = []
    for alt in body.split(" / "):
        alt = alt.strip()
        if not alt:
            continue
        params, partial = None, False
        paren = alt.find("(")
        if paren >= 0 and "`" not in alt[:paren]:
            close = alt.rfind(")")
            inner = alt[paren + 1:close if close > paren else len(alt)]
            head = alt[:paren].strip()
            if inner.strip() in ("", "void"):
                params = []
            else:
                listed = split_params(inner)
                if listed and listed[-1].strip() == "...":
                    listed, partial = listed[:-1], True
                types = [normalise_type(p) for p in listed]
                params = None if any(t is None for t in types) or not types else types
        else:
            head = alt
        if "`" in head:
            pre, tick = head.split("`", 1)
            parts = [p for p in pre.split("::") if p]
            components = [p.split()[-1] for p in parts] + ["`" + tick.strip()]
        else:
            head = head.split()[-1] if head.split() else head
            components = [p for p in head.split("::") if p]
        if not components:
            continue
        if len(components) == 1 and components[0].startswith("`") and names:
            components = names[0].components[:-1] + components
        names.append(SourceName(components, params, partial))
    return names, tags


def short_key(components):
    """Identity used to compare call targets: the last two name components."""
    tail = components[-2:] if len(components) >= 2 else components
    key = "::".join(tail)
    return CRT_FOLDS.get(key, key)


# --------------------------------------------------------------------------- linker map

@dataclass
class Symbol:
    name: str
    rva: int
    obj: str
    is_function: bool
    demangled: Demangled = None
    end: int = 0
    ident: str = None             # set by SymbolIndex when the short key is overloaded

    @property
    def key(self):
        return self.ident or short_key(self.demangled.components)


MAP_LINE = re.compile(r"^\s*([0-9a-fA-F]{4}):([0-9a-fA-F]{8})\s+(\S+)\s+([0-9a-fA-F]{8})\s+(.*)$")


def parse_map(text, base, image=None):
    """Symbols of an MSVC linker map (public and static), with function extents."""
    symbols = []
    in_symbols = False
    for line in text.splitlines():
        if "Publics by Value" in line or "Static symbols" in line:
            in_symbols = True
            continue
        if not in_symbols:
            continue
        m = MAP_LINE.match(line)
        if not m:
            continue
        address = int(m.group(4), 16)
        if address < base:
            continue
        flags = m.group(5).split()
        is_function = "f" in flags
        obj = flags[-1] if flags and flags[-1] not in ("f", "i") else ""
        symbols.append(Symbol(m.group(3), address - base, obj, is_function,
                              demangle(m.group(3))))
    symbols.sort(key=lambda s: s.rva)
    starts = sorted({s.rva for s in symbols})
    for s in symbols:
        i = bisect.bisect_right(starts, s.rva)
        nxt = starts[i] if i < len(starts) else s.rva + 0x10000
        if image is not None:
            section = image.section_of(s.rva)
            if section is not None:
                nxt = min(nxt, section.end)
        s.end = nxt
    return symbols


class SymbolIndex:
    def __init__(self, symbols):
        self.symbols = symbols
        self.by_last = defaultdict(list)
        self.by_name = {s.name: s for s in symbols}
        for s in symbols:
            if s.is_function:
                self.by_last[s.demangled.components[-1]].append(s)
        self.sorted = sorted(symbols, key=lambda s: s.rva)
        self.rvas = [s.rva for s in self.sorted]
        self.function_at = {}
        for s in symbols:
            if s.is_function:
                self.function_at.setdefault(s.rva, s)
        # Overloads share a short key; give each its parameter list so that a call to one
        # overload is not taken for a call to another.
        by_key = defaultdict(set)
        for s in symbols:
            if s.is_function:
                by_key[short_key(s.demangled.components)].add(s.name)
        self.overloaded = {k for k, names in by_key.items() if len(names) > 1}
        for s in symbols:
            k = short_key(s.demangled.components)
            # C functions and the global operators cannot be told apart by what they do.
            if s.is_function and k in self.overloaded and len(s.demangled.components) >= 2:
                params = s.demangled.params
                s.ident = (f"{k}({','.join(params)})" if params is not None
                           else f"{k}#{s.name}")

    def locate(self, token):
        """(name, rva, end, offset) of a candidate data token "symbol[+0xoff]"."""
        m = re.match(r"(.*?)(?:\+0x([0-9a-f]+))?$", token)
        s = self.by_name.get(m.group(1))
        if s is None:
            return None
        return s.name, s.rva, s.end, int(m.group(2) or "0", 16)

    def containing(self, rva):
        i = bisect.bisect_right(self.rvas, rva) - 1
        if i < 0:
            return None
        s = self.function_at.get(self.rvas[i], self.sorted[i])
        return s if rva < max(s.end, s.rva + 1) else None

    def lookup(self, source_names, source_file=""):
        """(symbol, None) on a unique match, (None, [options]) when ambiguous, (None, [])."""
        # A row that names a deleting destructor among its alternatives is that destructor
        # (the other alternative is the body inlined into it).
        ordered = sorted(source_names,
                         key=lambda n: n.components[-1] not in (SCALAR_DTOR, VECTOR_DTOR))
        for name in ordered:
            options = self._suffix_matches(name.components)
            if not options and name.components[-1] == SCALAR_DTOR:
                options = self._suffix_matches(name.components[:-1] + [VECTOR_DTOR])
            if not options:
                continue
            if len(options) > 1 and name.params is not None:
                n = len(name.params)
                exact = [s for s in options if s.demangled.params is not None
                         and (s.demangled.params[:n] == name.params if name.partial
                              else s.demangled.params == name.params)]
                if exact:
                    options = exact
            if len(options) > 1 and source_file:
                stem = Path(source_file.split()[0]).stem.lower()
                same = [s for s in options if Path(s.obj.split(":")[-1]).stem.lower() == stem]
                if same:
                    options = same
            if len(options) == 1:
                return options[0], None
            return None, options
        return None, []

    def _suffix_matches(self, components):
        out = []
        n = len(components)
        for s in self.by_last.get(components[-1], []):
            c = s.demangled.components
            if len(c) >= n and c[-n:] == components:
                out.append(s)
        return out


# --------------------------------------------------------------------------- disassembly

@dataclass
class Features:
    calls: list = field(default_factory=list)
    strings: Counter = field(default_factory=Counter)
    floats: Counter = field(default_factory=Counter)
    float_widths: Counter = field(default_factory=Counter)
    data: list = field(default_factory=list)
    imms: Counter = field(default_factory=Counter)
    n_insn: int = 0
    n_jcc: int = 0
    x87: int = 0
    sse: int = 0
    switch: int = 0
    icall_reg: int = 0
    fchs: int = 0
    frame: bool = False
    fields: set = field(default_factory=set)          # (base class, byte offset)
    accesses: set = field(default_factory=set)        # (base class, offset, size)
    logic: Counter = field(default_factory=Counter)
    x87ops: set = field(default_factory=set)
    jccpairs: Counter = field(default_factory=Counter)   # report only
    stores: Counter = field(default_factory=Counter)     # report only

    def merge(self, other):
        self.calls.extend(other.calls)
        self.strings.update(other.strings)
        self.floats.update(other.floats)
        self.float_widths.update(other.float_widths)
        self.data.extend(other.data)
        self.imms.update(other.imms)
        self.n_insn += other.n_insn
        self.n_jcc += other.n_jcc
        self.x87 += other.x87
        self.sse += other.sse
        self.switch += other.switch
        self.icall_reg += other.icall_reg
        self.fchs += other.fchs
        self.frame = self.frame or other.frame
        self.fields |= other.fields
        self.accesses |= other.accesses
        self.logic.update(other.logic)
        self.x87ops |= other.x87ops
        self.jccpairs.update(other.jccpairs)
        self.stores.update(other.stores)


_CS = None


def _disassembler():
    global _CS
    if _CS is None:
        _CS = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        _CS.detail = True
    return _CS


def _decode(image, rva):
    section = image.section_of(rva)
    if section is None:
        return None
    raw = section.data[rva - section.rva:rva - section.rva + 16]
    for insn in _disassembler().disasm(raw, rva, 1):
        return insn
    return None


def _relocated_values(image, insn):
    """Relocated 32-bit values inside the instruction, as target RVAs."""
    out = []
    for r in range(insn.address, insn.address + insn.size - 3):
        if r in image.relocations:
            value = image.u32(r)
            if value is not None:
                out.append(value - image.base)
    return out


def _is_float_insn(insn):
    """"x87" for an x87 instruction, "sse" for one with an xmm register operand, else None."""
    m = insn.mnemonic
    if m.startswith("f") and m != "fwait":
        return "x87"
    if any(op.type == x86.X86_OP_REG and insn.reg_name(op.reg).startswith("xmm")
           for op in insn.operands):
        return "sse"
    return None


def extract(image, entries, extent, resolve_code, resolve_data):
    """Features of the code reachable from `entries` inside `extent` [(start, end), ...].

    resolve_code(target_rva, is_call) -> identity string, or None when the target is internal.
    resolve_data(target_rva) -> data token for a non-constant reference.
    """
    def inside(rva):
        return any(lo <= rva < hi for lo, hi in extent)

    feats = Features()
    seen = {}
    work = list(entries)
    while work:
        rva = work.pop()
        while inside(rva) and rva not in seen:
            insn = _decode(image, rva)
            if insn is None:
                break
            seen[rva] = insn
            m = insn.mnemonic
            nxt = rva + insn.size
            ops = insn.operands
            if m in STOP_MNEMONICS:
                break
            if insn.group(x86.X86_GRP_JUMP):
                target = ops[0].imm if ops and ops[0].type == x86.X86_OP_IMM else None
                if m == "jmp":
                    if target is not None:
                        if inside(target):
                            work.append(target)
                        else:
                            ident = resolve_code(target, True)
                            if ident:
                                seen[rva] = (insn, "tail", ident)
                        break
                    # indirect jump: a switch table, or a jump through memory
                    op = ops[0]
                    if op.type == x86.X86_OP_MEM and op.mem.index != 0 and op.mem.scale == 4:
                        table = op.mem.disp - image.base
                        entry = table
                        while entry in image.relocations:
                            t = image.u32(entry) - image.base
                            if not inside(t):
                                break
                            work.append(t)
                            entry += 4
                    break
                if target is not None and inside(target):
                    work.append(target)
                rva = nxt
                continue
            rva = nxt
    # Features, in address order so the call sequence is stable.
    ordered, tails = [], {}
    for rva in sorted(seen):
        item = seen[rva]
        if isinstance(item, tuple):
            ordered.append(item[0])
            tails[rva] = item[2]
        else:
            ordered.append(item)
    ordered = Ordered(ordered)
    feats.frame = any(i.mnemonic == "mov" and i.op_str == "ebp, esp" for i in ordered)
    for position, insn in enumerate(ordered):
        _features_of(image, insn, feats, resolve_code, resolve_data,
                     tails.get(insn.address), inside, (ordered, position))
    return feats


X87_OPS = {}
for _cls, _names in (("add", "fadd faddp fiadd"), ("sub", "fsub fsubp fsubr fsubrp fisub fisubr"),
                     ("mul", "fmul fmulp fimul"), ("div", "fdiv fdivp fdivr fdivrp fidiv fidivr"),
                     ("sqrt", "fsqrt"), ("abs", "fabs")):
    for _name in _names.split():
        X87_OPS[_name] = _cls

LOGIC_MNEMONICS = frozenset({"cmp", "test", "and", "or", "xor", "shl", "shr", "sar", "sal"})
BIT_MNEMONICS = {"test": "bit", "and": "clear", "or": "set", "xor": "flip"}
X87_STATUS_REGISTERS = frozenset({"ah", "ax"})


def _signed(value, size):
    bits = 8 * size
    value &= (1 << bits) - 1
    return value - (1 << bits) if value >> (bits - 1) else value


def _features_of(image, insn, feats, resolve_code, resolve_data, tail, inside, context=None):
    m = insn.mnemonic
    ops = insn.operands
    feats.n_insn += 1
    if insn.group(x86.X86_GRP_JUMP) and m not in ("jmp",):
        feats.n_jcc += 1
    fp = _is_float_insn(insn)
    if fp == "x87":
        feats.x87 += 1
    elif fp == "sse":
        feats.sse += 1
    if m == "fchs":
        feats.fchs += 1
    if m in X87_OPS:
        feats.x87ops.add(X87_OPS[m])
    if context is not None:
        _report_pairs(insn, feats, context)
    if m in X87_CONSTANTS:
        feats.floats[repr(X87_CONSTANTS[m])] += 1
    relocated = set(_relocated_values(image, insn))
    if tail is not None:
        _add_call(feats, tail)
        return
    if m == "call":
        op = ops[0]
        if op.type == x86.X86_OP_IMM:
            _add_call(feats, resolve_code(op.imm, False))
        elif op.type == x86.X86_OP_MEM:
            slot = op.mem.disp - image.base
            if op.mem.base == 0 and op.mem.index == 0 and slot in image.imports:
                _add_call(feats, import_key(image.imports[slot]))
            else:
                tag = _icall_tag(image, insn, op, context)
                feats.calls.append(f"icall[{tag}]+0x{op.mem.disp & 0xffffffff:x}")
        else:
            feats.icall_reg += 1       # through a register: often a cached import address
        return
    string_op = m.startswith(("rep ", "repe ", "repne "))
    if m.startswith("rep stos"):
        _add_call(feats, "memset")     # the inline form of the CRT call
    elif m.startswith("rep movs"):
        _add_call(feats, "memcpy")
    elif m.startswith("repe cmps"):
        _add_call(feats, "memcmp")
    first_reg = (ops and ops[0].type == x86.X86_OP_REG
                 and insn.reg_name(ops[0].reg) in FRAME_REGISTERS)
    logic = m in LOGIC_MNEMONICS
    status = (logic and ops and ops[0].type == x86.X86_OP_REG
              and insn.reg_name(ops[0].reg) in X87_STATUS_REGISTERS)
    imm_ops = [op for op in ops if op.type == x86.X86_OP_IMM]
    for op in ops:
        if op.type == x86.X86_OP_IMM:
            value = op.imm & 0xffffffff
            if (value - image.base) in relocated:
                _data_ref(image, insn, value - image.base, None, feats, resolve_code,
                          resolve_data, inside)
            elif logic:
                bit_form = ops[0].type == x86.X86_OP_MEM and m in BIT_MNEMONICS
                if first_reg or status or not value or bit_form:
                    continue
                loaded = (_loaded_field(insn, ops, context)
                          if m in ("test", "and", "or", "xor") else None)
                kind = None
                if loaded is not None:
                    rmw = _stored_back(insn, ops, context, loaded)
                    if m in ("test", "and") and not rmw:
                        kind = "bit"          # a mask on a field just loaded: a test
                    elif rmw:
                        kind = BIT_MNEMONICS[m]   # load, modify, store back: a memory op
                if kind is not None:
                    # the same bits a memory-form instruction on the field would name
                    cls, offset, size, _, _ = loaded
                    mask = op.imm if kind != "clear" else ~op.imm
                    tokens, _ = _bit_tokens(kind, cls, offset, size, mask)
                    feats.logic.update(tokens)
                else:
                    feats.logic[str(_signed(op.imm, ops[0].size or 4))] += 1
            elif not first_reg and not insn.group(x86.X86_GRP_JUMP):
                feats.imms[value] += 1
        elif op.type == x86.X86_OP_MEM:
            disp = op.mem.disp & 0xffffffff
            if (disp - image.base) in relocated:
                _data_ref(image, insn, disp - image.base, op, feats, resolve_code,
                          resolve_data, inside)
                continue
            if m == "lea" or string_op:
                continue
            base = insn.reg_name(op.mem.base) if op.mem.base else None
            if base is None or base == "esp" or (base == "ebp" and feats.frame):
                continue
            offset = _signed(op.mem.disp, 4)
            size = op.size or 4
            if offset < 0 or op.mem.index:
                # a strength-reduced pointer walk or an indexed element, not a named field
                continue
            cls = _base_class(context[0], context[1], base) if context else "other"
            if m in BIT_MNEMONICS and imm_ops:
                mask = imm_ops[0].imm if m != "and" else ~imm_ops[0].imm
                tokens, bits = _bit_tokens(BIT_MNEMONICS[m], cls, offset, size, mask)
                feats.logic.update(tokens)
                touched = {offset + b // 8 for b in bits}
                feats.fields.update((cls, b) for b in touched)
                feats.accesses.update((cls, b, 1) for b in touched)
            else:
                feats.fields.update((cls, b) for b in range(offset, offset + size))
                feats.accesses.add((cls, offset, size))


def _report_pairs(insn, feats, context):
    """Report-only features: (compare immediate, jcc) pairs and (field, stored value) pairs."""
    ordered, position = context
    m = insn.mnemonic
    ops = insn.operands
    if (m == "cmp" and len(ops) == 2 and ops[1].type == x86.X86_OP_IMM
            and position + 1 < len(ordered)):
        nxt = ordered[position + 1]
        if nxt.group(x86.X86_GRP_JUMP) and nxt.mnemonic != "jmp":
            feats.jccpairs[f"{_signed(ops[1].imm, ops[0].size or 4)}:{nxt.mnemonic}"] += 1
    target = None
    if m == "mov" and len(ops) == 2 and ops[0].type == x86.X86_OP_MEM \
            and ops[1].type == x86.X86_OP_IMM:
        target, value = ops[0], _store_value(ops[1].imm, ops[0].size or 4)
    elif m in ("fstp", "fst") and ops and ops[0].type == x86.X86_OP_MEM and position:
        prior = ordered[position - 1].mnemonic
        if prior in ("fldz", "fld1"):
            target, value = ops[0], "0" if prior == "fldz" else "f:1.0"
    if target is None or not target.mem.base or target.mem.index:
        return
    base = insn.reg_name(target.mem.base)
    offset = _signed(target.mem.disp, 4)
    if base == "esp" or (base == "ebp" and feats.frame) or offset < 0:
        return
    cls = _base_class(ordered, position, base)
    feats.stores[f"{cls}:{_hex(offset)}={value}"] += 1


def _loaded_field(insn, ops, context, window=6):
    """(base class, offset, size, base register, load index) when the register operand of
    insn was last written by a plain load from a non-stack structure field, else None."""
    if context is None or ops[0].type != x86.X86_OP_REG:
        return None
    ordered, position = context
    reg = insn.reg_name(ops[0].reg)
    w = _last_writer(ordered, position, reg, window)
    if w is None:
        return None
    writer = ordered[w]
    wops = writer.operands
    if (writer.mnemonic not in ("mov", "movzx") or len(wops) != 2
            or wops[1].type != x86.X86_OP_MEM or not wops[1].mem.base or wops[1].mem.index):
        return None
    base = writer.reg_name(wops[1].mem.base)
    if base in FRAME_REGISTERS:
        return None
    offset = _signed(wops[1].mem.disp, 4)
    if offset < 0:
        return None
    return (_base_class(ordered, w, base), offset, wops[1].size or 4, base, w)


def _stored_back(insn, ops, context, loaded, window=4):
    """Whether the register of a read-modify-write goes back to the field it came from."""
    ordered, position = context
    reg = GPR_PARENT.get(insn.reg_name(ops[0].reg))
    _, offset, _, base, _ = loaded
    for later in ordered[position + 1:position + 1 + window]:
        lops = later.operands
        if (later.mnemonic == "mov" and len(lops) == 2 and lops[0].type == x86.X86_OP_MEM
                and lops[1].type == x86.X86_OP_REG
                and GPR_PARENT.get(later.reg_name(lops[1].reg)) == reg
                and lops[0].mem.base and later.reg_name(lops[0].mem.base) == base
                and not lops[0].mem.index and _signed(lops[0].mem.disp, 4) == offset):
            return True
        if reg in _written_gprs(later):
            return False
    return False


def _bit_tokens(kind, cls, offset, size, mask):
    """One token per contiguous run of the mask's bits, at absolute bit addresses."""
    mask &= (1 << (8 * size)) - 1
    bits = [b for b in range(8 * size) if mask >> b & 1]
    out = []
    for lo, hi in _runs(bits):
        lo, hi = offset * 8 + lo, offset * 8 + hi
        out.append(f"{cls}:{kind}@{lo}" + (f"..{hi}" if hi > lo else ""))
    return out, bits


def _store_value(value, size):
    """A stored immediate, normalised: 0 whatever its type, a float bit pattern as the float."""
    value &= (1 << (8 * size)) - 1
    if value == 0:
        return "0"
    if size == 4 and value > 0xffff and (value >> 23) & 0xff not in (0, 0xff):
        f = struct.unpack("<f", struct.pack("<I", value))[0]
        if 1e-6 <= abs(f) <= 1e9:
            return f"f:{f!r}"
    return str(_signed(value, size))


def _runs(values):
    """Contiguous runs of sorted integers, as (first, last) pairs."""
    runs = []
    for v in values:
        if runs and v == runs[-1][1] + 1:
            runs[-1][1] = v
        else:
            runs.append([v, v])
    return [tuple(r) for r in runs]


GPR_PARENT = {}
for _parent, _subs in (("eax", "ax al ah"), ("ebx", "bx bl bh"), ("ecx", "cx cl ch"),
                       ("edx", "dx dl dh"), ("esi", "si"), ("edi", "di"), ("ebp", "bp"),
                       ("esp", "sp")):
    GPR_PARENT[_parent] = _parent
    for _sub in _subs.split():
        GPR_PARENT[_sub] = _parent


def _written_gprs(insn):
    """The 32-bit general registers insn writes (calls clobber eax, ecx and edx)."""
    if insn.mnemonic == "call":
        return {"eax", "ecx", "edx"}
    try:
        _, written = insn.regs_access()
    except capstone.CsError:
        return set()
    return {GPR_PARENT[n] for n in (insn.reg_name(r) for r in written) if n in GPR_PARENT}


class Ordered(list):
    """Instructions in address order, with the last writer of every register before each."""

    def __init__(self, insns):
        super().__init__(insns)
        self.writers = []
        current = {}
        for i, insn in enumerate(self):
            self.writers.append(dict(current))
            for reg in _written_gprs(insn):
                current[reg] = i


def _last_writer(ordered, position, reg, window=16):
    """Index of the last instruction before position (in address order) that writes reg,
    within window instructions (None = no limit)."""
    reg = GPR_PARENT.get(reg, reg)
    if isinstance(ordered, Ordered):
        w = ordered.writers[position].get(reg)
        if w is None or (window is not None and position - w > window):
            return None
        return w
    low = -1 if window is None else max(-1, position - 1 - window)
    for i in range(position - 1, low, -1):
        if reg in _written_gprs(ordered[i]):
            return i
    return None


def _base_class(ordered, position, reg, depth=0):
    """What a base register holds at position, traced back through register copies.

    this     ecx as it came in (never written before): the thiscall object
    derived  a pointer loaded from memory, or an address computed from another register
    other    anything else (a stack argument, a returned pointer, an untraced value)
    """
    if ordered is None:
        return "other"
    w = _last_writer(ordered, position, reg, None)
    if w is None:
        return "this" if GPR_PARENT.get(reg) == "ecx" else "other"
    writer = ordered[w]
    wops = writer.operands
    if writer.mnemonic == "mov" and len(wops) == 2 and wops[1].type == x86.X86_OP_REG:
        if depth < 4:
            return _base_class(ordered, w, writer.reg_name(wops[1].reg), depth + 1)
        return "other"
    if writer.mnemonic in ("mov", "lea") and len(wops) == 2 and wops[1].type == x86.X86_OP_MEM:
        base = writer.reg_name(wops[1].mem.base) if wops[1].mem.base else None
        if base and base not in FRAME_REGISTERS:
            return "derived"
        if writer.mnemonic == "mov" and base == "ebp" and depth < 4:
            # a reload of a spilled register: follow the register that was stored there
            disp = wops[1].mem.disp
            for i in range(w - 1, -1, -1):
                sops = ordered[i].operands
                if (ordered[i].mnemonic == "mov" and len(sops) == 2
                        and sops[0].type == x86.X86_OP_MEM and sops[1].type == x86.X86_OP_REG
                        and sops[0].mem.base and ordered[i].reg_name(sops[0].mem.base) == "ebp"
                        and not sops[0].mem.index and sops[0].mem.disp == disp):
                    return _base_class(ordered, i, ordered[i].reg_name(sops[1].reg), depth + 1)
    return "other"


def _icall_tag(image, insn, op, context):
    """Where the base of a memory-indirect call came from.

    host    an ORACLE_HOST_GLOBALS global is read within the 12 instructions before the call
            (the qhull host passes its object explicitly, and the vtable register is often
            cached from further back)
    gobj    a vtable read from an object held in another global
    getter  a vtable read from an object a preceding call returned
    obj     a vtable read from any other object (this, a member, an argument)
    fptr    a function pointer read from a non-zero offset of a structure
    ret     a function pointer a preceding call returned in eax
    ?       not traced (the base was written too far back, or by something else)
    """
    if context is None or not op.mem.base:
        return "?"
    ordered, position = context
    hosts = getattr(image, "host_globals", ())
    if hosts:
        for prior in ordered[max(0, position - 12):position]:
            for pop in prior.operands:
                if (pop.type == x86.X86_OP_MEM and not pop.mem.base and not pop.mem.index
                        and (pop.mem.disp & 0xffffffff) - image.base in hosts):
                    return "host"
    base = insn.reg_name(op.mem.base)
    w = _last_writer(ordered, position, base)
    if w is None:
        return "?"
    writer = ordered[w]
    if writer.mnemonic == "call":
        return "ret" if base == "eax" else "?"
    wops = writer.operands
    if writer.mnemonic != "mov" or len(wops) != 2 or wops[1].type != x86.X86_OP_MEM:
        return "?"
    src = wops[1].mem
    if src.index or src.disp or not src.base:
        return "fptr"
    obj_reg = writer.reg_name(src.base)
    o = _last_writer(ordered, w, obj_reg)
    if o is None:
        return "obj"
    origin = ordered[o]
    if origin.mnemonic == "call":
        return "getter"
    oops = origin.operands
    if (origin.mnemonic == "mov" and len(oops) == 2 and oops[1].type == x86.X86_OP_MEM
            and not oops[1].mem.base and not oops[1].mem.index):
        target = (oops[1].mem.disp & 0xffffffff) - image.base
        return "host" if target in getattr(image, "host_globals", ()) else "gobj"
    return "obj"


def import_key(name):
    """Imports compare by their normalised name, so a UCRT import meets a static CRT row."""
    return short_key(demangle(name).components)


def _add_call(feats, ident):
    if ident and ident not in DROPPED_CALLS:
        feats.calls.append(ident)


def _data_ref(image, insn, target, mem_op, feats, resolve_code, resolve_data, inside):
    if target in image.imports:
        _add_call(feats, import_key(image.imports[target]))
        return
    section = image.section_of(target)
    if section is None:
        feats.data.append(f"unmapped:0x{target:08x}")
        return
    if section.executable:
        if mem_op is not None or inside(target):
            feats.switch += 1
        else:
            ident = resolve_code(target, False)
            feats.data.append("fnptr:" + (ident or "self"))
        return
    if mem_op is not None and _is_float_insn(insn) and not section.writable:
        width = mem_op.size
        raw = image.read(target, width)
        if raw is not None and width in (4, 8):
            value = struct.unpack("<f" if width == 4 else "<d", raw)[0]
            feats.floats[_float_key(value, raw)] += 1
            feats.float_widths[width] += 1
            return
    if mem_op is None:
        text = image.string_at(target)
        if text is not None:
            feats.strings[string_key(text)] += 1
            return
    token = resolve_data(target)
    if token is not None:
        feats.data.append(token)


SOURCE_PATH = re.compile(r"^(?:[A-Za-z]:)?[\\/].*[\\/]([^\\/]+\.(?:c|cpp|h))$", re.I)


def string_key(text):
    """Strings compare by content; a __FILE__ path compares by its file name only, because the
    two builds ran in different directories."""
    m = SOURCE_PATH.match(text)
    return f"file:{m.group(1)}" if m else "str:" + text


def _float_key(value, raw):
    if value != value:
        return "nan:" + raw.hex()
    return repr(float(value))


# --------------------------------------------------------------------------- matching

@dataclass
class MapRow:
    library: str
    map_file: str
    rva: int
    id: str
    size: int
    grade: str
    source_file: str
    source_line: str
    source_function: str
    names: list = None
    tags: list = None
    notes: str = ""
    symbol: Symbol = None
    options: list = None
    group: str = ""


def load_map_rows(library, map_dir=MAP_DIR):
    rows = []
    for name in LIBRARIES[library]:
        path = Path(map_dir) / name
        with path.open(encoding="utf-8", newline="") as handle:
            for r in csv.DictReader(handle):
                row = MapRow(library, name, int(r["rva"], 16), r["id"], int(r["size"]),
                             r["grade"], r["source_file"], r["source_line"],
                             r["source_function"])
                row.names, row.tags = parse_source_function(r["source_function"])
                row.notes = r.get("notes", "")
                rows.append(row)
    return rows


def resolve_rows(rows, index):
    for row in rows:
        if row.grade not in MATCHED_GRADES or not row.names:
            continue
        row.symbol, row.options = index.lookup(row.names, row.source_file)


def group_rows(rows):
    """Map rows grouped by their resolved candidate symbol."""
    groups = defaultdict(list)
    for row in rows:
        if row.grade not in MATCHED_GRADES:
            continue
        if row.symbol is not None:
            separate = any("instantiation" in t for t in row.tags)
            key = f"{row.symbol.name}#{row.rva:x}" if separate else row.symbol.name
        else:
            key = f"row:{row.rva:x}"
        groups[key].append(row)
        row.group = key
    return groups


class OracleResolver:
    """Oracle call-target identities: map rows, then inventory labels, then census ids."""

    def __init__(self, map_rows, inventory_functions):
        self.rows = {r.rva: r for r in map_rows}
        self.functions = sorted(inventory_functions, key=lambda f: f["rva"])
        self.starts = [f["rva"] for f in self.functions]

    def identity(self, target):
        row = self.rows.get(target)
        suffix = ""
        if row is None and target in ORACLE_KNOWN:
            return ORACLE_KNOWN[target]
        if row is None:
            i = bisect.bisect_right(self.starts, target) - 1
            if i >= 0:
                fn = self.functions[i]
                if target < fn["rva"] + max(fn["size"], 1):
                    row = self.rows.get(fn["rva"])
                    if target != fn["rva"]:
                        suffix = f"+0x{target - fn['rva']:x}"
                    if row is None:
                        if fn.get("label") and fn.get("label_confidence") != "stable-id":
                            label = fn["label"].split("FID_conflict:")[-1]
                            return short_key(demangle(label).components) + suffix
                        return f"census:{fn['id']}" + suffix
        if row is None:
            return f"code:0x{target:08x}"
        if row.symbol is not None:
            return row.symbol.key + suffix
        if row.names:
            return short_key(row.names[0].components) + suffix
        return f"census:{row.id}" + suffix


def candidate_identity(index, target):
    s = index.function_at.get(target)
    if s is None:
        s = index.containing(target)
        if s is None:
            return f"code:0x{target:08x}"
        return s.key + f"+0x{target - s.rva:x}"
    return s.key


def candidate_data_token(index, target):
    s = index.containing(target)
    if s is None:
        return f"cand:0x{target:08x}"
    if s.name in DROPPED_CANDIDATE_DATA:
        return None
    return f"{s.name}+0x{target - s.rva:x}" if target != s.rva else s.name


def primary_row(group):
    """The row that is the function's entry: an "[entry]" row, else the first untagged one."""
    for row in group:
        if any("entry" in t for t in row.tags or ()):
            return row
    for row in group:
        if not row.tags:
            return row
    return group[0]


def oracle_features(image, group, resolver):
    """Union of the group's rows. A call to the primary row is recursion ("self"); a call to
    another row of the group is an outlined block of the same function and is not a call."""
    starts = {r.rva for r in group}
    extent = [(r.rva, r.rva + r.size) for r in group]
    primary = primary_row(group)

    def resolve_code(target, is_jump):
        if any(lo <= target < hi for lo, hi in extent):
            return "self" if target == primary.rva and not is_jump else None
        return resolver.identity(target)

    def resolve_data(target):
        if target in ORACLE_HOST_GLOBALS:
            return None
        return f"oracle:0x{target:08x}"

    return extract(image, sorted(starts), extent, resolve_code, resolve_data)


def resolve_seams(resolver):
    """HOST_SEAMS with every "@rva" replaced by the oracle identity at that RVA."""
    return {name: [resolver.identity(int(t[1:], 16)) if t.startswith("@") else t
                   for t in tokens]
            for name, tokens in HOST_SEAMS.items()}


def candidate_features(image, symbol, index, seams=None):
    extent = [(symbol.rva, symbol.end)]

    def resolve_code(target, is_jump):
        if symbol.rva <= target < symbol.end:
            return None if is_jump else "self"
        return candidate_identity(index, target)

    feats = extract(image, [symbol.rva], extent, resolve_code,
                    lambda t: candidate_data_token(index, t))
    if seams:
        calls = []
        for call in feats.calls:
            calls.extend(seams.get(call, [call]))
        feats.calls = calls
    return feats


def learn_data_map(pairs, locate=None):
    """oracle token -> (candidate token, method), from [(oracle tokens, candidate tokens)].

    locate(candidate token) -> (symbol name, symbol rva, symbol end, offset) or None; it
    enables the delta phase (a symbol whose confident pairs share one oracle-candidate delta
    maps every oracle address that the delta places inside it).
    """
    def interesting(tok):
        return tok.startswith("oracle:")

    o_rows, c_rows, co = Counter(), Counter(), Counter()
    per_group = []
    for o_list, c_list in pairs:
        o_set = {t for t in o_list if interesting(t)}
        c_set = {t for t in c_list if not t.startswith("fnptr:")}
        per_group.append((o_set, c_set))
        o_rows.update(o_set)
        c_rows.update(c_set)
        for o in o_set:
            for c in c_set:
                co[(o, c)] += 1
    best_o, best_c = defaultdict(list), defaultdict(list)
    for (o, c), n in co.items():
        j = n / (o_rows[o] + c_rows[c] - n)
        if (j == 1.0 and n >= 2) or (j >= 0.6 and n >= 3):
            best_o[o].append((j, n, c))
            best_c[c].append((j, n, o))
    mapping = {}
    for o, cands in best_o.items():
        cands.sort(reverse=True)
        if len(cands) > 1 and cands[0][:2] == cands[1][:2]:
            continue
        j, n, c = cands[0]
        rivals = sorted(best_c[c], reverse=True)
        if rivals[0][2] != o or (len(rivals) > 1 and rivals[0][:2] == rivals[1][:2]):
            continue
        mapping[o] = (c, "cooccurrence")
    if locate is not None:
        deltas = defaultdict(Counter)
        extents = {}
        for o, (c, _) in mapping.items():
            where = locate(c)
            if where:
                name, rva, end, off = where
                deltas[name][_oracle_rva(o) - (rva + off)] += 1
                extents[name] = (rva, end)
        rigid = {}
        for name, counter in deltas.items():
            delta, n = counter.most_common(1)[0]
            if n >= 3 and n * 4 >= sum(counter.values()) * 3:
                rigid[name] = delta
        for o in sorted(o_rows):
            if o in mapping:
                continue
            for name, delta in rigid.items():
                rva, end = extents[name]
                t = _oracle_rva(o) - delta
                if rva <= t < end:
                    mapping[o] = (name if t == rva else f"{name}+0x{t - rva:x}", "delta")
                    break
    used = {c for c, _ in mapping.values()}
    votes = defaultdict(Counter)
    for o_set, c_set in per_group:
        o_left = sorted(o for o in o_set if o not in mapping)
        c_left = sorted((c for c in c_set if c not in used), key=_token_order)
        if o_left and len(o_left) == len(c_left):
            for o, c in zip(o_left, c_left):
                votes[o][c] += 1
    for o, counter in votes.items():
        (c, n), total = counter.most_common(1)[0], sum(counter.values())
        if n >= 2 and n * 2 > total and c not in used:
            mapping[o] = (c, f"rank {n}/{total}")
            used.add(c)
    return mapping


def _oracle_rva(token):
    return int(token.split(":", 1)[1], 16)


def _token_order(token):
    m = re.match(r"(.*)\+0x([0-9a-f]+)$", token)
    return (m.group(1), int(m.group(2), 16)) if m else (token, 0)


def _hex(value):
    return f"-{-value:#x}" if value < 0 else f"{value:#x}"


def _field_diff(oracle, candidate):
    """(base class, byte) pairs present on one side only, as "-this[0x88..0x8b]" ranges."""
    out = []
    for sign, only in (("-", oracle - candidate), ("+", candidate - oracle)):
        for cls in sorted({c for c, _ in only}):
            offsets = sorted(off for c, off in only if c == cls)
            for lo, hi in _runs(offsets):
                text = _hex(lo) if lo == hi else f"{_hex(lo)}..{_hex(hi)}"
                out.append(f"{sign}{cls}[{text}]")
    return out


POINTER_CLASSES = {"derived": "ptr", "other": "ptr"}


def _merge_token(token):
    cls, sep, rest = token.partition(":")
    return f"{POINTER_CLASSES.get(cls, cls)}:{rest}" if sep and cls in POINTER_CLASSES else token


def _pointer_merged(sets):
    """fields, accesses and imms with the derived and other base classes merged."""
    return {"fields": {(POINTER_CLASSES.get(c, c), b) for c, b in sets["fields"]},
            "accesses": {(POINTER_CLASSES.get(c, c), o, n) for c, o, n in sets["accesses"]},
            "imms": {_merge_token(t) for t in sets["imms"]}}


def _narrowing_only(o_sets, c_sets):
    """True when every byte one side touches and the other does not lies inside an access of
    its own side that also covers a byte the other side touches: a narrower or wider access to
    the same field. Two different one-byte fields next to each other are not narrowing."""
    sides = ((o_sets, c_sets), (c_sets, o_sets))
    for mine, other in sides:
        for cls, byte in mine["fields"] - other["fields"]:
            if not any(a_cls == cls and off <= byte < off + size
                       and any((cls, x) in other["fields"] for x in range(off, off + size))
                       for a_cls, off, size in mine["accesses"]):
                return False
    return True


def _set_diff(oracle, candidate):
    o, c = set(oracle), set(candidate)
    return [f"-{k}" for k in sorted(o - c)] + [f"+{k}" for k in sorted(c - o)]


def _multiset_diff(oracle, candidate):
    o, c = Counter(oracle), Counter(candidate)
    out = [f"-{k}" + (f" x{v}" if v > 1 else "") for k, v in sorted((o - c).items())]
    out += [f"+{k}" + (f" x{v}" if v > 1 else "") for k, v in sorted((c - o).items())]
    return out


def _is_number(text):
    try:
        float(text)
        return True
    except ValueError:
        return False


def _float_pairs(diff, o=None, c=None):
    """Oracle-only/candidate-only float pairs that are one constant written two ways.

    reciprocal  a*b == 1 (a division by a constant compiled as a multiplication)
    halving     a*b == 2 (the same, with a factor of two folded in)
    negated     a == -b, only when the side holding the positive value has more fchs than
                the other (it negates at run time what the other side stores negative)
    """
    minus = [t[1:] for t in diff if t.startswith("-") and _is_number(t[1:])]
    plus = [t[1:] for t in diff if t.startswith("+") and _is_number(t[1:])]
    pairs, taken = [], set()
    for a in minus:
        for b in plus:
            if b in taken:
                continue
            x, y = float(a), float(b)
            if not x or not y or x != x or y != y:
                continue
            product = x * y
            if abs(product - 1.0) < 1e-6:
                how = "reciprocal"
            elif abs(product - 2.0) < 2e-6:
                how = "halving"
            elif x == -y and o is not None and c is not None and (
                    (x < 0 and c.fchs > o.fchs) or (x > 0 and o.fchs > c.fchs)):
                how = "negated"
            else:
                continue
            pairs.append((a, b, how))
            taken.add(b)
            break
    return pairs


FEATURE_KINDS = ("calls", "strings", "floats", "data")
REVIEW_KINDS = ("fields", "imms", "x87ops")
ALL_KINDS = FEATURE_KINDS + REVIEW_KINDS


def _key_sets(feats, data_map, oracle_side):
    data = [data_map.get(t, (t, ""))[0] for t in feats.data] if oracle_side else feats.data
    return {"calls": set(feats.calls), "strings": set(feats.strings),
            "floats": set(feats.floats), "data": set(data), "fields": set(feats.fields),
            "imms": set(feats.logic), "x87ops": set(feats.x87ops),
            "accesses": set(feats.accesses)}


def _diff_size(a, b):
    return sum(len(a[k] ^ b[k]) for k in ALL_KINDS)


def _expand_inlining(o_sets, c_sets, expand_o, expand_c, data_map, rounds=3):
    """Absorb one side's out-of-line callee into it when the other side does not call it and
    absorbing it shrinks the difference: the other side inlined that callee."""
    notes = []
    sides = ((o_sets, c_sets, expand_o, "candidate inlines", True),
             (c_sets, o_sets, expand_c, "oracle inlines", False))
    for _ in range(rounds):
        changed = False
        for mine, other, expand, label, oracle_side in sides:
            if expand is None:
                continue
            for ident in sorted(mine["calls"] - other["calls"]):
                if ident == "self" or ident.startswith("icall"):
                    continue
                feats = expand(ident)
                if feats is None:
                    continue
                trial = {k: set(v) for k, v in mine.items()}
                extra = _key_sets(feats, data_map, oracle_side)
                extra["calls"] = {ident if c == "self" else c for c in extra["calls"]}
                for k in ALL_KINDS + ("accesses",):
                    trial[k] |= extra[k]
                trial["calls"].discard(ident)
                if _diff_size(trial, other) < _diff_size(mine, other):
                    mine.clear()
                    mine.update(trial)
                    notes.append(f"{label} {ident}")
                    changed = True
        if not changed:
            break
    return notes


def classify(o, c, data_map, expand_o=None, expand_c=None):
    """(class, details) for oracle and candidate Features.

    calls, strings, floats and data are compared as sets, after inlining is accounted for; a
    difference only in how often an element occurs, in trivial float constants, or in a float
    written as its reciprocal or negation is shape.
    """
    o_sets = _key_sets(o, data_map, True)
    c_sets = _key_sets(c, data_map, False)
    inlined = _expand_inlining(o_sets, c_sets, expand_o, expand_c, data_map)
    details = {k: _set_diff(o_sets[k], c_sets[k]) for k in FEATURE_KINDS}
    details["fields"] = _field_diff(o_sets["fields"], c_sets["fields"])
    # Classification treats "derived" and "other" as one pointer class: which of the two a
    # register is depends on which writer comes last in address order, not on the source.
    mo, mc = _pointer_merged(o_sets), _pointer_merged(c_sets)
    fields_review = bool(_field_diff(mo["fields"], mc["fields"])) and not _narrowing_only(mo, mc)
    details["x87ops"] = _set_diff(o_sets["x87ops"], c_sets["x87ops"])
    details["report_jcc"] = _multiset_diff(list(o.jccpairs.elements()),
                                           list(c.jccpairs.elements()))
    details["report_stores"] = _multiset_diff(list(o.stores.elements()),
                                              list(c.stores.elements()))
    if inlined:
        details["imms"] = _set_diff(o_sets["imms"], c_sets["imms"])
        imms_review = bool(_set_diff(mo["imms"], mc["imms"]))
    else:
        details["imms"] = _multiset_diff(list(o.logic.elements()), list(c.logic.elements()))
        imms_review = bool(_multiset_diff([_merge_token(t) for t in o.logic.elements()],
                                          [_merge_token(t) for t in c.logic.elements()]))
    shape = []
    if inlined:
        shape.append("inlining: " + ", ".join(inlined))
    memops = [t for t in details["calls"] if t[1:] in MEMORY_CALLS]
    if memops:
        details["calls"] = [t for t in details["calls"] if t not in memops]
        shape.append("memory ops " + " ".join(memops))
    trivial = [t for t in details["floats"] if t[1:] in TRIVIAL_FLOATS]
    if trivial:
        details["floats"] = [t for t in details["floats"] if t not in trivial]
        shape.append("trivial floats " + " ".join(trivial))
    for a, b, how in _float_pairs(details["floats"], o, c):
        details["floats"].remove(f"-{a}")
        details["floats"].remove(f"+{b}")
        shape.append(f"{how} float {a}~{b}")
    if not inlined:
        o_data = [data_map.get(t, (t, ""))[0] for t in o.data]
        raw = {"calls": (o.calls, c.calls),
               "strings": (list(o.strings.elements()), list(c.strings.elements())),
               "floats": (list(o.floats.elements()), list(c.floats.elements())),
               "data": (o_data, c.data)}
        for k, (a, b) in raw.items():
            if not details[k] and Counter(a) != Counter(b):
                shape.append(f"{k} counts " + " ".join(_multiset_diff(a, b)[:6]))
        if not details["calls"] and Counter(o.calls) == Counter(c.calls) and o.calls != c.calls:
            shape.append("call order")
    for name in ("n_insn", "n_jcc", "x87", "sse", "switch", "icall_reg", "fchs"):
        a, b = getattr(o, name), getattr(c, name)
        if a != b:
            shape.append(f"{name} {a}->{b}")
    if o.frame != c.frame:
        shape.append(f"ebp frame {'yes' if o.frame else 'no'}->{'yes' if c.frame else 'no'}")
    if o.float_widths != c.float_widths:
        shape.append("float widths " + ",".join(_multiset_diff(o.float_widths.elements(),
                                                                c.float_widths.elements())))
    imm = _multiset_diff([f"0x{v:x}" for v in o.imms.elements()],
                         [f"0x{v:x}" for v in c.imms.elements()])
    if imm:
        shape.append("imms " + " ".join(imm[:12]) + (" ..." if len(imm) > 12 else ""))
    details["shape"] = shape
    if any(details[k] for k in FEATURE_KINDS):
        return "DIFF", details
    if details["fields"] and not fields_review:
        # narrower/wider access to the same field, or the same field through a pointer the
        # tracer called derived on one side and other on the other
        shape.append("field bytes " + " ".join(details["fields"]))
        details["fields"] = []
    if details["imms"] and not imms_review:
        shape.append("imms base class " + " ".join(details["imms"]))
        details["imms"] = []
    if any(details[k] for k in REVIEW_KINDS):
        return "REVIEW", details
    if shape:
        return "SHAPE", details
    return "MATCH", details


def diff_causes(details, seams, library):
    """Coarse tags for a DIFF's elements, to sort the rows for triage."""
    host, alloc = set(), {"operator new", "operator delete", "operator new[]",
                          "operator delete[]", "malloc", "free", "calloc", "realloc"}
    if library == "qhull":
        host = {t for name, toks in seams.items() if name.startswith("qh") for t in toks}
        host.add("fprintf")
    else:
        alloc |= {t for name, toks in seams.items() if name.startswith("opc") for t in toks}
    tags = []
    for token in details.get("calls", []):
        ident = token[1:]
        if ident.startswith("icall[host]"):
            host.add(ident)
        if ident.startswith("icall[getter]") and library != "qhull":
            alloc.add(ident)
        if ident in alloc:
            tag = "allocator"
        elif ident in host:
            tag = "host-seam"
        elif ident in ("_CIsqrt", "sqrt"):
            tag = "sqrt-intrinsic"
        elif "iterator'" in ident:
            tag = "vector-iterator"
        else:
            tag = "calls"
        if tag not in tags:
            tags.append(tag)
    for kind in ("strings", "floats", "data"):
        if details.get(kind):
            tags.append(kind)
    return tags


# --------------------------------------------------------------------------- driver

OUTPUT_COLUMNS = ["rva", "id", "size", "grade", "source_function", "class", "compare_class",
                  "group_rows", "candidate_symbol", "candidate_rva", "candidate_size",
                  "oracle_insns", "candidate_insns", "diff_causes", "diff_calls",
                  "diff_strings", "diff_floats", "diff_data", "diff_fields", "diff_imms",
                  "diff_x87ops", "shape", "report_jcc", "report_stores", "notes"]


def match_library(rows, oracle_image, candidate_image, index, resolver, seams=None):
    """Features for every group. Returns (records, pairs); pairs feed learn_data_map."""
    groups = group_rows(rows)
    records, pairs = [], []
    for key, group in groups.items():
        group.sort(key=lambda r: r.rva)
        head = group[0]
        o_feat = oracle_features(oracle_image, group, resolver)
        rec = {"key": key, "rows": group, "oracle": o_feat, "candidate": None,
               "class": None, "details": {}, "notes": "", "mapcheck": mapcheck(group)}
        if head.symbol is None:
            if head.options:
                rec["class"] = "AMBIGUOUS"
                rec["notes"] = "candidates: " + " | ".join(s.name for s in head.options[:6])
            else:
                rec["class"] = "MISSING"
        else:
            rec["candidate"] = candidate_features(candidate_image, head.symbol, index, seams)
            pairs.append((o_feat.data, rec["candidate"].data))
        records.append(rec)
    return records, pairs


def mapcheck(group):
    """A group that unites several untagged rows none of which the map calls a continuation
    block: the map may have given two different functions the same name."""
    heads = [r for r in group if not r.tags and "continuation" not in (r.notes or "")]
    return len(heads) > 1


def record_identity(rec):
    head = rec["rows"][0]
    if head.symbol is not None:
        return head.symbol.key
    return short_key(head.names[0].components) if head.names else f"census:{head.id}"


class Expanders:
    """Out-of-line bodies by call identity, for the inlining check: the oracle side from the
    matched groups (then any inventory row), the candidate side from the map."""

    def __init__(self, records, oracle_image, candidate_image, index, resolver, functions,
                 seams):
        self.oracle_by_ident = {}
        for rec in records:
            if not any("instantiation" in t for t in rec["rows"][0].tags or ()):
                self.oracle_by_ident.setdefault(record_identity(rec), rec["oracle"])
        self.functions = {f"census:{f['id']}": f for f in functions}
        self.oracle_image, self.candidate_image = oracle_image, candidate_image
        self.index, self.resolver, self.seams = index, resolver, seams
        self.by_key = defaultdict(list)
        for s in index.symbols:
            if s.is_function:
                self.by_key[s.key].append(s)
        self.cache = {}

    def oracle(self, ident):
        if ident in self.oracle_by_ident:
            return self.oracle_by_ident[ident]
        fn = self.functions.get(ident)
        if fn is None or fn.get("kind") != "code":
            return None
        key = ("o", ident)
        if key not in self.cache:
            row = MapRow("", "", fn["rva"], fn["id"], fn["size"], "", "", "", "", [], [])
            self.cache[key] = oracle_features(self.oracle_image, [row], self.resolver)
        return self.cache[key]

    def candidate(self, ident):
        symbols = self.by_key.get(ident)
        if not symbols or len(symbols) > 4:
            return None
        key = ("c", ident)
        if key not in self.cache:
            feats = Features()
            for s in symbols:
                feats.merge(candidate_features(self.candidate_image, s, self.index, self.seams))
            self.cache[key] = feats
        return self.cache[key]


def finish(records, data_map, all_records, expanders=None, seams=None):
    """Classify every record; name the likely inliners of MISSING rows."""
    by_identity = defaultdict(set)
    for rec in all_records:
        head = rec["rows"][0]
        name = head.symbol.name if head.symbol else None
        for call in rec["oracle"].calls:
            by_identity[call].add((head.source_function, name))
    expand_o = expanders.oracle if expanders else None
    expand_c = expanders.candidate if expanders else None
    for rec in records:
        if rec["candidate"] is not None:
            rec["class"], rec["details"] = classify(rec["oracle"], rec["candidate"], data_map,
                                                    expand_o, expand_c)
            if rec["class"] == "DIFF":
                rec["details"]["causes"] = diff_causes(rec["details"], seams or {},
                                                       rec["rows"][0].library)
            rec["compare_class"] = rec["class"]
            if rec["mapcheck"]:
                rec["class"] = "MAPCHECK"
                rec["notes"] = ("several untagged map rows resolve to this symbol; check the "
                                "map before trusting compare_class")
        elif rec["class"] == "MISSING":
            ident = record_identity(rec)
            inliners = sorted(n or f"(unresolved: {s})" for s, n in by_identity.get(ident, ()))
            rec["notes"] = ("no candidate symbol; oracle callers -> candidate: "
                            + (", ".join(inliners[:6]) if inliners else "none among map rows"))
    return records


def write_csv(path, records):
    lines = []
    for rec in records:
        group = rec["rows"]
        c = rec["candidate"]
        head = group[0]
        d = rec["details"]
        for row in group:
            lines.append({
                "rva": f"0x{row.rva:08x}", "id": row.id, "size": row.size, "grade": row.grade,
                "source_function": row.source_function, "class": rec["class"],
                "compare_class": rec.get("compare_class", ""),
                "group_rows": len(group),
                "candidate_symbol": head.symbol.name if head.symbol else "",
                "candidate_rva": f"0x{head.symbol.rva:08x}" if head.symbol else "",
                "candidate_size": head.symbol.end - head.symbol.rva if head.symbol else "",
                "oracle_insns": rec["oracle"].n_insn,
                "candidate_insns": c.n_insn if c else "",
                "diff_causes": " ".join(d.get("causes", [])),
                "diff_calls": "; ".join(d.get("calls", [])),
                "diff_strings": "; ".join(d.get("strings", [])),
                "diff_floats": "; ".join(d.get("floats", [])),
                "diff_data": "; ".join(d.get("data", [])),
                "diff_fields": " ".join(d.get("fields", [])),
                "diff_imms": " ".join(d.get("imms", [])),
                "diff_x87ops": " ".join(d.get("x87ops", [])),
                "report_jcc": " ".join(d.get("report_jcc", [])[:16]),
                "report_stores": " ".join(d.get("report_stores", [])[:16]),
                "shape": "; ".join(d.get("shape", [])),
                "notes": rec["notes"],
            })
    lines.sort(key=lambda r: r["rva"])
    with Path(path).open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=OUTPUT_COLUMNS, lineterminator="\n")
        writer.writeheader()
        writer.writerows(lines)
    return lines


def summarise(lines):
    counts = Counter(r["class"] for r in lines)
    size = Counter()
    for r in lines:
        size[r["class"]] += int(r["size"])
    return counts, size


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--oracle", default=str(DEFAULT_ORACLE))
    parser.add_argument("--candidate",
                        default=str(REPO_DIR / "build" / "Release" / "NxPhysics.dll"))
    parser.add_argument("--candidate-map",
                        default=str(REPO_DIR / "build" / "Release" / "NxPhysics.map"))
    parser.add_argument("--inventory", default=str(EV_DIR / "inventory.json"))
    parser.add_argument("--map-dir", default=str(MAP_DIR))
    parser.add_argument("--out-dir", default=str(MAP_DIR))
    args = parser.parse_args(argv)
    try:
        oracle = Image.from_pe(args.oracle)
        candidate = Image.from_pe(args.candidate)
        map_text = Path(args.candidate_map).read_text(encoding="utf-8", errors="replace")
        inventory = json.loads(Path(args.inventory).read_text(encoding="utf-8"))
        all_rows = {lib: load_map_rows(lib, args.map_dir) for lib in LIBRARIES}
    except (OSError, ValueError, KeyError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    oracle.host_globals = ORACLE_HOST_GLOBALS
    symbols = parse_map(map_text, candidate.base, candidate)
    if not symbols:
        print("error: no symbols in the candidate map", file=sys.stderr)
        return 2
    index = SymbolIndex(symbols)
    functions = [dict(f, rva=int(f["rva"], 16)) for f in inventory["functions"]]
    for rows in all_rows.values():
        resolve_rows(rows, index)
    resolver = OracleResolver([r for rows in all_rows.values() for r in rows], functions)
    seams = resolve_seams(resolver)
    results, pairs = {}, []
    for lib in sorted(LIBRARIES):
        records, lib_pairs = match_library(all_rows[lib], oracle, candidate, index, resolver,
                                           seams)
        results[lib] = records
        pairs.extend(lib_pairs)
    data_map = learn_data_map(pairs, index.locate)
    every = [rec for recs in results.values() for rec in recs]
    expanders = Expanders(every, oracle, candidate, index, resolver, functions, seams)
    out_dir = Path(args.out_dir)
    for lib in sorted(LIBRARIES):
        finish(results[lib], data_map, every, expanders, seams)
        lines = write_csv(out_dir / f"{lib}_match.csv", results[lib])
        counts, size = summarise(lines)
        print(f"{lib}: rows={len(lines)} " + " ".join(
            f"{k}={counts.get(k, 0)}/{size.get(k, 0)}B" for k in CLASSES))
    with (out_dir / "vendored_data_map.csv").open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle, lineterminator="\n")
        writer.writerow(["oracle_rva", "candidate_symbol", "method"])
        for o, (c, how) in sorted(data_map.items()):
            writer.writerow([o.split(":", 1)[1], c, how])
    print(f"data correspondences={len(data_map)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
