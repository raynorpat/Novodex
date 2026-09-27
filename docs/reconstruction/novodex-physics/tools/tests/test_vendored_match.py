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



def run(code, host_globals=(), extra=None):
    """Features of one synthetic function at TEXT (raw bytes or an Asm)."""
    asm = code if isinstance(code, Asm) else Asm(TEXT).raw(code)
    functions = {TEXT: asm}
    functions.update(extra or {})
    image = make_image(functions, rdata_blob())
    image.host_globals = frozenset(host_globals)
    return vm.extract(image, [TEXT], [(TEXT, TEXT + 0x100)],
                      lambda t, j: f"fn:{t:x}", lambda t: f"g:{t:x}")


class FieldAndLogicTest(unittest.TestCase):
    def test_field_bytes_exclude_stack_lea_negative_and_indexed(self):
        feats = run(b"\x8b\x81\x88\x00\x00\x00"      # mov eax,[ecx+0x88]
                    b"\x8b\x44\x24\x08"              # mov eax,[esp+8]
                    b"\x8d\x41\x10"                  # lea eax,[ecx+0x10]
                    b"\x8b\x41\xfc"                  # mov eax,[ecx-4]
                    b"\x8b\x44\x81\x08"              # mov eax,[ecx+eax*4+8]
                    b"\xc3")
        self.assertEqual(feats.fields, {("this", b) for b in range(0x88, 0x8c)})

    def test_ebp_is_a_field_base_only_without_a_frame(self):
        framed = run(b"\x55\x8b\xec\x8b\x45\x08\x8b\xe5\x5d\xc3")
        frameless = run(b"\x8b\x45\x08\xc3")
        self.assertTrue(framed.frame)
        self.assertEqual(framed.fields, set())
        self.assertEqual(frameless.fields, {("other", b) for b in range(8, 12)})

    def test_logic_immediates_skip_status_masks_and_zero(self):
        feats = run(b"\x83\xf8\x05"                  # cmp eax,5
                    b"\xf6\xc4\x41"                  # test ah,0x41
                    b"\x83\xf9\x00"                  # cmp ecx,0
                    b"\x83\xf8\xff"                  # cmp eax,-1
                    b"\xc3")
        self.assertEqual(feats.logic, Counter({"5": 1, "-1": 1}))

    def test_narrowed_memory_test_and_register_test_agree(self):
        narrow = run(b"\xf6\x41\x02\x01\xc3")                   # test byte [ecx+2],1
        wide = run(b"\xf7\x01\x00\x00\x01\x00\xc3")             # test dword [ecx],0x10000
        loaded = run(b"\x8b\x01\xa9\x00\x00\x01\x00\xc3")       # mov eax,[ecx]; test eax,..
        self.assertEqual(narrow.logic, Counter({"this:bit@16": 1}))
        self.assertEqual(wide.logic, narrow.logic)
        self.assertEqual(loaded.logic, narrow.logic)
        self.assertEqual(narrow.fields, {("this", 2)})

    def test_high_byte_register_test_names_the_high_byte_bits(self):
        # mov ecx,[eax+0x50]; test ch,8 tests bit 11 of the dword (byte +0x51, bit 3), the
        # same bit as test byte [eax+0x51],8 and test dword [eax+0x50],0x800.
        high = run(b"\x8b\x48\x50\xf6\xc5\x08\xc3")
        memory = run(b"\xf6\x40\x51\x08\xc3")
        wide = run(b"\xf7\x40\x50\x00\x08\x00\x00\xc3")
        self.assertEqual(high.logic, Counter({"other:bit@651": 1}))
        self.assertEqual(memory.logic, high.logic)
        self.assertEqual(wide.logic, high.logic)

    def test_ah_after_a_field_load_is_a_field_test_not_a_status_mask(self):
        field = run(b"\x8b\x40\x50\xf6\xc4\x08\xc3")        # mov eax,[eax+0x50]; test ah,8
        status = run(b"\xdf\xe0\xf6\xc4\x41\xc3")            # fnstsw ax; test ah,0x41
        self.assertEqual(field.logic, Counter({"other:bit@651": 1}))
        self.assertEqual(status.logic, Counter())

    def test_mask_outside_a_byte_field_is_kept_as_a_raw_immediate(self):
        # movzx eax,byte [ecx+4]; test ah,1 -- bit 8 is outside the one-byte field
        outside = run(b"\x0f\xb6\x41\x04\xf6\xc4\x01\xc3")
        inside = run(b"\x0f\xb6\x41\x04\xa8\x01\xc3")   # ...; test al,1
        self.assertEqual(outside.logic, Counter({"1": 1}))
        self.assertEqual(inside.logic, Counter({"this:bit@32": 1}))

    def test_memory_and_records_the_cleared_bits(self):
        feats = run(b"\x80\x61\x04\xfe\xc3")                    # and byte [ecx+4],0xfe
        self.assertEqual(feats.logic, Counter({"this:clear@32": 1}))

    def test_repe_cmps_is_memcmp(self):
        self.assertEqual(run(b"\xf3\xa6\xc3").calls, ["memcmp"])


