import struct
import sys
import unittest
from collections import Counter
from pathlib import Path


TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import vendored_match as vm  # noqa: E402


BASE = 0x10000000
TEXT, RDATA, DATA = 0x1000, 0x2000, 0x3000


class Asm:
    """Just enough of an x86 assembler to lay out synthetic functions with relocations."""

    def __init__(self, origin):
        self.origin = origin
        self.code = bytearray()
        self.relocs = []

    @property
    def here(self):
        return self.origin + len(self.code)

    def raw(self, data):
        self.code += bytes(data)
        return self

    def abs32(self, rva):
        self.relocs.append(self.here)
        self.code += struct.pack("<I", BASE + rva)
        return self

    def push_addr(self, rva):
        return self.raw(b"\x68").abs32(rva)

    def call(self, target):
        rel = target - (self.here + 5)
        return self.raw(b"\xe8" + struct.pack("<i", rel))

    def jmp(self, target):
        rel = target - (self.here + 5)
        return self.raw(b"\xe9" + struct.pack("<i", rel))

    def fld_dword(self, rva):
        return self.raw(b"\xd9\x05").abs32(rva)

    def fld_qword(self, rva):
        return self.raw(b"\xdd\x05").abs32(rva)

    def load_global(self, rva):          # mov eax, [abs]
        return self.raw(b"\xa1").abs32(rva)

    def mov_ecx(self, value):
        return self.raw(b"\xb9" + struct.pack("<I", value))

    def add_esp(self, value):
        return self.raw(bytes([0x83, 0xc4, value]))

    def je_next(self):
        return self.raw(b"\x85\xc0\x74\x00")       # test eax,eax; je +0

    def icall(self, disp):                          # call [eax+disp8]
        return self.raw(bytes([0xff, 0x50, disp]))

    def ret(self):
        return self.raw(b"\xc3")


def make_image(functions, rdata=b"", data_size=0x100, extra_relocs=()):
    """functions: {rva: Asm}. Returns an Image with .text/.rdata/.data."""
    text = bytearray(b"\xcc" * 0x800)
    relocs = list(extra_relocs)
    for rva, asm in functions.items():
        text[rva - TEXT:rva - TEXT + len(asm.code)] = asm.code
        relocs.extend(asm.relocs)
    sections = [vm.Section(".text", TEXT, bytes(text), True, False),
                vm.Section(".rdata", RDATA, bytes(rdata).ljust(0x100, b"\0"), False, False),
                vm.Section(".data", DATA, bytes(data_size), False, True)]
    return vm.Image(BASE, sections, relocs, {})


def rdata_blob():
    """.rdata: "hello\\n" at +0, 0.5f at +0x10, 2.0 at +0x18, "world" at +0x20."""
    blob = bytearray(0x40)
    blob[0:7] = b"hello\n\0"
    blob[0x10:0x14] = struct.pack("<f", 0.5)
    blob[0x18:0x20] = struct.pack("<d", 2.0)
    blob[0x20:0x26] = b"world\0"
    return bytes(blob)


def map_text(symbols):
    """A tiny MSVC linker map: symbols = [(name, rva, flags, obj)]."""
    lines = [" NxPhysics", "",
             "  Address         Publics by Value              Rva+Base       Lib:Object", ""]
    for name, rva, flags, obj in symbols:
        lines.append(f" 0001:{rva - TEXT:08x}       {name:<26} {BASE + rva:08x} {flags:<3} {obj}")
    return "\n".join(lines) + "\n"


def body(asm_fn, origin):
    asm = Asm(origin)
    asm_fn(asm)
    return asm


