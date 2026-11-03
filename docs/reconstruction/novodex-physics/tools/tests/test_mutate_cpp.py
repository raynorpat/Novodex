import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import mutate_cpp as mc

SRC = '''#include <x.h>
// a < b in a comment
int A::f(int x)
{
	const char* s = "a < b";
	if(x < 3 && x != 7)
		return x + 2;
	return 0;
}

int A::g(int y) const
{
	return y * 3;
}

// phys_fn_000123 (0x00001000, 99 B)
// notes about it
static int helper(int q)
{
	for(int i = 0; i < q; ++i) q -= 4;
	return q;
}
'''


class SpanTests(unittest.TestCase):
    def body(self, text, span):
        return text[span[0]:span[1]]

    def test_span_finds_member_definition(self):
        span = mc.find_function_span(SRC, 'A::f(int)')
        self.assertTrue(self.body(SRC, span).startswith('{') and self.body(SRC, span).endswith('}'))
        self.assertIn('x + 2', self.body(SRC, span))
        self.assertNotIn('y * 3', self.body(SRC, span))

    def test_span_const_member(self):
        span = mc.find_function_span(SRC, 'A::g(int) const')
        self.assertIn('y * 3', self.body(SRC, span))

    def test_span_ambiguous_overload_returns_none(self):
        text = 'int B::h(int a) { return a; }\nint B::h(float a) { return 1; }\n'
        self.assertIsNone(mc.find_function_span(text, 'B::h'))
        self.assertIsNotNone(mc.find_function_span(text, 'B::h(float)'))

    def test_span_ignores_calls_and_comments(self):
        text = 'void k() { A::f(1); }\n// A::f(int) { fake }\nint A::f(int x) { return 2; }\n'
        span = mc.find_function_span(text, 'A::f(int)')
        self.assertEqual(text[span[0]:span[1]], '{ return 2; }')

    def test_span_by_marker(self):
        span = mc.find_span_by_marker(SRC, 'phys_fn_000123')
        self.assertIn('q -= 4', SRC[span[0]:span[1]])
        self.assertIsNone(mc.find_span_by_marker(SRC, 'phys_fn_999999'))


class MutationTests(unittest.TestCase):
    def setUp(self):
        self.span = mc.find_function_span(SRC, 'A::f(int)')
        self.muts = mc.cpp_mutations(SRC, *self.span)

    def test_mutations_skip_comments_and_strings(self):
        for m in mc.cpp_mutations(SRC, 0, len(SRC)):
            line = SRC.count('\n', 0, m[0])
            self.assertNotIn(line, (1, 4))

    def test_kinds_present(self):
        kinds = {m[3] for m in self.muts}
        self.assertTrue({'cmp', 'arith', 'const', 'neg'} <= kinds, kinds)

    def test_apply_changes_exactly_one_site(self):
        m = self.muts[0]
        out = mc.apply_mutation(SRC, m)
        self.assertNotEqual(out, SRC)
        off, old, new = m[0], m[1], m[2]
        self.assertEqual(out[:off], SRC[:off])
        self.assertEqual(out[off:off + len(new)], new)
        self.assertEqual(out[off + len(new):], SRC[off + len(old):])

    def test_apply_refuses_stale_mutation(self):
        m = self.muts[0]
        with self.assertRaises(ValueError):
            mc.apply_mutation('x' * len(SRC), m)

    def test_spread_is_even_and_deterministic(self):
        items = [(i, 'a', 'b', 'cmp') for i in range(20)]
        got = mc.spread(items, 5)
        self.assertEqual(len(got), 5)
        self.assertEqual(got, mc.spread(items, 5))
        self.assertEqual((got[0], got[-1]), (items[0], items[-1]))
        self.assertEqual(mc.spread(items, 50), items)


import struct