class BaseClassAndPairsTest(unittest.TestCase):
    def test_this_through_a_copy_derived_and_other(self):
        copied = run(b"\x8b\xf1\x8b\x46\x08\xc3")          # mov esi,ecx; mov eax,[esi+8]
        derived = run(b"\x8b\x51\x04\x8b\x42\x08\xc3")     # mov edx,[ecx+4]; [edx+8]
        other = run(b"\x8b\x44\x24\x04\x8b\x40\x08\xc3")  # mov eax,[esp+4]; [eax+8]
        self.assertEqual({c for c, _ in copied.fields}, {"this"})
        self.assertEqual({(c, b) for c, b in derived.fields if b >= 8}, {("derived", b)
                                                                         for b in range(8, 12)})
        self.assertEqual(other.fields, {("other", b) for b in range(8, 12)})

    def test_register_read_modify_write_is_the_memory_form(self):
        rmw = run(b"\x8b\x41\x08\x83\xc8\x04\x89\x41\x08\xc3")  # load; or eax,4; store
        memory = run(b"\x83\x49\x08\x04\xc3")                   # or dword [ecx+8],4
        self.assertEqual(rmw.logic, Counter({"this:set@66": 1}))
        self.assertEqual(memory.logic, rmw.logic)

    def test_register_and_that_is_not_stored_back_is_a_test(self):
        feats = run(b"\x8b\x41\x08\x83\xe0\x04\xc3")           # mov eax,[ecx+8]; and eax,4
        self.assertEqual(feats.logic, Counter({"this:bit@66": 1}))

    def test_x87_operation_classes(self):
        feats = run(b"\xd8\xc1\xd9\xfa\xde\xf9\xc3")          # fadd; fsqrt; fdivp
        self.assertEqual(feats.x87ops, {"add", "sqrt", "div"})

    def test_jcc_pairs_and_normalised_stores(self):
        feats = run(b"\x83\xf8\x05\x7c\x00"                     # cmp eax,5; jl
                    b"\xc7\x41\x04\x00\x00\x80\x3f"             # mov dword [ecx+4],1.0f
                    b"\xd9\xe8\xd9\x59\x08"                     # fld1; fstp dword [ecx+8]
                    b"\xc7\x41\x0c\x00\x00\x00\x00\xc3")        # mov dword [ecx+0xc],0
        self.assertEqual(feats.jccpairs, Counter({"5:jl": 1}))
        self.assertEqual(feats.stores, Counter({"this:0x4=f:1.0": 1, "this:0x8=f:1.0": 1,
                                                "this:0xc=0": 1}))