class DemangleTest(unittest.TestCase):
    def test_c_name_loses_one_underscore(self):
        self.assertEqual(vm.demangle("_qh_distplane").components, ["qh_distplane"])
        self.assertEqual(vm.demangle("@__security_check_cookie@4").components,
                         ["_security_check_cookie"])

    def test_member_function_with_parameters(self):
        d = vm.demangle("?Build@AABBTree@Opcode@@QAE_NPAVAABBTreeBuilder@2@@Z")
        self.assertEqual(d.components, ["Opcode", "AABBTree", "Build"])
        self.assertEqual(d.params, ["AABBTreeBuilder*"])

    def test_constructor_destructor_and_deleting_destructor(self):
        self.assertEqual(vm.demangle("??0Container@IceCore@@QAE@ABV01@@Z").components[-2:],
                         ["Container", "Container"])
        self.assertEqual(vm.demangle("??0Container@IceCore@@QAE@ABV01@@Z").params,
                         ["const Container&"])
        self.assertEqual(vm.demangle("??1RadixSort@IceCore@@QAE@XZ").components[-1], "~RadixSort")
        self.assertEqual(vm.demangle("??_GAABBTreeCollider@Opcode@@UAEPAXI@Z").components[-1],
                         vm.SCALAR_DTOR)

    def test_argument_back_references(self):
        d = vm.demangle("?_Collide@AABBTreeCollider@Opcode@@IAEXPBVAABBNoLeafNode@2@0@Z")
        self.assertEqual(d.params, ["const AABBNoLeafNode*", "const AABBNoLeafNode*"])

    def test_function_pointer_parameter(self):
        d = vm.demangle("?DumpPairs@SAP_PairData@Opcode@@QBEXP6AHIIPAX@Z0@Z")
        self.assertEqual(d.params, ["fnptr", "void*"])

    def test_local_scope_is_unwrapped(self):
        d = vm.demangle("?_Walk@Local@?1??Walk@AABBCollisionTree@Opcode@@UBE_NP6A_NPBXPAX@Z1@Z@"
                        "SAXPBVAABBCollisionNode@4@21@Z")
        self.assertEqual(d.components,
                         ["Opcode", "AABBCollisionTree", "Walk", "Local", "_Walk"])


class SourceFunctionTest(unittest.TestCase):
    def test_tags_and_alternatives(self):
        names, tags = vm.parse_source_function("Model::~Model / `scalar deleting destructor'")
        self.assertEqual([n.components for n in names],
                         [["Model", "~Model"], ["Model", vm.SCALAR_DTOR]])
        names, tags = vm.parse_source_function(
            "RayCollider::_SegmentStab(const AABBCollisionNode*) [body]")
        self.assertEqual(tags, ["body"])
        self.assertEqual(names[0].params, ["const AABBCollisionNode*"])

    def test_elided_parameters_are_a_prefix(self):
        names, _ = vm.parse_source_function(
            "AABBTreeCollider::Collide(const AABBQuantizedTree*, const AABBQuantizedTree*, ...)")
        self.assertTrue(names[0].partial)
        self.assertEqual(names[0].params, ["const AABBQuantizedTree*"] * 2)

    def test_return_type_and_typedefs(self):
        names, _ = vm.parse_source_function("Container& Container::Empty()")
        self.assertEqual(names[0].components, ["Container", "Empty"])
        names, _ = vm.parse_source_function("RadixSort::Sort(const udword*, udword, RadixHint)")
        self.assertEqual(names[0].params, ["const unsigned int*", "unsigned int", "RadixHint"])


class LookupTest(unittest.TestCase):
    def setUp(self):
        self.symbols = vm.parse_map(map_text([
            ("_qh_distplane", 0x1000, "f", "NxQhull:geom.obj"),
            ("?Sort@RadixSort@IceCore@@QAEAAV12@PBIIW4RadixHint@2@@Z", 0x1100, "f",
             "NxOpcode:IceRevisitedRadix.obj"),
            ("?Sort@RadixSort@IceCore@@QAEAAV12@PBMI@Z", 0x1200, "f",
             "NxOpcode:IceRevisitedRadix.obj"),
            ("?Walk@Tree@Opcode@@QAEXXZ", 0x1300, "f", "NxOpcode:A.obj"),
            ("?Walk@Tree@Opcode@@QAEXH@Z", 0x1400, "f", "NxOpcode:B.obj"),
            ("?Run@Other@Opcode@@QAEXXZ", 0x1500, "f", "NxOpcode:A.obj"),
            ("?Run@Other@Opcode@@QAEXH@Z", 0x1600, "f", "NxOpcode:A.obj"),
        ]), BASE)
        self.index = vm.SymbolIndex(self.symbols)

    def lookup(self, text, source_file=""):
        names, _ = vm.parse_source_function(text)
        return self.index.lookup(names, source_file)

    def test_c_name(self):
        symbol, _ = self.lookup("qh_distplane")
        self.assertEqual(symbol.name, "_qh_distplane")
        self.assertEqual(symbol.end, 0x1100)

    def test_overload_by_parameters(self):
        symbol, _ = self.lookup("RadixSort::Sort(const float*, udword)")
        self.assertEqual(symbol.rva, 0x1200)

    def test_overload_by_object_file(self):
        symbol, _ = self.lookup("Tree::Walk", "B.cpp")
        self.assertEqual(symbol.rva, 0x1400)

    def test_ambiguous(self):
        symbol, options = self.lookup("Other::Run", "A.cpp")
        self.assertIsNone(symbol)
        self.assertEqual(len(options), 2)

    def test_missing(self):
        self.assertEqual(self.lookup("qh_nowhere"), (None, []))


