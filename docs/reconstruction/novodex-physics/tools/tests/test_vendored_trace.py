import struct
import sys
import tempfile
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import vendored_match as vm  # noqa: E402
import vendored_trace as vt  # noqa: E402

BASE = 0x10000000
TEXT, DATA = 0x1000, 0x3000


def image(code, relocs=(), data=b"\0" * 0x40):
    text = bytes(code).ljust(0x200, b"\xcc")
    return vm.Image(BASE, [vm.Section(".text", TEXT, text, True, False),
                           vm.Section(".data", DATA, data.ljust(0x100, b"\0"), False, True)],
                    list(relocs), {})


def symbol(name, rva, function=True, obj="lib:a.obj"):
    return vm.Symbol(name, rva, obj, function, vm.demangle(name))


def mapped(code, symbols, relocs=()):
    img = image(code, relocs)
    syms = sorted(symbols, key=lambda s: s.rva)
    starts = sorted({s.rva for s in syms})
    for s in syms:
        later = [r for r in starts if r > s.rva]
        s.end = later[0] if later else TEXT + 0x200
    return vt.Mapped(img, syms)


def call_rel(at, target):
    return b"\xe8" + struct.pack("<i", target - (at + 5))


class CompareBodies(unittest.TestCase):
    """Same code at different addresses is the same; anything else is reported."""

    def pair(self, body_a, body_b, extra_a=(), extra_b=(), relocs_a=(), relocs_b=()):
        a = mapped(body_a, [symbol("_f", TEXT)] + list(extra_a), relocs_a)
        b = mapped(body_b, [symbol("_f", TEXT)] + list(extra_b), relocs_b)
        return vt.compare_bodies(a, b, a.function("_f"), b.function("_f"), 0x40)

    def test_identical_bytes_are_the_same(self):
        body = b"\x55\x8b\xec\x5d\xc3"
        self.assertIsNone(self.pair(body, body))

    def test_a_call_is_compared_by_the_name_it_reaches(self):
        # _g sits at a different address on each side; the call reaches it on both.
        a = call_rel(TEXT, TEXT + 0x80) + b"\xc3"
        b = call_rel(TEXT, TEXT + 0x90) + b"\xc3"
        self.assertIsNone(self.pair(a, b, [symbol("_g", TEXT + 0x80)], [symbol("_g", TEXT + 0x90)]))

    def test_a_call_to_another_function_differs(self):
        a = call_rel(TEXT, TEXT + 0x80) + b"\xc3"
        b = call_rel(TEXT, TEXT + 0x80) + b"\xc3"
        found = self.pair(a, b, [symbol("_g", TEXT + 0x80)], [symbol("_h", TEXT + 0x80)])
        self.assertIn("_g", found)
        self.assertIn("_h", found)

    def test_folded_symbols_share_an_address(self):
        # /OPT:ICF gives one address two names; either name matches.
        a = call_rel(TEXT, TEXT + 0x80) + b"\xc3"
        b = call_rel(TEXT, TEXT + 0x80) + b"\xc3"
        self.assertIsNone(self.pair(a, b, [symbol("_g", TEXT + 0x80)],
                                    [symbol("_g", TEXT + 0x80), symbol("_other", TEXT + 0x80)]))

    def test_a_relocated_operand_is_compared_by_its_symbol(self):
        # mov eax, [abs]; ret -- the absolute address differs, the data symbol does not.
        a = b"\xa1" + struct.pack("<I", BASE + DATA + 0x10) + b"\xc3"
        b = b"\xa1" + struct.pack("<I", BASE + DATA + 0x20) + b"\xc3"
        self.assertIsNone(self.pair(a, b, [symbol("_d", DATA + 0x10, False)],
                                    [symbol("_d", DATA + 0x20, False)],
                                    [TEXT + 1], [TEXT + 1]))

    def test_a_changed_byte_is_reported(self):
        found = self.pair(b"\x6a\x01\xc3", b"\x6a\x02\xc3")
        self.assertIsNotNone(found)
        self.assertIn("byte", found)

    def test_padding_after_the_return_is_not_compared(self):
        self.assertIsNone(self.pair(b"\x33\xc0\xc3\xcc\xcc", b"\x33\xc0\xc3\x90\x90"))

    def test_code_after_a_return_reached_by_a_branch_is_compared(self):
        # je +1 skips the ret; the byte after it is inside the body.
        a = b"\x74\x01\xc3\x6a\x01\xc3"
        b = b"\x74\x01\xc3\x6a\x02\xc3"
        self.assertIsNotNone(self.pair(a, b))


