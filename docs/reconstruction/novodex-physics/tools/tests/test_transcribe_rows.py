import re
import sys
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace


TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import transcribe_rows  # noqa: E402


# A synthetic image: rva 0x1000 holds two small functions.
#   0x1000: mov eax, ecx; ret                    (89 c8 c3)
#   0x1003: cmp ecx, 1; ja +7; jmp [ecx*4+tbl]   (83 f9 01 / 77 07 / ff 24 8d <tbl>)
#   0x100f: xor eax, eax; ret                    (31 c0 c3)  table target 0
#   0x1012: mov al, 1; ret                       (b0 01 c3)  table target 1, also the ja target
#   0x1015: table of two absolute addresses
BASE = 0x10000000
TABLE = BASE + 0x1015


def image():
    code = bytes([0x89, 0xc8, 0xc3,
                  0x83, 0xf9, 0x01, 0x77, 0x07, 0xff, 0x24, 0x8d]) + TABLE.to_bytes(4, 'little')
    code += bytes([0x31, 0xc0, 0xc3, 0xb0, 0x01, 0xc3])
    code += (BASE + 0x100f).to_bytes(4, 'little') + (BASE + 0x1012).to_bytes(4, 'little')
    return transcribe_rows.Image(code, 0x1000)


def cfg():
    return SimpleNamespace(
        ROWS=[('rowA', 0x1000, 0x1003, 'phys_fn_000001', '__declspec(naked) void rowA()'),
              ('rowB', 0x1003, 0x1015, 'phys_fn_000002', '__declspec(naked) void rowB()')],
        TABLES={TABLE: 2}, PREFIX='kT')


class TranscribeTest(unittest.TestCase):
    def test_emits_one_naked_body_per_row(self):
        protos, body = transcribe_rows.transcribe(cfg(), image())
        self.assertEqual(body.count('__declspec(naked)'), 2)
        self.assertEqual(protos.count(';'), 2)
        comments = [int(x, 16) for x in re.findall(r'// 0x([0-9a-f]{8})$', body, re.M)]
        self.assertEqual(comments[0], 0x1000)
        self.assertIn(0x1012, comments)
        self.assertTrue(all(0x1000 <= c < 0x1015 for c in comments))

    def test_jump_table_becomes_compare_chain(self):
        _, body = transcribe_rows.transcribe(cfg(), image())
        self.assertIn('cmp ecx, 0', body)
        self.assertIn('je L100f', body)
        self.assertIn('jmp L1012', body)
        self.assertNotIn('*4', body)

    def test_unmapped_call_target_raises(self):
        img = transcribe_rows.Image(bytes([0xe8, 0x00, 0x01, 0x00, 0x00, 0xc3]), 0x1000)
        c = SimpleNamespace(ROWS=[('r', 0x1000, 0x1006, 't', 'void r()')])
        with self.assertRaises(ValueError):
            transcribe_rows.transcribe(c, img)

    def test_int3_is_written_for_the_inline_assembler(self):
        img = transcribe_rows.Image(bytes([0xcc, 0xc3]), 0x1000)
        c = SimpleNamespace(ROWS=[('r', 0x1000, 0x1002, 't', 'void r()')])
        _, body = transcribe_rows.transcribe(c, img)
        self.assertIn('		int 3		// 0x00001000', body)


    def test_register_plus_absolute_operand_is_named(self):
        # lea edx, [eax + 0x10123da0]; mov dword ptr [ecx*4 + 0x10123cf8], esi; ret
        code = bytes([0x8d, 0x90, 0xa0, 0x3d, 0x12, 0x10, 0x89, 0x34, 0x8d, 0xf8, 0x3c, 0x12, 0x10, 0xc3])
        img = transcribe_rows.Image(code, 0x1000)
        c = SimpleNamespace(ROWS=[('r', 0x1000, 0x100e, 't', 'void r()')],
                            DATA={0x10123da0: 'nxNormals', 0x10123cf8: 'nxMaterial'})
        _, body = transcribe_rows.transcribe(c, img)
        self.assertIn('lea edx, [eax + nxNormals]', body)
        self.assertIn('mov dword ptr [ecx*4 + nxMaterial], esi', body)

    def test_unnamed_register_plus_absolute_operand_raises(self):
        code = bytes([0x8d, 0x90, 0xa0, 0x3d, 0x12, 0x10, 0xc3])
        img = transcribe_rows.Image(code, 0x1000)
        c = SimpleNamespace(ROWS=[('r', 0x1000, 0x1007, 't', 'void r()')])
        with self.assertRaises(ValueError):
            transcribe_rows.transcribe(c, img)


    def test_call_to_a_label_inside_the_row(self):
        # 0x1000: call 0x1006; ret; nop; nop; 0x1006: ret
        img = transcribe_rows.Image(bytes([0xe8, 0x01, 0x00, 0x00, 0x00, 0xc3, 0xc3]), 0x1000)
        c = SimpleNamespace(ROWS=[('r', 0x1000, 0x1007, 't', 'void r()')])
        _, body = transcribe_rows.transcribe(c, img)
        self.assertIn('call L1006', body)
        self.assertIn('L1006:', body)


    def test_unnamed_writable_data_operand_raises(self):
        # mov eax, dword ptr [0x10002000]; ret -- 0x2000 is a writable section the config does not name
        code = bytes([0xa1, 0x00, 0x20, 0x00, 0x10, 0xc3])
        img = transcribe_rows.Image(None, None, [(0x1000, 0x1006, code, 0, False), (0x2000, 0x2004, bytes(4), 0, True)])
        c = SimpleNamespace(ROWS=[('r', 0x1000, 0x1006, 't', 'void r()')])
        with self.assertRaises(ValueError):
            transcribe_rows.transcribe(c, img)

    def test_unnamed_image_address_immediate_raises(self):
        # mov eax, 0x10002000; ret -- an in-image address the config neither pushes nor names
        code = bytes([0xb8, 0x00, 0x20, 0x00, 0x10, 0xc3])
        img = transcribe_rows.Image(None, None, [(0x1000, 0x1006, code, 0, False), (0x2000, 0x2004, bytes(4), 0, False)])
        c = SimpleNamespace(ROWS=[('r', 0x1000, 0x1006, 't', 'void r()')])
        with self.assertRaises(ValueError):
            transcribe_rows.transcribe(c, img)


    def test_cmp_with_an_image_address_is_named(self):
        # cmp esi, 0x10002004; ret -- a pointer compared with the address of a named object's field
        code = bytes([0x81, 0xfe, 0x04, 0x20, 0x00, 0x10, 0xc3])
        img = transcribe_rows.Image(None, None, [(0x1000, 0x1007, code, 0, False), (0x2000, 0x2010, bytes(16), 0, True)])
        c = SimpleNamespace(ROWS=[('r', 0x1000, 0x1007, 't', 'void r()')], PUSHES={0x10002004: 'qh_qh + 0x4'})
        _, body = transcribe_rows.transcribe(c, img)
        self.assertIn('cmp esi, offset qh_qh + 0x4', body)

    def test_member_rows_emit_no_prototype(self):
        img = transcribe_rows.Image(bytes([0xc3, 0xc3]), 0x1000)
        c = SimpleNamespace(ROWS=[('a', 0x1000, 0x1001, 't', '__declspec(naked) float Triangle::Area() const'),
                                  ('b', 0x1001, 0x1002, 't', 'extern "C" __declspec(naked) void qh_x()')])
        protos, _ = transcribe_rows.transcribe(c, img)
        self.assertEqual(protos, 'extern "C" void qh_x();\n')