class IcallTagTest(unittest.TestCase):
    def test_returned_pointer_and_clobbered_base(self):
        callee = {TEXT + 0x80: body(lambda a: a.ret(), TEXT + 0x80)}
        ret = Asm(TEXT).call(TEXT + 0x80).raw(b"\xff\x50\x04\xc3")     # call [eax+4]
        lost = Asm(TEXT).call(TEXT + 0x80).raw(b"\xff\x52\x04\xc3")    # call [edx+4]
        self.assertEqual(run(ret, extra=callee).calls, ["fn:1080", "icall[ret]+0x4"])
        self.assertEqual(run(lost, extra=callee).calls, ["fn:1080", "icall[?]+0x4"])

    def test_host_global_object(self):
        asm = Asm(TEXT).raw(b"\x8b\x0d").abs32(DATA).raw(b"\x8b\x01\x51\xff\x50\x10\xc3")
        self.assertEqual(run(asm, host_globals={DATA}).calls, ["icall[host]+0x10"])
        self.assertEqual(run(asm).calls, ["icall[gobj]+0x10"])

    def test_getter_object(self):
        asm = Asm(TEXT).call(TEXT + 0x80).raw(b"\x8b\x10\xff\x52\x0c\xc3")
        feats = run(asm, extra={TEXT + 0x80: body(lambda a: a.ret(), TEXT + 0x80)})
        self.assertEqual(feats.calls, ["fn:1080", "icall[getter]+0xc"])

    def test_this_vtable_and_member_function_pointer(self):
        self.assertEqual(run(b"\x8b\x01\xff\x50\x04\xc3").calls, ["icall[obj]+0x4"])
        self.assertEqual(run(b"\x8b\x41\x08\xff\x50\x04\xc3").calls, ["icall[fptr]+0x4"])