def make_pe(section_bytes, va=0x1000, raw=0x400, debug=None):
    """A minimal PE32 image: headers, one section, optionally a debug directory entry whose raw data is the last 16 bytes."""
    head = bytearray(raw)
    head[0:2] = b'MZ'
    struct.pack_into('<I', head, 0x3c, 0x80)
    head[0x80:0x84] = b'PE\0\0'
    struct.pack_into('<HHIIIHH', head, 0x84, 0x14c, 1, 0x11111111, 0, 0, 0xe0, 0x2102)
    opt = 0x98
    struct.pack_into('<H', head, opt, 0x10b)
    struct.pack_into('<I', head, opt + 28, 0x10000000)
    struct.pack_into('<I', head, opt + 60, raw)
    struct.pack_into('<I', head, opt + 64, 0xcafe0001)   # checksum
    struct.pack_into('<I', head, opt + 92, 16)
    sec = 0x80 + 24 + 0xe0
    head[sec:sec + 8] = b'.text\0\0\0'
    struct.pack_into('<IIII', head, sec + 8, len(section_bytes), va, len(section_bytes), raw)
    if debug is not None:
        dd = opt + 96 + 6 * 8
        struct.pack_into('<II', head, dd, va + len(section_bytes) - 48, 28)
    body = bytearray(section_bytes)
    if debug is not None:
        struct.pack_into('<IIHHIIII', body, len(body) - 48, 0, debug, 0, 0, 2, 16, va + len(body) - 16, raw + len(body) - 16)
    return bytes(head) + bytes(body)


class PeTests(unittest.TestCase):
    def test_inside_requires_all_changes_in_range(self):
        self.assertTrue(mc.inside([(0x1010, 0x1012), (0x1020, 0x1021)], 0x1000, 0x40))
        self.assertFalse(mc.inside([(0x1010, 0x1012), (0x1040, 0x1041)], 0x1000, 0x40))

    def test_inside_false_when_nothing_changed(self):
        self.assertFalse(mc.inside([], 0x1000, 0x40))

    def test_pe_diff_ignores_header_timestamp_and_checksum(self):
        a = make_pe(bytes(range(64)) + bytes(64))
        b = bytearray(a)
        struct.pack_into('<I', b, 0x88, 0x22222222)
        struct.pack_into('<I', b, 0x98 + 64, 0xcafe0002)
        self.assertEqual(mc.pe_changed_rvas(a, bytes(b)), [])

    def test_pe_diff_maps_file_offset_to_rva(self):
        a = make_pe(bytes(range(64)) + bytes(64))
        b = bytearray(a)
        b[0x400 + 5] ^= 0xff
        b[0x400 + 6] ^= 0xff
        b[0x400 + 20] ^= 0xff
        self.assertEqual(mc.pe_changed_rvas(a, bytes(b)), [(0x1005, 0x1007), (0x1014, 0x1015)])

    def test_pe_diff_ignores_debug_directory_data(self):
        a = make_pe(bytes(range(64)) + bytes(112), debug=1)
        b = bytearray(a)
        b[-1] ^= 0xff
        b[-40] ^= 0xff
        self.assertEqual(mc.pe_changed_rvas(a, bytes(b)), [])

    def test_symbol_at_and_matches(self):
        syms = [(0x1000, '?f@A@@QAEHH@Z'), (0x1040, '?g@A@@QBEHH@Z'), (0x1080, '_main')]
        self.assertEqual(mc.symbol_at(syms, 0x1044, 0x1100), ('?g@A@@QBEHH@Z', 0x1040, 0x1080))
        self.assertEqual(mc.symbol_at(syms, 0x1090, 0x1100), ('_main', 0x1080, 0x1100))
        self.assertIsNone(mc.symbol_at(syms, 0x0fff, 0x1100))
        self.assertTrue(mc.symbol_matches('?f@A@@QAEHH@Z', 'A::f'))
        self.assertFalse(mc.symbol_matches('?g@A@@QBEHH@Z', 'A::f'))
        self.assertTrue(mc.symbol_matches('?helper@@YAHH@Z', 'helper'))
        self.assertTrue(mc.symbol_matches('_qh_facetvertices', 'qh_facetvertices'))
        self.assertTrue(mc.symbol_matches('??0A@@QAE@XZ', 'A::A'))

    def test_changes_inside_one_function_named_like_the_row(self):
        syms = [(0x1000, '?f@A@@QAEHH@Z'), (0x1040, '?g@A@@QBEHH@Z')]
        self.assertTrue(mc.in_row_function([(0x1010, 0x1012)], syms, 0x1100, 'A::f'))
        self.assertFalse(mc.in_row_function([(0x1010, 0x1012)], syms, 0x1100, 'A::g'))
        self.assertFalse(mc.in_row_function([(0x1010, 0x1012), (0x1044, 0x1045)], syms, 0x1100, 'A::f'))
        self.assertFalse(mc.in_row_function([], syms, 0x1100, 'A::f'))