class FeatureTest(unittest.TestCase):
    def test_constants_calls_data_and_immediates(self):
        f = body(lambda a: a.push_addr(RDATA).call(0x1100).fld_dword(RDATA + 0x10)
                 .load_global(DATA + 8).add_esp(4).mov_ecx(7).je_next().ret(), 0x1000)
        callee = body(lambda a: a.ret(), 0x1100)
        image = make_image({0x1000: f, 0x1100: callee}, rdata_blob())
        feats = vm.extract(image, [0x1000], [(0x1000, 0x1100)],
                           lambda t, j: f"fn:{t:x}", lambda t: f"g:{t:x}")
        self.assertEqual(feats.calls, ["fn:1100"])
        self.assertEqual(feats.strings, Counter({"str:hello\n": 1}))
        self.assertEqual(feats.floats, Counter({"0.5": 1}))
        self.assertEqual(feats.float_widths, Counter({4: 1}))
        self.assertEqual(feats.data, ["g:3008"])
        self.assertEqual(feats.imms, Counter({7: 1}))      # add esp,4 is a stack adjustment
        self.assertEqual(feats.n_jcc, 1)
        self.assertEqual(feats.x87, 1)

    def test_tail_call_and_x87_constant(self):
        f = body(lambda a: a.raw(b"\xd9\xee").jmp(0x1100), 0x1000)      # fldz; jmp callee
        image = make_image({0x1000: f, 0x1100: body(lambda a: a.ret(), 0x1100)})
        feats = vm.extract(image, [0x1000], [(0x1000, 0x1100)],
                           lambda t, j: f"fn:{t:x}", lambda t: None)
        self.assertEqual(feats.calls, ["fn:1100"])
        self.assertEqual(feats.floats, Counter({"0.0": 1}))

    def test_switch_table_entries_are_followed(self):
        asm = Asm(0x1000)
        asm.raw(b"\xff\x24\x85").abs32(0x1040)          # jmp [eax*4+table]
        asm.raw(b"\xcc" * (0x20 - len(asm.code)))
        asm.call(0x1100).ret()                          # case 0 at 0x1020
        asm.raw(b"\xcc" * (0x40 - len(asm.code)))
        asm.abs32(0x1020)                               # the one-entry table at 0x1040
        image = make_image({0x1000: asm, 0x1100: body(lambda a: a.ret(), 0x1100)})
        feats = vm.extract(image, [0x1000], [(0x1000, 0x1080)],
                           lambda t, j: f"fn:{t:x}", lambda t: f"g:{t:x}")
        self.assertEqual(feats.calls, ["fn:1100"])
        self.assertEqual(feats.switch, 1)

    def test_path_source_strings_compare_by_file_name(self):
        self.assertEqual(vm.string_key(r"\Epic\Novodex\SDKs\Physics\src\opcode\OPC_Model.cpp"),
                         vm.string_key(r"D:\build\External\opcode-tree\OPC_Model.cpp"))