class Task3RulesTest(unittest.TestCase):
    def test_register_call_through_a_vtable_slot_is_the_memory_call(self):
        # mov eax,[ecx]; mov edx,[eax+8]; call edx  ==  mov eax,[ecx]; call [eax+8]
        register = run(b"\x8b\x01\x8b\x50\x08\xff\xd2\xc3")
        memory = run(b"\x8b\x01\xff\x50\x08\xc3")
        self.assertEqual(register.calls, ["icall[obj]+0x8"])
        self.assertEqual(register.calls, memory.calls)
        self.assertEqual(register.icall_reg, 0)
        # the slot load is the call's operand, not a field read
        self.assertEqual(register.fields, memory.fields)

    def test_register_call_through_an_absolute_slot_stays_a_register_call(self):
        asm = Asm(TEXT).raw(b"\x8b\x35").abs32(DATA).raw(b"\xff\xd6\xc3")  # mov esi,[g]; call esi
        feats = run(asm)
        self.assertEqual([c for c in feats.calls if c.startswith("icall")], [])
        self.assertEqual(feats.icall_reg, 1)

    def test_trivial_constructor_fnptr_and_iterator_drop_out(self):
        extra = {TEXT + 0x200: Asm(TEXT + 0x200).raw(b"\x8b\xc1\xc3"),
                 TEXT + 0x210: body(lambda a: a.ret(), TEXT + 0x210)}
        asm = Asm(TEXT).push_addr(TEXT + 0x200).call(TEXT + 0x210).ret()
        self.assertEqual(run(asm, extra=extra).data, ["fnptr:" + vm.TRIVIAL_CTOR_TOKEN])
        o = features(calls=[vm.VECTOR_CTOR_ITERATOR], data=["fnptr:" + vm.TRIVIAL_CTOR_TOKEN])
        cls, details = vm.classify(o, features(), {})
        self.assertEqual(cls, "SHAPE")
        self.assertIn("oracle vector constructor iterator over " + vm.TRIVIAL_CTOR_TOKEN,
                      details["shape"])

    def test_iterator_over_a_real_constructor_needs_that_constructor_inlined(self):
        o = features(calls=[vm.VECTOR_CTOR_ITERATOR], data=["fnptr:Node::Node"])
        cls, details = vm.classify(o, features(), {})
        self.assertEqual(cls, "DIFF")
        self.assertEqual(details["calls"], ["-Node::Node"])
        inner = features(strings=Counter({"str:ctor": 1}))
        o = features(calls=[vm.VECTOR_CTOR_ITERATOR], data=["fnptr:Node::Node"])
        cls, details = vm.classify(o, features(strings=Counter({"str:ctor": 1})), {},
                                   expand_o=lambda i: inner if i == "Node::Node" else None)
        self.assertEqual(cls, "SHAPE")

    def test_float_stored_as_an_integer_immediate(self):
        rdata = bytearray(rdata_blob())
        rdata[0x28:0x2c] = struct.pack("<f", 2.0)
        # fld dword [2.0]; mov eax,ecx; fstp dword [ecx+0xc]
        code = Asm(TEXT).raw(b"\xd9\x05").abs32(RDATA + 0x28).raw(b"\x8b\xc1\xd9\x59\x0c\xc3")
        image = make_image({TEXT: code}, bytes(rdata))
        cand = vm.extract(image, [TEXT], [(TEXT, TEXT + 0x100)], lambda t, j: None,
                          lambda t: None)
        orac = run(b"\xc7\x41\x0c\x00\x00\x00\x40\xc3")        # mov dword [ecx+0xc], 2.0f
        self.assertEqual(cand.const_store_loads, Counter({"2.0": 1}))
        cls, details = vm.classify(orac, cand, {})
        self.assertEqual(cls, "SHAPE")
        self.assertIn("float 2.0 stored as an immediate on the other side", details["shape"])
        # stored to a different field, it stays a difference
        elsewhere = run(b"\xc7\x41\x10\x00\x00\x00\x40\xc3")
        self.assertEqual(vm.classify(elsewhere, cand, {})[0], "DIFF")

    def test_float_used_in_arithmetic_is_not_a_store(self):
        feats = run(Asm(TEXT).fld_dword(RDATA + 0x10).raw(b"\xd8\xc1\xd9\x59\x0c\xc3"))
        self.assertEqual(feats.const_loads, Counter({"0.5": 1}))
        self.assertEqual(feats.const_store_loads, Counter())

    def test_conversion_operator_source_name(self):
        names, tags = vm.parse_source_function("Matrix3x3::operator cast [operator Matrix4x4() const]")
        self.assertEqual(names[0].components, ["Matrix3x3", "operator cast"])
        self.assertEqual(tags, ["operator Matrix4x4() const"])

    def test_seeded_data_objects_map_by_offset(self):
        start, (symbol, size, _) = next(iter(sorted(vm.SEEDED_DATA.items())))
        pairs = [([f"oracle:0x{start:08x}"], ["unrelated"])]
        mapping = vm.learn_data_map(pairs)
        self.assertEqual(mapping[f"oracle:0x{start:08x}"], (symbol, "seed"))

    def test_minus_one_against_a_negated_one(self):
        o = features(floats=Counter({"-1.0": 1}))
        c = features(floats=Counter({"1.0": 1}))
        self.assertEqual(vm.classify(o, c, {})[0], "DIFF")
        c.fchs = 1
        cls, details = vm.classify(o, c, {})
        self.assertEqual(cls, "SHAPE")
        self.assertIn("negated trivial float -1.0", details["shape"])

    def test_crt_operator_thunks_are_named(self):
        for rva, name in ((0x000f48c0, "operator new[]"), (0x000f48bb, "operator delete[]"),
                          (0x000f41f0, "operator delete"), (0x000f48c5, "operator new")):
            self.assertEqual(vm.ORACLE_KNOWN[rva], name)