class NameTests(unittest.TestCase):
    def test_function_name_at(self):
        span = mc.find_function_span(SRC, 'A::g(int) const')
        self.assertEqual(mc.function_name_at(SRC, span), 'A::g')
        span = mc.find_span_by_marker(SRC, 'phys_fn_000123')
        self.assertEqual(mc.function_name_at(SRC, span), 'helper')


import tempfile


class SweepTests(unittest.TestCase):
    """sweep_row with a fake compiler: the 'DLL' is a PE whose bytes change where the test says."""

    def setUp(self):
        self.dir = tempfile.mkdtemp()
        self.path = Path(self.dir) / 'f.cpp'
        self.path.write_text(SRC, encoding='utf-8')
        self.span = mc.find_function_span(SRC, 'A::f(int)')
        self.muts = mc.spread(mc.cpp_mutations(SRC, *self.span), 4)
        self.base = make_pe(bytes(256))
        self.syms = [(0x1000, '?f@A@@QAEHH@Z'), (0x1080, '?g@A@@QBEHH@Z')]
        self.end = 0x1100
        self.rvas = [0x1010, 0x1020, 0x1090, 0x1030]    # the third mutation lands in A::g
        self.built = []

    def build_dll(self):
        k = len(self.built)
        self.built.append(self.path.read_text(encoding='utf-8'))
        body = bytearray(256)
        body[self.rvas[k % len(self.rvas)] - 0x1000] = 0xff
        return make_pe(bytes(body))

    def sweep(self, run, survey=False, control=None):
        return mc.sweep_row(str(self.path), 'A::f', self.span, self.syms, self.end, self.base, run, self.build_dll,
                            4, survey=survey, control=control)

    def test_sweep_restores_source_when_run_raises(self):
        def run(_):
            raise RuntimeError('boom')
        with self.assertRaises(RuntimeError):
            self.sweep(run)
        self.assertEqual(self.path.read_text(encoding='utf-8'), SRC)

    def test_sweep_rejects_mutation_outside_range(self):
        calls = []
        def run(_):
            calls.append(1)
            return None
        out = self.sweep(run, survey=True)
        self.assertEqual(len(out['rejected']), 1)
        self.assertEqual(len(calls), 3)

    def test_sweep_stops_at_first_catch_unless_survey(self):
        n = []
        def run(_):
            n.append(1)
            return 5
        out = self.sweep(run)
        self.assertEqual(len(out['caught']), 1)
        self.assertEqual(len(n), 1)
        n.clear()
        self.built.clear()
        out = self.sweep(run, survey=True)
        self.assertEqual(len(out['caught']), 3)

    def test_survey_lists_survivors(self):
        out = self.sweep(lambda _: None, survey=True)
        self.assertEqual((len(out['caught']), len(out['survived']), len(out['rejected'])), (0, 3, 1))

    def test_control_run_must_be_exact(self):
        with self.assertRaises(mc.ControlNotExact):
            self.sweep(lambda _: 1, control=lambda: 2)
        self.assertEqual(self.path.read_text(encoding='utf-8'), SRC)



TWO = """int A::f(int x)
{
	if(x < 3) return x + 2;
	return 5;
}

int A::g(int y)
{
	if(y > 4) return y - 3;
	return 6;
}
"""