def oracle_setup():
    """An oracle image with map rows f (0x1000), g (0x1100) and a census row at 0x1200."""
    f = body(lambda a: a.push_addr(RDATA).call(0x1100).call(0x1200).call(0x1000)
             .fld_qword(RDATA + 0x18).load_global(DATA + 4).ret(), 0x1000)
    g = body(lambda a: a.push_addr(RDATA + 0x20).ret(), 0x1100)
    census = body(lambda a: a.ret(), 0x1200)
    image = make_image({0x1000: f, 0x1100: g, 0x1200: census}, rdata_blob())
    rows = []
    for rva, size, source in ((0x1000, 0x100, "qh_f"), (0x1100, 0x10, "qh_g")):
        row = vm.MapRow("qhull", "qhull_map.csv", rva, f"phys_fn_{rva:06x}", size, "mapped",
                        "geom.c", "1", source)
        row.names, row.tags = vm.parse_source_function(source)
        rows.append(row)
    functions = [{"id": "phys_fn_001000", "rva": 0x1000, "size": 0x100, "kind": "code",
                  "label": "phys_fn_001000", "label_confidence": "stable-id"},
                 {"id": "phys_fn_001100", "rva": 0x1100, "size": 0x10, "kind": "code",
                  "label": "phys_fn_001100", "label_confidence": "stable-id"},
                 {"id": "phys_fn_001200", "rva": 0x1200, "size": 0x10, "kind": "code",
                  "label": "_strtod", "label_confidence": "semantic"}]
    return image, rows, functions


class ResolutionTest(unittest.TestCase):
    def test_oracle_call_identities(self):
        image, rows, functions = oracle_setup()
        index = vm.SymbolIndex(vm.parse_map(map_text([("_qh_g", 0x1100, "f", "geom.obj")]),
                                            BASE))
        vm.resolve_rows(rows, index)
        resolver = vm.OracleResolver(rows, functions)
        self.assertEqual(resolver.identity(0x1100), "qh_g")       # map row, resolved
        self.assertEqual(resolver.identity(0x1000), "qh_f")       # map row, source key
        self.assertEqual(resolver.identity(0x1200), "strtod")     # inventory label
        self.assertEqual(resolver.identity(0x1204), "strtod+0x4")
        self.assertEqual(resolver.identity(0x000f7a3c), "ftol")        # ORACLE_KNOWN
        self.assertEqual(resolver.identity(0x7000), "code:0x00007000")

    def test_oracle_group_calls_itself_self(self):
        image, rows, functions = oracle_setup()
        resolver = vm.OracleResolver(rows, functions)
        feats = vm.oracle_features(image, [rows[0]], resolver)
        self.assertEqual(feats.calls, ["qh_g", "strtod", "self"])
        self.assertEqual(feats.floats, Counter({"2.0": 1}))
        self.assertEqual(feats.data, ["oracle:0x00003004"])

    def test_candidate_identities_and_seams(self):
        f = body(lambda a: a.call(0x1100).call(0x1200).ret(), 0x1000)
        image = make_image({0x1000: f, 0x1100: body(lambda a: a.ret(), 0x1100),
                            0x1200: body(lambda a: a.ret(), 0x1200)})
        index = vm.SymbolIndex(vm.parse_map(map_text([
            ("_qh_f", 0x1000, "f", "a.obj"), ("_qh_g", 0x1100, "f", "a.obj"),
            ("_qhNovodeXFprintf", 0x1200, "f", "host.obj")]), BASE, image))
        symbol = index.function_at[0x1000]
        feats = vm.candidate_features(image, symbol, index, {"qhNovodeXFprintf": ["icall+0x10"]})
        self.assertEqual(feats.calls, ["qh_g", "icall+0x10"])


def features(**kw):
    f = vm.Features()
    for k, v in kw.items():
        setattr(f, k, v)
    return f