class ReviewClassTest(unittest.TestCase):
    @staticmethod
    def accessed(*accesses):
        """Features touching (class, offset, size) accesses."""
        return features(fields={(c, off + i) for c, off, size in accesses for i in range(size)},
                        accesses=set(accesses))

    def test_field_difference_is_review(self):
        o = self.accessed(("this", 0x88, 4))
        c = self.accessed(("this", 0x4c, 4))
        cls, details = vm.classify(o, c, {})
        self.assertEqual(cls, "REVIEW")
        self.assertEqual(details["fields"], ["-this[0x88..0x8b]", "+this[0x4c..0x4f]"])

    def test_a_wider_access_covering_the_narrow_one_is_shape(self):
        o = self.accessed(("this", 0x52, 1))
        c = self.accessed(("this", 0x50, 4))
        cls, details = vm.classify(o, c, {})
        self.assertEqual(cls, "SHAPE")
        self.assertTrue(any(s.startswith("field bytes") for s in details["shape"]))

    def test_adjacent_one_byte_fields_are_review(self):
        # RayCollider: mClosestHit at +0x8c against mCulling at +0x8d
        o = self.accessed(("this", 0x8c, 1))
        c = self.accessed(("this", 0x8d, 1))
        self.assertEqual(vm.classify(o, c, {})[0], "REVIEW")

    def test_same_offset_through_another_base_class_is_review(self):
        o = self.accessed(("this", 0x10, 4))
        c = self.accessed(("derived", 0x10, 4))
        self.assertEqual(vm.classify(o, c, {})[0], "REVIEW")

    def test_derived_and_other_pointers_are_one_class(self):
        o = self.accessed(("other", 0x10, 4))
        c = self.accessed(("derived", 0x10, 4))
        o.logic, c.logic = Counter({"other:bit@3": 1}), Counter({"derived:bit@3": 1})
        cls, details = vm.classify(o, c, {})
        self.assertEqual(cls, "SHAPE")
        self.assertTrue(any(s.startswith("imms base class") for s in details["shape"]))

    def test_x87_operation_classes_are_review(self):
        o = features(x87ops={"add", "mul"})
        c = features(x87ops={"add", "div"})
        cls, details = vm.classify(o, c, {})
        self.assertEqual(cls, "REVIEW")
        self.assertEqual(details["x87ops"], ["-mul", "+div"])

    def test_report_only_columns_do_not_classify(self):
        o = features(jccpairs=Counter({"5:jl": 1}), stores=Counter({"this:0x4=0": 1}))
        c = features(jccpairs=Counter({"5:jge": 1}), stores=Counter({"this:0x4=1": 1}))
        cls, details = vm.classify(o, c, {})
        self.assertEqual(cls, "MATCH")
        self.assertEqual(details["report_jcc"], ["-5:jl", "+5:jge"])

    def test_logic_immediate_difference_is_review_and_diff_wins(self):
        o = features(logic=Counter({"5": 1}))
        c = features(logic=Counter({"6": 1}))
        self.assertEqual(vm.classify(o, c, {})[0], "REVIEW")
        o.calls, c.calls = ["a"], ["b"]
        self.assertEqual(vm.classify(o, c, {})[0], "DIFF")

    def test_frame_difference_is_a_shape_note(self):
        cls, details = vm.classify(features(frame=False), features(frame=True), {})
        self.assertEqual(cls, "SHAPE")
        self.assertIn("ebp frame no->yes", details["shape"])


class FloatRuleTest(unittest.TestCase):
    def test_halving_pair_is_shape_but_a_factor_of_four_is_not(self):
        self.assertEqual(vm.classify(features(floats=Counter({"0.25": 1})),
                                     features(floats=Counter({"8.0": 1})), {})[0], "SHAPE")
        self.assertEqual(vm.classify(features(floats=Counter({"0.25": 1})),
                                     features(floats=Counter({"16.0": 1})), {})[0], "DIFF")

    def test_negation_needs_a_compensating_fchs(self):
        o = features(floats=Counter({"-2.5": 1}))
        c = features(floats=Counter({"2.5": 1}))
        self.assertEqual(vm.classify(o, c, {})[0], "DIFF")
        c.fchs = 1
        cls, details = vm.classify(o, c, {})
        self.assertEqual(cls, "SHAPE")
        self.assertIn("negated float -2.5~2.5", details["shape"])