class WriteTest(unittest.TestCase):
    def test_header_and_append_and_crlf(self):
        with tempfile.TemporaryDirectory() as d:
            head = Path(d) / 'head.txt'
            head.write_text('// top\n@@PROTOS@@\n@@BODY@@\n// tail\n')
            dest = Path(d) / 'out.cpp'
            transcribe_rows.write(dest, 'void a();\n', 'BODY-A\n', header=head)
            raw = dest.read_bytes()
            self.assertIn(b'\r\n', raw)
            self.assertNotIn(b'\n', raw.replace(b'\r\n', b''))
            transcribe_rows.write(dest, 'void b();\n', 'BODY-B\n', append=True)
            text = dest.read_bytes().decode()
            self.assertTrue(text.startswith('// top\r\n'))
            self.assertTrue(text.endswith('// @@ROWS-END@@\r\n// tail\r\n'))
            self.assertLess(text.index('BODY-A'), text.index('BODY-B'))
            self.assertLess(text.index('void b();'), text.index('BODY-A'))

    def test_append_skips_constants_already_defined(self):
        with tempfile.TemporaryDirectory() as d:
            dest = Path(d) / 'out.cpp'
            dest.write_text('static const unsigned kA = 0x1;\n// @@PROTOS-END@@\n// @@ROWS-END@@\n')
            body = 'static const unsigned kA = 0x1;\nstatic const char kS[] = "x";\nBODY\n'
            transcribe_rows.write(dest, 'void b();\n', body, append=True)
            text = dest.read_text()
            self.assertEqual(text.count('static const unsigned kA'), 1)
            self.assertEqual(text.count('static const char kS[]'), 1)

    def test_append_without_marker_raises(self):
        with tempfile.TemporaryDirectory() as d:
            dest = Path(d) / 'out.cpp'
            dest.write_text('no marker\n')
            with self.assertRaises(ValueError):
                transcribe_rows.write(dest, 'void b();\n', 'B\n', append=True)


if __name__ == '__main__':
    unittest.main()