class ClassifyTest(unittest.TestCase):
    def test_match(self):
        o = features(calls=["a"], strings=Counter({"str:x": 1}), n_insn=5)
        c = features(calls=["a"], strings=Counter({"str:x": 1}), n_insn=5)
        self.assertEqual(vm.classify(o, c, {})[0], "MATCH")

    def test_shape_on_branches_counts_and_trivial_floats(self):
        o = features(calls=["a", "a"], n_jcc=2, floats=Counter({"0.5": 1}))
        c = features(calls=["a"], n_jcc=3, floats=Counter({"0.5": 1, "0.0": 1}))
        cls, details = vm.classify(o, c, {})
        self.assertEqual(cls, "SHAPE")
        self.assertTrue(any(s.startswith("n_jcc 2->3") for s in details["shape"]))
        self.assertTrue(any(s.startswith("calls counts") for s in details["shape"]))

    def test_reciprocal_float_is_shape(self):
        o = features(floats=Counter({"0.25": 1}))
        c = features(floats=Counter({"4.0": 1}))
        self.assertEqual(vm.classify(o, c, {})[0], "SHAPE")

    def test_diff_lists_both_sides(self):
        o = features(calls=["a", "b"], strings=Counter({"str:x": 1}), data=["oracle:0x3000"])
        c = features(calls=["a", "c"], strings=Counter({"str:y": 1}), data=["_g+0x4"])
        cls, details = vm.classify(o, c, {"oracle:0x3000": ("_g", "test")})
        self.assertEqual(cls, "DIFF")
        self.assertEqual(details["calls"], ["-b", "+c"])
        self.assertEqual(details["strings"], ["-str:x", "+str:y"])
        self.assertEqual(details["data"], ["-_g", "+_g+0x4"])

    def test_inlining_is_accounted_for(self):
        o = features(calls=["f", "g"], strings=Counter({"str:x": 1}))
        c = features(calls=["g"], strings=Counter({"str:x": 1, "str:inner": 1}))
        inner = features(calls=["g"], strings=Counter({"str:inner": 1}))
        cls, details = vm.classify(o, c, {}, expand_o=lambda i: inner if i == "f" else None)
        self.assertEqual(cls, "SHAPE")
        self.assertIn("inlining: candidate inlines f", details["shape"])

    def test_a_missing_call_is_not_explained_away(self):
        o = features(calls=["f"])
        c = features(calls=[])
        unrelated = features(strings=Counter({"str:z": 1}))
        cls, details = vm.classify(o, c, {}, expand_o=lambda i: unrelated)
        self.assertEqual(cls, "DIFF")
        self.assertEqual(details["calls"], ["-f"])


class DataMapTest(unittest.TestCase):
    def test_cooccurrence_then_delta(self):
        pairs = [(["oracle:0x00003000", "oracle:0x00003004"], ["_qh+0x0", "_qh+0x4"]),
                 (["oracle:0x00003000", "oracle:0x00003008"], ["_qh+0x0", "_qh+0x8"]),
                 (["oracle:0x00003004", "oracle:0x00003008"], ["_qh+0x4", "_qh+0x8"]),
                 (["oracle:0x00003010"], ["_other"])]
        # _qh+0x0/4/8 each co-occur in exactly two groups: Phase A pairs all three.
        extents = {"_qh": (0x5000, 0x5100)}

        def locate(token):
            name, _, off = token.partition("+0x")
            if name not in extents:
                return None
            return (name, extents[name][0], extents[name][1], int(off or "0", 16))

        mapping = vm.learn_data_map(pairs, locate)
        self.assertEqual(mapping["oracle:0x00003004"][0], "_qh+0x4")
        self.assertEqual(mapping["oracle:0x00003010"][0], "_qh+0x10")   # by delta
        self.assertEqual(mapping["oracle:0x00003010"][1], "delta")


class MatchLibraryTest(unittest.TestCase):
    def test_missing_names_the_inliner(self):
        image, rows, functions = oracle_setup()
        cand_f = body(lambda a: a.push_addr(RDATA).push_addr(RDATA + 0x20).call(0x1200)
                      .fld_qword(RDATA + 0x18).load_global(DATA + 4).call(0x1000).ret(), 0x1000)
        cand_image = make_image({0x1000: cand_f, 0x1200: body(lambda a: a.ret(), 0x1200)},
                                rdata_blob())
        index = vm.SymbolIndex(vm.parse_map(map_text([
            ("_qh_f", 0x1000, "f", "geom.obj"), ("_strtod", 0x1200, "f", "crt.obj")]),
            BASE, cand_image))
        vm.resolve_rows(rows, index)
        resolver = vm.OracleResolver(rows, functions)
        records, pairs = vm.match_library(rows, image, cand_image, index, resolver)
        data_map = {"oracle:0x00003004": ("cand:0x00003004", "test")}
        expanders = vm.Expanders(records, image, cand_image, index, resolver, functions, {})
        vm.finish(records, data_map, records, expanders, {})
        by_source = {r["rows"][0].source_function: r for r in records}
        self.assertEqual(by_source["qh_g"]["class"], "MISSING")
        self.assertIn("_qh_f", by_source["qh_g"]["notes"])
        # qh_f: the candidate inlined qh_g (its string "world" moved in); the rest agrees.
        f_rec = by_source["qh_f"]
        self.assertEqual(f_rec["class"], "SHAPE")
        self.assertIn("inlining: candidate inlines qh_g", f_rec["details"]["shape"])


if __name__ == "__main__":
    unittest.main()