class LookupExtrasTest(unittest.TestCase):
    def setUp(self):
        self.index = vm.SymbolIndex(vm.parse_map(map_text([
            ("??_EModel@Opcode@@UAEPAXI@Z", 0x1000, "f", "a.obj"),
            ("??1Model@Opcode@@UAE@XZ", 0x1100, "f", "a.obj"),
            ("?Collide@T@Opcode@@QAE_NPBVA@2@0H@Z", 0x1200, "f", "a.obj"),
            ("?Collide@T@Opcode@@QAE_NPBVB@2@0H@Z", 0x1300, "f", "a.obj"),
            ("??DPoint@IceMaths@@QBEMABV01@@Z", 0x1400, "f", "a.obj"),
            ("_qh_f", 0x1500, "f", "a.obj"),
        ]), BASE))

    def lookup(self, text):
        names, _ = vm.parse_source_function(text)
        return self.index.lookup(names)

    def test_scalar_deleting_destructor_falls_back_to_vector(self):
        symbol, _ = self.lookup("Model::`scalar deleting destructor'")
        self.assertEqual(symbol.rva, 0x1000)

    def test_deleting_destructor_alternative_is_preferred(self):
        symbol, _ = self.lookup("Model::~Model / `scalar deleting destructor'")
        self.assertEqual(symbol.rva, 0x1000)

    def test_partial_parameters(self):
        symbol, _ = self.lookup("T::Collide(const B*, ...)")
        self.assertEqual(symbol.rva, 0x1300)

    def test_overloads_get_parameter_keys_and_operators_real_names(self):
        keys = {s.rva: s.key for s in self.index.symbols}
        self.assertEqual(keys[0x1200], "T::Collide(const A*,const A*,int)")
        self.assertEqual(keys[0x1500], "qh_f")
        self.assertEqual(keys[0x1400], "Point::operator*")


class SeamsAndDropsTest(unittest.TestCase):
    def test_resolve_seams_replaces_rva_tokens(self):
        _, rows, functions = oracle_setup()
        seams = vm.resolve_seams(vm.OracleResolver(rows, functions))
        self.assertEqual(seams["qhNovodeXFprintf"], ["icall[host]+0x10"])
        functions.append({"id": "phys_fn_004803", "rva": 0x000b4000, "size": 20,
                          "kind": "code", "label": "phys_fn_004803",
                          "label_confidence": "stable-id"})
        seams = vm.resolve_seams(vm.OracleResolver(rows, functions))
        self.assertEqual(seams["opcNovodeXAlloc"], ["census:phys_fn_004803",
                                                    "icall[getter]+0x0"])

    def test_dropped_calls(self):
        f = body(lambda a: a.call(0x1100).call(0x1200).ret(), 0x1000)
        image = make_image({0x1000: f, 0x1100: body(lambda a: a.ret(), 0x1100),
                            0x1200: body(lambda a: a.ret(), 0x1200)})
        index = vm.SymbolIndex(vm.parse_map(map_text([
            ("_qh_f", 0x1000, "f", "a.obj"), ("@__security_check_cookie@4", 0x1100, "f", "gs.obj"),
            ("_qh_g", 0x1200, "f", "a.obj")]), BASE, image))
        feats = vm.candidate_features(image, index.function_at[0x1000], index)
        self.assertEqual(feats.calls, ["qh_g"])

    def test_diff_causes(self):
        seams = {"qhNovodeXFprintf": ["icall[host]+0x10"],
                 "opcNovodeXFree": ["census:phys_fn_004803", "icall[getter]+0xc"]}
        details = {"calls": ["-fprintf", "+icall[host]+0x10", "+_CIsqrt", "+qh_x"],
                   "floats": ["-0.5"]}
        self.assertEqual(vm.diff_causes(details, seams, "qhull"),
                         ["host-seam", "sqrt-intrinsic", "calls", "floats"])
        details = {"calls": ["-census:phys_fn_004803", "+operator delete[]",
                             "-`vector constructor iterator'"]}
        self.assertEqual(vm.diff_causes(details, seams, "opcode"),
                         ["allocator", "vector-iterator"])


class GroupingTest(unittest.TestCase):
    def rows(self, specs):
        out = []
        for rva, size, source, notes in specs:
            row = vm.MapRow("qhull", "qhull_map.csv", rva, f"phys_fn_{rva:06x}", size,
                            "mapped", "geom.c", "1", source)
            row.names, row.tags = vm.parse_source_function(source)
            row.notes = notes
            out.append(row)
        return out

    def test_continuation_rows_form_one_group_and_calls_between_them_vanish(self):
        entry = body(lambda a: a.call(0x1040).call(0x1000).ret(), 0x1000)
        block = body(lambda a: a.push_addr(RDATA).ret(), 0x1040)
        image = make_image({0x1000: entry, 0x1040: block}, rdata_blob())
        rows = self.rows([(0x1000, 0x40, "qh_f", ""),
                          (0x1040, 0x20, "qh_f", "continuation block of the function")])
        index = vm.SymbolIndex(vm.parse_map(map_text([("_qh_f", 0x1000, "f", "a.obj")]), BASE))
        vm.resolve_rows(rows, index)
        groups = vm.group_rows(rows)
        self.assertEqual(len(groups), 1)
        group = next(iter(groups.values()))
        self.assertFalse(vm.mapcheck(group))
        feats = vm.oracle_features(image, group, vm.OracleResolver(rows, []))
        self.assertEqual(feats.calls, ["self"])                  # the block call is internal
        self.assertEqual(feats.strings, Counter({"str:hello\n": 1}))

    def test_two_untagged_heads_are_mapcheck(self):
        rows = self.rows([(0x1000, 0x10, "Model::Release", ""),
                          (0x1100, 0x10, "Model::Release", "")])
        self.assertTrue(vm.mapcheck(rows))