class BatchTests(unittest.TestCase):
    def setUp(self):
        self.dir = Path(tempfile.mkdtemp())
        self.files = {}
        self.entries = []
        for k, (name, fam) in enumerate((('a', 'fa'), ('b', 'fb'), ('c', 'fc'))):
            p = self.dir / (name + '.cpp')
            p.write_text(SRC, encoding='utf-8')
            span = mc.find_function_span(SRC, 'A::f(int)')
            self.entries.append({'id': 'phys_fn_%06d' % k, 'path': str(p), 'qualified': 'A::f', 'span': span, 'family': fam,
                                 'rva': 0x1000 + 0x40 * k})
        self.syms = [(0x1000, '?f@A@@QAEHH@Z'), (0x1040, '?f@A@@QAEHH@Z'), (0x1080, '?f@A@@QAEHH@Z')]
        self.base = make_pe(bytes(256))
        self.seen = []

    def build_all(self):
        self.seen.append({e['id']: Path(e['path']).read_text(encoding='utf-8') for e in self.entries})
        body = bytearray(256)
        for e in self.entries:
            if Path(e['path']).read_text(encoding='utf-8') != SRC and not e.get('nochange'):
                body[e['rva'] - 0x1000 + 4] = 0xff
        return make_pe(bytes(body))

    def batch(self, run_family, entries=None, limit=3):
        return mc.sweep_batch(entries or self.entries, self.syms, 0x1100, self.base, run_family, self.build_all, limit)

    def test_batch_attributes_by_family(self):
        out = self.batch(lambda fam: 7 if fam == 'fb' else None)
        caught = {i: len(v['candidates']) for i, v in out.items()}
        self.assertEqual(caught, {'phys_fn_000000': 0, 'phys_fn_000001': 3, 'phys_fn_000002': 0})
        self.assertEqual(len(out['phys_fn_000000']['survived']), 3)

    def test_batch_refuses_shared_family(self):
        self.entries[1]['family'] = 'fa'
        with self.assertRaises(ValueError):
            self.batch(lambda fam: None)

    def test_batch_restores_every_file_even_when_run_raises(self):
        def run(fam):
            raise RuntimeError('boom')
        with self.assertRaises(RuntimeError):
            self.batch(run)
        for e in self.entries:
            self.assertEqual(Path(e['path']).read_text(encoding='utf-8'), SRC)

    def test_batch_applies_every_row_before_one_build(self):
        self.batch(lambda fam: None, limit=2)
        self.assertEqual(len(self.seen), 2)
        self.assertTrue(all(t != SRC for t in self.seen[0].values()))

    def test_batch_rejects_a_row_whose_function_did_not_change(self):
        self.entries[2]['nochange'] = True
        out = self.batch(lambda fam: None)
        self.assertEqual(len(out['phys_fn_000002']['rejected']), 3)

    def test_same_file_rows_get_all_their_mutations(self):
        p = self.dir / 'two.cpp'
        p.write_text(TWO, encoding='utf-8')
        f = mc.find_function_span(TWO, 'A::f(int)')
        g = mc.find_function_span(TWO, 'A::g(int)')
        entries = [{'id': 'f', 'path': str(p), 'qualified': 'A::f', 'span': f, 'family': 'x', 'rva': 0x1000},
                   {'id': 'g', 'path': str(p), 'qualified': 'A::g', 'span': g, 'family': 'y', 'rva': 0x1040}]
        seen = []
        def build():
            seen.append(p.read_text(encoding='utf-8'))
            return self.base
        mc.sweep_batch(entries, self.syms, 0x1100, self.base, lambda fam: None, build, 1)
        mf = TWO[f[0]:f[1]]
        mg = TWO[g[0]:g[1]]
        self.assertNotIn(mf, seen[0])
        self.assertNotIn(mg, seen[0])
        self.assertEqual(p.read_text(encoding='utf-8'), TWO)