class Script(unittest.TestCase):
    def test_breakpoints_count_into_the_page_and_boundaries_dump_it(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "s.cdb"
            vt.write_script({0x1234: ["a"], 0x2000: ["b", "c"]}, 0x500, 0x40, 7, path,
                            marks=[(0x80, "layout")])
            lines = path.read_text(encoding="ascii").splitlines()
        self.assertEqual(lines[0], "r $t19 = @$exentry - 0x500")
        self.assertTrue(lines[1].startswith(".dvalloc /b 0x60000000"))
        self.assertIn('bp1 @$t19+0x80 ".printf \\"SEG layout\\\\n\\"', lines[2])
        self.assertIn("bp2 @$t19+0x40", lines[3])
        self.assertIn("poi(@esp+4)", lines[3])
        self.assertIn("bp3 @$t19+0x1234", lines[4])
        self.assertIn("ed 0x60000000 (dwo(0x60000000)+1)", lines[4])
        self.assertIn(">= 0n7) {bd 3}", lines[4])
        self.assertIn("ed 0x60000004", lines[5])
        self.assertEqual(lines[-1], "q")


class ParseLog(unittest.TestCase):
    def test_segments_take_the_counts_printed_after_them(self):
        log = "\n".join([
            "noise",
            "SEG layout",
            "60000000  00000002 00000000 00000005",
            "SEG qh_rand",
            "60000000  00000000 00000001 00000000",
            "SEG END",
            "60000000  00000000 00000000 00000000",
        ])
        segments = vt.parse_log(log, 3)
        self.assertEqual(segments, [("layout", [2, 0, 5]), ("qh_rand", [0, 1, 0]), ("END", [0, 0, 0])])

    def test_counters_past_the_first_line_land_at_their_index(self):
        log = "SEG x\n60000000  00000001 00000000 00000000 00000000\n60000010  00000003\n"
        self.assertEqual(vt.parse_log(log, 5), [("x", [1, 0, 0, 0, 3])])

    def test_a_dump_of_other_memory_is_ignored(self):
        log = "SEG x\n0012ff00  00000009 00000009\n60000000  00000001\n"
        self.assertEqual(vt.parse_log(log, 1), [("x", [1])])


class Groups(unittest.TestCase):
    def test_a_separate_instantiation_keeps_its_own_group(self):
        header = ("rva,id,size,grade,source_function,class,compare_class,group_rows,candidate_symbol,"
                  "candidate_rva,candidate_size\n")
        rows = [
            '0x00001000,phys_fn_1,16,mapped,"F(int)",SHAPE,SHAPE,1,?F@@YAXH@Z,0x00000100,16\n',
            '0x00002000,phys_fn_2,16,probable,"F(int) [2nd instantiation]",DIFF,DIFF,1,?F@@YAXH@Z,0x00000100,16\n',
            '0x00003000,phys_fn_3,8,mapped,"G",MATCH,MATCH,2,?G@@YAXXZ,0x00000200,8\n',
            '0x00003008,phys_fn_4,8,mapped,"G",MATCH,MATCH,2,?G@@YAXXZ,0x00000200,8\n',
        ]
        with tempfile.TemporaryDirectory() as tmp:
            (Path(tmp) / "qhull_match.csv").write_text(header, encoding="utf-8")
            (Path(tmp) / "opcode_match.csv").write_text(header + "".join(rows), encoding="utf-8")
            groups = vt.load_groups(tmp)
        self.assertEqual([g["key"] for g in groups], ["?F@@YAXH@Z", "?F@@YAXH@Z#00002000", "?G@@YAXXZ"])
        self.assertTrue(groups[1]["separate"])
        self.assertEqual(groups[2]["rows"], [(0x3000, 8), (0x3008, 8)])

    def test_breakpoints_are_one_per_address(self):
        exe = mapped(b"\xc3", [symbol("?F@@YAXH@Z", TEXT), symbol("?G@@YAXXZ", TEXT)])
        groups = [
            {"class": "SHAPE", "symbol": "?F@@YAXH@Z", "key": "f"},
            {"class": "MATCH", "symbol": "?G@@YAXXZ", "key": "g"},
            {"class": "MISSING", "symbol": "?H@@YAXXZ", "key": "h"},
        ]
        self.assertEqual(dict(vt.breakpoints(groups, exe)), {TEXT: ["f", "g"]})


if __name__ == "__main__":
    unittest.main()