class RankPhaseTest(unittest.TestCase):
    def test_single_vote_is_not_enough(self):
        mapping = vm.learn_data_map([(["oracle:0x00003000"], ["_a"])])
        self.assertNotIn("oracle:0x00003000", mapping)

    def test_two_agreeing_votes_pair(self):
        pairs = [(["oracle:0x00003000", "oracle:0x00003010"], ["_a", "_b"]),
                 (["oracle:0x00003000", "oracle:0x00003010"], ["_a", "_c"]),
                 (["oracle:0x00003000", "oracle:0x00003020"], ["_a", "_d"])]
        mapping = vm.learn_data_map(pairs)
        self.assertEqual(mapping["oracle:0x00003000"][0], "_a")


class MainTest(unittest.TestCase):
    def test_missing_input_returns_2(self):
        import contextlib
        import io
        with contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(vm.main(["--oracle", "no-such-file.dll", "--out-dir", "."]), 2)

    def test_full_run_on_synthetic_images_returns_0(self):
        import tempfile
        from unittest import mock
        image, rows, functions = oracle_setup()
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            (tmp / "qhull_map.csv").write_text(
                "rva,id,size,grade,source_file,source_line,source_function,signals,notes\n"
                "0x00001000,phys_fn_001000,256,mapped,geom.c,1,qh_f,,\n", encoding="utf-8")
            for name in ("opcode_map.csv", "opcode_outside_span_map.csv"):
                (tmp / name).write_text(
                    "rva,id,size,grade,source_file,source_line,source_function,signals,notes\n",
                    encoding="utf-8")
            (tmp / "cand.map").write_text(map_text([("_qh_f", 0x1000, "f", "geom.obj")]),
                                          encoding="utf-8")
            (tmp / "inv.json").write_text(
                '{"functions": [{"id": "phys_fn_001000", "rva": "0x00001000", "size": 256,'
                ' "kind": "code", "label": "phys_fn_001000", "label_confidence": "stable-id"}]}',
                encoding="utf-8")
            import contextlib
            import io
            patch = mock.patch.object(vm.Image, "from_pe", side_effect=lambda p: image)
            with patch, contextlib.redirect_stdout(io.StringIO()):
                code = vm.main(["--oracle", "o", "--candidate", "c",
                                "--candidate-map", str(tmp / "cand.map"),
                                "--inventory", str(tmp / "inv.json"),
                                "--map-dir", str(tmp), "--out-dir", str(tmp)])
            self.assertEqual(code, 0)
            text = (tmp / "qhull_match.csv").read_text(encoding="utf-8")
            self.assertIn("0x00001000,phys_fn_001000,256,mapped,qh_f,", text)


class TraceAttributionTest(unittest.TestCase):
    def test_trace_macro_detection(self):
        import qhull_trace_attribution as qt
        sources = {"geom.c": 'x;\n  trace4((qh ferr, "qh_x: f%d\\n", id));\n'
                             '  fprintf(qh ferr, "qh_y: plain\\n");\n'}
        self.assertTrue(qt.in_trace_macro(sources, "qh_x: f%d\n"))
        self.assertFalse(qt.in_trace_macro(sources, "qh_y: plain\n", lines=1))
        self.assertIsNone(qt.in_trace_macro(sources, "nowhere"))


if __name__ == "__main__":
    unittest.main()