class ManifestTests(unittest.TestCase):
    LOG = [
        'SURVEY phys_fn_000001 NxPhysicsJointSlotTests tried=2 caught=2 survived=0 rejected=1',
        'CAUGHT phys_fn_000001 NxPhysicsJointSlotTests stdout_delta=2842 line 935: determinant == 0.0f  ->  !(determinant == 0.0f)  (neg)',
        'CAUGHT phys_fn_000001 NxPhysicsJointSlotTests stdout_delta=16 line 1045: 4  ->  5  (const)',
        'CAUGHT phys_fn_000002 NxPhysicsCollisionTests mismatches=3 line 12: a < b  ->  a <= b  (cmp)',
        'NOT CAUGHT phys_fn_000003 (4 survived, 0 rejected)']

    def test_manifest_from_log(self):
        m = mc.write_manifest(self.LOG, 'evidence/raw/x/survey.log',
                              {'phys_fn_000001': 'Physics/src/core/RevoluteJoint.cpp', 'phys_fn_000002': 'Physics/src/Distance.cpp'},
                              {'phys_fn_000002': 'convex_mesh_faces'})
        self.assertEqual(sorted(m['rows']), ['phys_fn_000001', 'phys_fn_000002'])
        a = m['rows']['phys_fn_000001']
        self.assertEqual((a['gate'], a['family'], a['detected'], a['source_mutation']),
                         ('NxPhysicsJointSlotTests', 'NxPhysicsJointSlotTests', 'stdout_delta=2842', True))
        self.assertTrue(a['mutation'].startswith('Physics/src/core/RevoluteJoint.cpp line 935'))
        b = m['rows']['phys_fn_000002']
        self.assertEqual((b['gate'], b['family'], b['detected']), ('NxPhysicsCollisionTests', 'convex_mesh_faces', 'mismatches=3'))
        self.assertEqual(m['doc'], 'evidence/raw/x/survey.log')

    def test_manifest_skips_rows_without_a_catch(self):
        m = mc.write_manifest(self.LOG, 'd', {'phys_fn_000001': 'a.cpp', 'phys_fn_000002': 'b.cpp'}, {})
        self.assertNotIn('phys_fn_000003', m['rows'])



class CrlfTests(unittest.TestCase):
    def test_spans_index_the_bytes_that_are_rewritten(self):
        crlf = SRC.replace(chr(10), chr(13) + chr(10))
        d = Path(tempfile.mkdtemp())
        path = d / 'c.cpp'
        path.write_bytes(crlf.encode('latin-1'))
        text = path.read_bytes().decode('latin-1')
        span = mc.find_function_span(text, 'A::f(int)')
        seen = []

        def build():
            seen.append(path.read_bytes().decode('latin-1'))
            return make_pe(bytes(256))
        mc.sweep_row(str(path), 'A::f', span, [(0x1000, '?f@A@@QAEHH@Z')], 0x1100, make_pe(bytes(256)), lambda _t: None, build, 3, survey=True)
        self.assertEqual(len(seen), 3)
        for t in seen:
            self.assertEqual(t[:span[0]], text[:span[0]])
            self.assertTrue(t.endswith(text[span[1]:]))
            self.assertIn(chr(13) + chr(10), t)
        self.assertEqual(path.read_bytes(), crlf.encode('latin-1'))



class OracleOnlyTests(unittest.TestCase):
    def test_output_lines_drop_the_pair_header(self):
        out = chr(10).join(['pair_directory=D:\\x', 'modules pair=2', 'loaded module=NxPhysics.dll', 'case a ok', 'case b ok'])
        self.assertEqual(mc.output_lines(out), ['case a ok', 'case b ok'])

    def test_no_difference_is_not_a_detection(self):
        self.assertIsNone(mc.digest_difference(['a', 'b'], 'a' + chr(10) + 'b', 0))

    def test_a_changed_line_is_counted(self):
        self.assertEqual(mc.digest_difference(['a', 'b'], 'a' + chr(10) + 'c', 0), 2)

    def test_a_crash_with_the_same_output_counts_only_when_asked(self):
        self.assertIsNone(mc.digest_difference(['a'], 'a', -1073741819))
        self.assertEqual(mc.digest_difference(['a'], 'a', -1073741819, count_crash=True), 1)


if __name__ == '__main__':
    unittest.main()
